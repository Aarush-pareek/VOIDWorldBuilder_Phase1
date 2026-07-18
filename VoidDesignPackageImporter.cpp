// Copyright VOID Engineering Studio. Internal tool - not for shipping content.

#include "VoidDesignPackageImporter.h"
#include "VoidJsonReader.h"
#include "VoidPackageValidator.h"
#include "VoidWorldBuilderLog.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

namespace VoidImportPrivate
{
	static FVector2D JsonValueToVector2D(const TSharedPtr<FJsonValue>& Value)
	{
		const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
		if (Value.IsValid() && Value->TryGetArray(Arr) && Arr->Num() >= 2)
		{
			return FVector2D((*Arr)[0]->AsNumber(), (*Arr)[1]->AsNumber());
		}
		return FVector2D::ZeroVector;
	}
}

FVoidImportResult FVoidDesignPackageImporter::LoadFromFile(const FString& FilePath)
{
	FVoidImportResult Result;

	TSharedPtr<FJsonObject> JsonObject;
	FString ReadError;
	if (!FVoidJsonReader::ReadFromFile(FilePath, JsonObject, ReadError))
	{
		Result.ValidationReport.AddError(ReadError);
		UE_LOG(LogVoidWorldBuilder, Error, TEXT("Import failed: %s"), *ReadError);
		return Result;
	}

	TArray<FString> MappingWarnings;
	Result.Package = MapJsonToPackage(JsonObject, MappingWarnings);

	for (const FString& Warning : MappingWarnings)
	{
		Result.ValidationReport.AddWarning(Warning);
	}

	Result.ValidationReport = FVoidPackageValidator::Validate(Result.Package, Result.ValidationReport);

	UE_LOG(LogVoidWorldBuilder, Log, TEXT("Imported package '%s': valid=%s, errors=%d, warnings=%d"),
		*Result.Package.DistrictId.Value.ToString(),
		Result.ValidationReport.bIsValid ? TEXT("true") : TEXT("false"),
		Result.ValidationReport.NumErrors(),
		Result.ValidationReport.NumWarnings());

	return Result;
}

FVoidImportResult FVoidDesignPackageImporter::LoadFromJsonString(const FString& RawJson)
{
	FVoidImportResult Result;

	TSharedPtr<FJsonObject> JsonObject;
	FString ReadError;
	if (!FVoidJsonReader::ParseString(RawJson, JsonObject, ReadError))
	{
		Result.ValidationReport.AddError(ReadError);
		return Result;
	}

	TArray<FString> MappingWarnings;
	Result.Package = MapJsonToPackage(JsonObject, MappingWarnings);

	for (const FString& Warning : MappingWarnings)
	{
		Result.ValidationReport.AddWarning(Warning);
	}

	Result.ValidationReport = FVoidPackageValidator::Validate(Result.Package, Result.ValidationReport);
	return Result;
}

FVoidDesignPackage FVoidDesignPackageImporter::MapJsonToPackage(const TSharedPtr<FJsonObject>& JsonObject, TArray<FString>& OutMappingWarnings)
{
	using namespace VoidImportPrivate;

	FVoidDesignPackage Package;

	if (!JsonObject.IsValid())
	{
		OutMappingWarnings.Add(TEXT("Root JSON object was invalid; returning empty package."));
		return Package;
	}

	Package.DistrictId = FVoidElementId(FName(*JsonObject->GetStringField(TEXT("districtId"))));

	const TSharedPtr<FJsonObject>* MetadataObject = nullptr;
	if (JsonObject->TryGetObjectField(TEXT("metadata"), MetadataObject))
	{
		Package.Metadata.SourceDocumentName = (*MetadataObject)->GetStringField(TEXT("sourceDocumentName"));
		Package.Metadata.SourceDocumentVersion = (*MetadataObject)->GetStringField(TEXT("sourceDocumentVersion"));
		Package.Metadata.bIsApproved = (*MetadataObject)->GetBoolField(TEXT("isApproved"));
	}
	else
	{
		OutMappingWarnings.Add(TEXT("Package is missing a 'metadata' object."));
	}

	const TArray<TSharedPtr<FJsonValue>>* BuildingsArray = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("buildings"), BuildingsArray))
	{
		for (const TSharedPtr<FJsonValue>& BuildingValue : *BuildingsArray)
		{
			const TSharedPtr<FJsonObject>* BuildingObject;
			if (!BuildingValue->TryGetObject(BuildingObject))
			{
				OutMappingWarnings.Add(TEXT("Skipped a non-object entry in 'buildings' array."));
				continue;
			}

			FVoidBuildingSpec Building;
			Building.Id = FVoidElementId(FName(*(*BuildingObject)->GetStringField(TEXT("id"))));
			Building.HeightUnits = (*BuildingObject)->GetNumberField(TEXT("heightUnits"));
			Building.BuildingType = (*BuildingObject)->GetStringField(TEXT("buildingType"));

			const TArray<TSharedPtr<FJsonValue>>* FootprintArray = nullptr;
			if ((*BuildingObject)->TryGetArrayField(TEXT("footprintCorners"), FootprintArray))
			{
				for (const TSharedPtr<FJsonValue>& CornerValue : *FootprintArray)
				{
					Building.FootprintCorners.Add(JsonValueToVector2D(CornerValue));
				}
			}

			Package.Buildings.Add(MoveTemp(Building));
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* RoadsArray = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("roads"), RoadsArray))
	{
		for (const TSharedPtr<FJsonValue>& RoadValue : *RoadsArray)
		{
			const TSharedPtr<FJsonObject>* RoadObject;
			if (!RoadValue->TryGetObject(RoadObject))
			{
				OutMappingWarnings.Add(TEXT("Skipped a non-object entry in 'roads' array."));
				continue;
			}

			FVoidRoadSpec Road;
			Road.Id = FVoidElementId(FName(*(*RoadObject)->GetStringField(TEXT("id"))));
			Road.WidthUnits = (*RoadObject)->GetNumberField(TEXT("widthUnits"));

			const TArray<TSharedPtr<FJsonValue>>* CenterlineArray = nullptr;
			if ((*RoadObject)->TryGetArrayField(TEXT("centerlinePoints"), CenterlineArray))
			{
				for (const TSharedPtr<FJsonValue>& PointValue : *CenterlineArray)
				{
					Road.CenterlinePoints.Add(JsonValueToVector2D(PointValue));
				}
			}

			Package.Roads.Add(MoveTemp(Road));
		}
	}

	return Package;
}
