#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Mesh/BBSurfaceNets.h"
#include "BBPlanetActor.generated.h"

USTRUCT()
struct FPlanetChunk
{
    GENERATED_BODY()

    FIntVector Position;
    int32 LOD;
    bool bIsVisible;
    int32 MeshSectionIndex;
};

UCLASS()
class BUILDINGBLOCKS_API ABBPlanetActor : public AActor
{
    GENERATED_BODY()

public:
    ABBPlanetActor();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere)
    UProceduralMeshComponent* MeshComponent;

    UPROPERTY(EditAnywhere, Category = "Planet Generation")
    float Radius = 1000.0f;

    UPROPERTY(EditAnywhere, Category = "Planet Generation")
    float NoiseScale = 0.001f;

    UPROPERTY(EditAnywhere, Category = "Planet Generation")
    float NoiseAmplitude = 100.0f;

    UPROPERTY(EditAnywhere, Category = "Planet Generation")
    int32 Resolution = 100;

    UPROPERTY(EditAnywhere, Category = "Planet Generation")
    float VoxelSize = 20.0f;

    UPROPERTY(EditAnywhere, Category = "Planet Generation")
    bool bEstimateNormals = true;

    UPROPERTY(EditAnywhere, Category = "Planet Generation")
    UMaterialInterface* PlanetMaterial;

    // LOD Settings
    UPROPERTY(EditAnywhere, Category = "Planet LOD")
    int32 MaxLODLevels = 6;

    UPROPERTY(EditAnywhere, Category = "Planet LOD")
    float LODDistanceMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Planet LOD")
    float ChunkSize = 32.0f;

private:
    void GeneratePlanet();
    float GetSignedDistance(const FVector& Position) const;
    void UpdateLOD();
    void GenerateChunk(const FPlanetChunk& Chunk);

    TArray<FPlanetChunk> Chunks;
}; 