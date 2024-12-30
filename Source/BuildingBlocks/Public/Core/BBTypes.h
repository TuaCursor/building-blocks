#pragma once

#include "CoreMinimal.h"
#include "BBTypes.generated.h"

USTRUCT(BlueprintType)
struct BUILDINGBLOCKS_API FBBExtent
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuildingBlocks")
    FIntVector Min;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuildingBlocks")
    FIntVector Max;

    FBBExtent() : Min(FIntVector::ZeroValue), Max(FIntVector::ZeroValue) {}
    FBBExtent(const FIntVector& InMin, const FIntVector& InMax) : Min(InMin), Max(InMax) {}

    static FBBExtent FromMinAndMax(const FIntVector& Min, const FIntVector& Max)
    {
        return FBBExtent(Min, Max);
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

    bool Intersects(const FBBExtent& Other) const
    {
        return !(Other.Max.X <= Min.X || Other.Min.X >= Max.X ||
                Other.Max.Y <= Min.Y || Other.Min.Y >= Max.Y ||
                Other.Max.Z <= Min.Z || Other.Min.Z >= Max.Z);
    }
};

typedef FBBExtent FIntBox; 