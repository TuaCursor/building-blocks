#pragma once

#include "CoreMinimal.h"
#include "Core/BBPoint.h"
#include "Core/BBSphere.h"
#include "BBOctree.generated.h"

USTRUCT()
struct FOctreeNode
{
    GENERATED_BODY()

    FIntVector Position;
    int32 Size;
    int32 LOD;
    bool bIsLeaf;
    TArray<FOctreeNode> Children;

    FOctreeNode() : Position(FIntVector::ZeroValue), Size(0), LOD(0), bIsLeaf(true) {}
    FOctreeNode(const FIntVector& InPosition, int32 InSize, int32 InLOD)
        : Position(InPosition), Size(InSize), LOD(InLOD), bIsLeaf(true) {}

    FIntBox GetBounds() const
    {
        return FIntBox(Position, Position + FIntVector(Size));
    }
};

UCLASS()
class BUILDINGBLOCKS_API UBBOctreeLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    static void BuildOctree(
        FOctreeNode& Root,
        const FBBSphere& ViewSphere,
        int32 MaxLOD,
        float LODDistanceMultiplier
    );

    static void GetVisibleChunks(
        const FOctreeNode& Root,
        const FBBSphere& ViewSphere,
        TArray<FOctreeNode>& OutChunks
    );

private:
    static void SubdivideNode(
        FOctreeNode& Node,
        const FBBSphere& ViewSphere,
        int32 MaxLOD,
        float LODDistanceMultiplier
    );

    static bool ShouldSubdivide(
        const FOctreeNode& Node,
        const FBBSphere& ViewSphere,
        int32 MaxLOD,
        float LODDistanceMultiplier
    );

    static bool IsNodeVisible(
        const FOctreeNode& Node,
        const FBBSphere& ViewSphere
    );
}; 