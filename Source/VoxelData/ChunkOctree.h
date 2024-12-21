#pragma once

#include "CoreMinimal.h"
#include "VoxelChunk.h"

class VOXELPLANET_API FOctreeNode 
{
public:
    FOctreeNode(const FIntVector& InPosition, int32 InSize, int32 InLOD);
    
    void Split();
    void Merge();
    void UpdateLOD(const FVector& ViewerPosition);
    
    TSharedPtr<FVoxelChunk> GetChunk() const { return Chunk; }
    
private:
    TArray<TSharedPtr<FOctreeNode>> Children;
    TSharedPtr<FVoxelChunk> Chunk;
    FIntVector Position;
    int32 Size;
    int32 LOD;
    bool bHasChildren;
};

class VOXELPLANET_API FChunkOctree
{
public:
    FChunkOctree();
    
    void UpdateLOD(const FVector& ViewerPosition);
    void GenerateChunks();
    
private:
    TSharedPtr<FOctreeNode> RootNode;
    TArray<TSharedPtr<FVoxelChunk>> ChunksToGenerate;
}; 