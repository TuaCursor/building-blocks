#pragma once

#include "CoreMinimal.h"
#include "Core/BBTypes.h"
#include "BBSphere.generated.h"

USTRUCT(BlueprintType)
struct BUILDINGBLOCKS_API FBBSphere
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuildingBlocks")
    FVector Center;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuildingBlocks")
    float Radius;

    FBBSphere() : Center(FVector::ZeroVector), Radius(0.0f) {}
    FBBSphere(const FVector& InCenter, float InRadius) : Center(InCenter), Radius(InRadius) {}

    bool Contains(const FBBSphere& Other) const;
    bool Intersects(const FBBSphere& Other) const;
    FBBExtent GetBounds() const;

    static FBBSphere FromCenterAndRadius(const FVector& Center, float Radius)
    {
        return FBBSphere(Center, Radius);
    }
}; 