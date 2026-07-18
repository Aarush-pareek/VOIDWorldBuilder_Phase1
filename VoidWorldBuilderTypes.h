// Copyright VOID Engineering Studio. Internal tool - not for shipping content.

#pragma once

#include "CoreMinimal.h"
#include "VoidWorldBuilderTypes.generated.h"

/**
 * Severity of a single validation issue found while importing a design
 * package. Errors block generation; Warnings do not.
 */
UENUM(BlueprintType)
enum class EVoidValidationSeverity : uint8
{
	Warning UMETA(DisplayName = "Warning"),
	Error   UMETA(DisplayName = "Error")
};

/**
 * Stable identifier type for design elements (districts, buildings, roads).
 * A thin FName wrapper rather than a raw FString/FGuid so future generator
 * code passes IDs around with type-checked intent instead of an ambiguous
 * FString that could be a name, a path, or an ID.
 */
USTRUCT(BlueprintType)
struct VOIDWORLDBUILDERCORE_API FVoidElementId
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VOID World Builder")
	FName Value;

	FVoidElementId()
		: Value(NAME_None)
	{
	}

	explicit FVoidElementId(FName InValue)
		: Value(InValue)
	{
	}

	bool IsValid() const
	{
		return Value != NAME_None;
	}

	bool operator==(const FVoidElementId& Other) const
	{
		return Value == Other.Value;
	}

	friend uint32 GetTypeHash(const FVoidElementId& Id)
	{
		return GetTypeHash(Id.Value);
	}
};
