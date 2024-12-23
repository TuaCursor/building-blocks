#pragma once

#include "CoreMinimal.h"

struct FVoxelData
{
    float Distance; // Signed distance value
    bool bIsEmpty;
};

class VOXELPLANET_API FVoxelChunk
{
public:
    static const int32 CHUNK_SIZE = 32;
    
    FVoxelChunk(const FIntVector& InPosition, int32 InLOD);
    
    // Generate chunk data using noise/SDF
    void GenerateData();
    float GenerateNoise(const FVector& WorldPos) const;

    // Get voxel data at local position
    FVoxelData GetVoxel(const FIntVector& LocalPos) const;
    
    // Mesh generation
    void CreateMesh(class UProceduralMeshComponent* MeshComponent);
    
    const FIntVector& GetPosition() const { return Position; }
    int32 GetLOD() const { return LOD; }

    bool IsGenerated() const { return bIsGenerated; }

private:
    TArray<FVoxelData> Data;
    FIntVector Position;
    int32 LOD;
    bool bIsGenerated;
}; 