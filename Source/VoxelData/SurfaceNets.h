#pragma once

#include "CoreMinimal.h"

struct FSurfaceVertex 
{
    FVector Position;
    FVector Normal;
};

class VOXELPLANET_API FSurfaceNets
{
public:
    static void GenerateMesh(
        const TArray<FVoxelData>& VoxelData,
        const FIntVector& ChunkSize,
        TArray<FSurfaceVertex>& OutVertices,
        TArray<int32>& OutIndices
    );
    
private:
    static FVector CalculateVertexPosition(const TArray<FVoxelData>& VoxelData, const FIntVector& Position);
    static FVector CalculateNormal(const TArray<FVoxelData>& VoxelData, const FIntVector& Position);
}; 