#include "ChunkOctree.h"
#include "ProceduralMeshComponent.h"
#include "Async/Async.h"
#include "Async/ParallelFor.h"

FOctreeNode::FOctreeNode(const FIntVector& InPosition, int32 InSize, int32 InLOD, TArray<TSharedPtr<FVoxelChunk>>& InChunksToGenerate)
    : Position(InPosition)
    , Size(InSize)
    , LOD(InLOD)
    , bHasChildren(false)
    , ChunksToGenerate(InChunksToGenerate)
{
    Chunk = MakeShared<FVoxelChunk>(Position, LOD);
    Chunk->GenerateData();
}

void FOctreeNode::Split()
{
    if (bHasChildren || Size <= FVoxelChunk::CHUNK_SIZE)
        return;
        
    bHasChildren = true;
    int32 ChildSize = Size / 2;
    
    // Create 8 children
    for (int32 i = 0; i < 8; i++)
    {
        FIntVector Offset(
            (i & 1) ? ChildSize : 0,
            (i & 2) ? ChildSize : 0,
            (i & 4) ? ChildSize : 0
        );
        
        Children.Add(MakeShared<FOctreeNode>(
            Position + Offset,
            ChildSize,
            LOD - 1,
            ChunksToGenerate
        ));
    }
}

void FOctreeNode::Merge()
{
    if (!bHasChildren)
        return;
        
    Children.Empty();
    bHasChildren = false;
}

void FOctreeNode::UpdateLOD(const FVector& ViewerPosition)
{
    // Calculate distance to viewer
    FVector ChunkCenter = FVector(Position) + FVector(Size/2);
    float Distance = (ViewerPosition - ChunkCenter).Size();
    
    // LOD transition distance threshold based on detail factor
    const float DetailFactor = 6.0f; // Similar to Rust version's "detail" parameter
    float ThresholdBase = Size * DetailFactor;
    
    if (Distance < ThresholdBase && Size > FVoxelChunk::CHUNK_SIZE)
    {
        // If we weren't split before, queue new chunks for generation
        if (!bHasChildren)
        {
            Split();
            
            // Queue child chunks for generation
            for (auto& Child : Children)
            {
                if (!Child->Chunk->IsGenerated())
                {
                    ChunksToGenerate.Add(Child->Chunk);
                }
            }
        }
        
        // Update children
        for (auto& Child : Children)
        {
            Child->UpdateLOD(ViewerPosition);
        }
    }
    else
    {
        Merge();
    }
}

FChunkOctree::FChunkOctree()
{
    const int32 RootSize = 2048; // Adjust based on your needs
    RootNode = MakeShared<FOctreeNode>(
        FIntVector(-RootSize/2),
        RootSize,
        8,  // Max LOD level
        ChunksToGenerate  // Pass reference to member variable
    );
}

void FChunkOctree::UpdateLOD(const FVector& ViewerPosition)
{
    if (RootNode)
    {
        RootNode->UpdateLOD(ViewerPosition);
    }
}

void FChunkOctree::GenerateChunks()
{
    if (!RootNode || ChunksToGenerate.Num() == 0)
        return;

    // Store chunks locally since we'll clear the queue
    TArray<TSharedPtr<FVoxelChunk>> ChunksToProcess = ChunksToGenerate;
    ChunksToGenerate.Empty();

    // Create async task for chunk generation
    Async(EAsyncExecution::ThreadPool, [ChunksToProcess]()
    {
        // Generate all chunks in parallel
        ParallelFor(ChunksToProcess.Num(), [&](int32 Index)
        {
            if (ChunksToProcess[Index])
            {
                ChunksToProcess[Index]->GenerateData();
            }
        });

        // Create meshes on game thread after generation is complete
        AsyncTask(ENamedThreads::GameThread, [ChunksToProcess]()
        {
            for (auto& Chunk : ChunksToProcess)
            {
                if (Chunk && Chunk->IsGenerated())
                {
                    UProceduralMeshComponent* MeshComponent = NewObject<UProceduralMeshComponent>();
                    if (MeshComponent)
                    {
                        MeshComponent->RegisterComponent();
                        Chunk->CreateMesh(MeshComponent);
                    }
                }
            }
        });
    });
} 