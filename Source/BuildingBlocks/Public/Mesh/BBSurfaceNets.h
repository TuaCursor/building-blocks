#pragma once

#include "CoreMinimal.h"
#include "Core/BBTypes.h"
#include "BBSurfaceNets.generated.h"

USTRUCT(BlueprintType)
struct BUILDINGBLOCKS_API FBBPosNormMesh
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FVector> Positions;

    UPROPERTY()
    TArray<FVector> Normals;

    UPROPERTY()
    TArray<int32> Indices;

    void Clear()
    {
        Positions.Empty();
        Normals.Empty();
        Indices.Empty();
    }

    void Append(const FBBPosNormMesh& Other);
};

USTRUCT()
struct BUILDINGBLOCKS_API FBBSurfaceNetsBuffer
{
    GENERATED_BODY()

    UPROPERTY()
    FBBPosNormMesh Mesh;

    UPROPERTY()
    TArray<FIntVector> SurfacePoints;

    UPROPERTY()
    TArray<int32> SurfaceStrides;

    UPROPERTY()
    TArray<int32> StrideToIndex;

    void Reset(int32 ArraySize);
};

UCLASS()
class BUILDINGBLOCKS_API UBBSurfaceNetsLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "BuildingBlocks|SurfaceNets")
    static void GetPaddedSurfaceNetsChunkExtent(const FBBExtent& ChunkExtent, FBBExtent& OutExtent);

    UFUNCTION(BlueprintCallable, Category = "BuildingBlocks|SurfaceNets")
    static void GenerateSurfaceNets(
        const TArray<float>& SignedDistanceField,
        const FIntVector& Dimensions,
        float VoxelSize,
        bool bEstimateNormals,
        FBBSurfaceNetsBuffer& OutBuffer
    );
}; 