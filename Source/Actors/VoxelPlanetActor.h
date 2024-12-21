#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoxelPlanetActor.generated.h"

UCLASS()
class VOXELPLANET_API AVoxelPlanetActor : public AActor
{
    GENERATED_BODY()

public:
    AVoxelPlanetActor();

protected:
    virtual void BeginPlay() override;
    
    UPROPERTY(VisibleAnywhere)
    class UVoxelTerrainComponent* TerrainComponent;
}; 