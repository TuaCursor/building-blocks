#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VoxelData/ChunkOctree.h"
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

private:
    TSharedPtr<FChunkOctree> ChunkOctree;
    void UpdateLOD();
}; 