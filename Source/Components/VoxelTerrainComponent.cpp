#include "VoxelTerrainComponent.h"
#include "GameFramework/PlayerController.h"

UVoxelTerrainComponent::UVoxelTerrainComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    ChunkOctree = MakeShared<FChunkOctree>();
}

void UVoxelTerrainComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Initial generation
    ChunkOctree->GenerateChunks();
}

void UVoxelTerrainComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    UpdateLOD();
}

void UVoxelTerrainComponent::UpdateLOD()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
        return;
        
    FVector ViewerPosition = PC->GetPawn()->GetActorLocation();
    
    // Update LOD based on viewer position
    ChunkOctree->UpdateLOD(ViewerPosition * LODDistanceMultiplier);
} 