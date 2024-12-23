#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VoxelPlanet/VoxelData/ChunkOctree.h"
#include "VoxelTerrainComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VOXELPLANET_API UVoxelTerrainComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UVoxelTerrainComponent();
    
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    
    UPROPERTY(EditAnywhere, Category = "Voxel")
    float LODDistanceMultiplier = 1.0f;

protected:
    virtual void BeginPlay() override;
    TSharedPtr<FChunkOctree> ChunkOctree;
    
private:
    void UpdateLOD();
}; 