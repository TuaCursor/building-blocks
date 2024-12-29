#pragma once

#include "CoreMinimal.h"
#include "BBPoint.generated.h"

USTRUCT(BlueprintType)
struct BUILDINGBLOCKS_API FBBBox
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuildingBlocks")
    FIntVector Min;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuildingBlocks")
    FIntVector Max;

    FBBBox() : Min(FIntVector::ZeroValue), Max(FIntVector::ZeroValue) {}
    FBBBox(const FIntVector& InMin, const FIntVector& InMax) : Min(InMin), Max(InMax) {}

    static FBBBox FromMinAndMax(const FIntVector& Min, const FIntVector& Max)
    {
        return FBBBox(Min, Max);
    }

    FIntVector GetSize() const
    {
        return Max - Min;
    }

    bool Contains(const FIntVector& Point) const
    {
        return Point.X >= Min.X && Point.X < Max.X &&
               Point.Y >= Min.Y && Point.Y < Max.Y &&
               Point.Z >= Min.Z && Point.Z < Max.Z;
    }

    bool Intersects(const FBBBox& Other) const
    {
        return !(Other.Max.X <= Min.X || Other.Min.X >= Max.X ||
                Other.Max.Y <= Min.Y || Other.Min.Y >= Max.Y ||
                Other.Max.Z <= Min.Z || Other.Min.Z >= Max.Z);
    }
};

// Alias for FBBBox to match the Rust naming
typedef FBBBox FIntBox; 