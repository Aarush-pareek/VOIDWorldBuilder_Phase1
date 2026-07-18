// Copyright VOID Engineering Studio. Internal tool - not for shipping content.

#include "VoidWorldBuilderCommandlet.h"
#include "VoidDesignPackageImporter.h"
#include "VoidWorldBuilderLog.h"

UVoidWorldBuilderCommandlet::UVoidWorldBuilderCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UVoidWorldBuilderCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParamsMap;
	ParseCommandLine(*Params, Tokens, Switches, ParamsMap);

	const FString* PackagePathPtr = ParamsMap.Find(TEXT("Package"));
	if (!PackagePathPtr || PackagePathPtr->IsEmpty())
	{
		UE_LOG(LogVoidWorldBuilder, Error, TEXT("VoidWorldBuilderCommandlet requires -Package=\"<path to json>\""));
		return 1;
	}

	const FVoidImportResult Result = FVoidDesignPackageImporter::LoadFromFile(*PackagePathPtr);

	UE_LOG(LogVoidWorldBuilder, Log, TEXT("Package '%s' - valid: %s, errors: %d, warnings: %d"),
		*Result.Package.DistrictId.Value.ToString(),
		Result.WasSuccessful() ? TEXT("true") : TEXT("false"),
		Result.ValidationReport.NumErrors(),
		Result.ValidationReport.NumWarnings());

	for (const FVoidValidationIssue& Issue : Result.ValidationReport.Issues)
	{
		const bool bIsError = (Issue.Severity == EVoidValidationSeverity::Error);
		UE_LOG(LogVoidWorldBuilder, Log, TEXT("  [%s] %s (%s)"),
			bIsError ? TEXT("ERROR") : TEXT("WARN"),
			*Issue.Message,
			*Issue.FieldPath);
	}

	// Non-zero exit code on validation failure so CI treats it as a failed check.
	return Result.WasSuccessful() ? 0 : 1;
}
