#include "Core/BBSphere.h"

bool FBBSphere::Contains(const FBBSphere& Other) const
{
    const float Distance = FVector::Distance(Center, Other.Center);
    return Distance + Other.Radius < Radius;
}

bool FBBSphere::Intersects(const FBBSphere& Other) const
{
    const float Distance = FVector::Distance(Center, Other.Center);
    return Distance - Other.Radius < Radius;
}

FBox FBBSphere::GetBounds() const
{
    const FVector Extent(Radius);
    return FBox(Center - Extent, Center + Extent);
} 