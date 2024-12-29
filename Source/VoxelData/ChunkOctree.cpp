#include "ChunkOctree.h"
#include "ProceduralMeshComponent.h"
#include "Async/TaskGraphInterfaces.h"

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

    // Use Unreal's task graph system instead of ParallelFor
    FGraphEventArray Tasks;
    Tasks.Reserve(ChunksToGenerate.Num());

    for (int32 Index = 0; Index < ChunksToGenerate.Num(); Index++)
    {
        auto& ChunkToGenerate = ChunksToGenerate[Index];
        
        Tasks.Add(FFunctionGraphTask::CreateAndDispatchWhenReady([ChunkToGenerate]()
        {
            // Generate chunk data using noise/SDF
            ChunkToGenerate->GenerateData();
        }, TStatId(), nullptr, ENamedThreads::AnyBackgroundThreadNormalTask));
    }

    // Wait for all tasks to complete
    FTaskGraphInterface::Get().WaitUntilTasksComplete(Tasks);

    // Create meshes on the game thread
    FTaskGraphInterface::Get().WaitUntilTasksComplete(FFunctionGraphTask::CreateAndDispatchWhenReady([this]()
    {
        for (auto& ChunkToGenerate : ChunksToGenerate)
        {
            // Create procedural mesh component for this chunk
            UProceduralMeshComponent* MeshComponent = NewObject<UProceduralMeshComponent>();
            if (MeshComponent)
            {
                MeshComponent->RegisterComponent();

                // Generate mesh using surface nets
                ChunkToGenerate->CreateMesh(MeshComponent);
            }
        }
    }, TStatId(), nullptr, ENamedThreads::GameThread));

    // Clear the generation queue
    ChunksToGenerate.Empty();
} 