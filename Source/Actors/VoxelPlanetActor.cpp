#include "VoxelPlanetActor.h"
#include "VoxelPlanet/Components/VoxelTerrainComponent.h"

AVoxelPlanetActor::AVoxelPlanetActor()
{
    PrimaryActorTick.bCanEverTick = true;
    
    TerrainComponent = CreateDefaultSubobject<UVoxelTerrainComponent>(TEXT("TerrainComponent"));
}

void AVoxelPlanetActor::BeginPlay()
{
    Super::BeginPlay();
} 