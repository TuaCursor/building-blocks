#include "BBPlanetActor.h"
#include "ProceduralMeshComponent.h"
#include "Storage/BBOctree.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

namespace
{
    // Simple 3D noise function based on sine waves
    float SimpleNoise3D(float X, float Y, float Z)
    {
        const float Frequency = 1.0f;
        return (
            FMath::Sin(X * Frequency) * 
            FMath::Sin(Y * Frequency) * 
            FMath::Sin(Z * Frequency) +
            FMath::Sin(X * Frequency * 2.1f) * 
            FMath::Sin(Y * Frequency * 2.3f) * 
            FMath::Sin(Z * Frequency * 2.7f) * 0.5f
        ) * 0.5f;
    }
}

ABBPlanetActor::ABBPlanetActor()
{
    PrimaryActorTick.bCanEverTick = true;

    MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("PlanetMesh"));
    RootComponent = MeshComponent;
    MeshComponent->bUseAsyncCooking = true;
}

void ABBPlanetActor::BeginPlay()
{
    Super::BeginPlay();
    GeneratePlanet();
}

void ABBPlanetActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateLOD();
}

void ABBPlanetActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    GeneratePlanet();
}

float ABBPlanetActor::GetSignedDistance(const FVector& Position) const
{
    const float DistanceFromCenter = Position.Size();
    
    // Base sphere SDF
    float Distance = DistanceFromCenter - Radius;

    // Add noise
    if (NoiseAmplitude > 0.0f)
    {
        const FVector NormalizedPos = Position / DistanceFromCenter;
        const float NoiseValue = SimpleNoise3D(
            NormalizedPos.X * NoiseScale,
            NormalizedPos.Y * NoiseScale,
            NormalizedPos.Z * NoiseScale
        );
        
        Distance += NoiseValue * NoiseAmplitude;
    }

    return Distance;
}

void ABBPlanetActor::UpdateLOD()
{
    // Get camera position
    APlayerCameraManager* CameraManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
    if (!CameraManager)
    {
        return;
    }

    FVector CameraLocation = CameraManager->GetCameraLocation();
    
    // Create view sphere for LOD calculation
    FBBSphere ViewSphere(CameraLocation, ChunkSize * LODDistanceMultiplier);

    // Create root node for octree
    const int32 RootSize = FMath::RoundUpToPowerOfTwo(Resolution);
    FOctreeNode RootNode(FIntVector::ZeroValue, RootSize, 0);

    // Build octree
    UBBOctreeLibrary::BuildOctree(RootNode, ViewSphere, MaxLODLevels, LODDistanceMultiplier);

    // Get visible chunks
    TArray<FOctreeNode> VisibleNodes;
    UBBOctreeLibrary::GetVisibleChunks(RootNode, ViewSphere, VisibleNodes);

    // Update chunks
    for (const FOctreeNode& Node : VisibleNodes)
    {
        FPlanetChunk NewChunk;
        NewChunk.Position = Node.Position;
        NewChunk.LOD = Node.LOD;
        NewChunk.bIsVisible = true;
        NewChunk.MeshSectionIndex = Chunks.Num();

        bool bChunkExists = false;
        for (FPlanetChunk& ExistingChunk : Chunks)
        {
            if (ExistingChunk.Position == Node.Position && ExistingChunk.LOD == Node.LOD)
            {
                ExistingChunk.bIsVisible = true;
                bChunkExists = true;
                break;
            }
        }

        if (!bChunkExists)
        {
            Chunks.Add(NewChunk);
            GenerateChunk(NewChunk);
        }
    }

    // Hide chunks that are no longer visible
    for (FPlanetChunk& Chunk : Chunks)
    {
        bool bStillVisible = false;
        for (const FOctreeNode& Node : VisibleNodes)
        {
            if (Chunk.Position == Node.Position && Chunk.LOD == Node.LOD)
            {
                bStillVisible = true;
                break;
            }
        }

        if (!bStillVisible)
        {
            Chunk.bIsVisible = false;
            MeshComponent->SetMeshSectionVisible(Chunk.MeshSectionIndex, false);
        }
    }
}

void ABBPlanetActor::GenerateChunk(const FPlanetChunk& Chunk)
{
    const int32 ChunkResolution = Resolution >> Chunk.LOD;
    const float ChunkVoxelSize = VoxelSize * (1 << Chunk.LOD);
    
    // Create the signed distance field for this chunk
    const FIntVector Dimensions(ChunkResolution, ChunkResolution, ChunkResolution);
    const float HalfSize = (ChunkResolution * ChunkVoxelSize) * 0.5f;
    TArray<float> SignedDistanceField;
    SignedDistanceField.SetNum(ChunkResolution * ChunkResolution * ChunkResolution);

    // Fill the SDF
    for (int32 Z = 0; Z < ChunkResolution; ++Z)
    {
        for (int32 Y = 0; Y < ChunkResolution; ++Y)
        {
            for (int32 X = 0; X < ChunkResolution; ++X)
            {
                const FVector LocalPosition(
                    X * ChunkVoxelSize - HalfSize,
                    Y * ChunkVoxelSize - HalfSize,
                    Z * ChunkVoxelSize - HalfSize
                );

                const FVector WorldPosition = LocalPosition + FVector(Chunk.Position);
                const int32 Index = X + Y * ChunkResolution + Z * ChunkResolution * ChunkResolution;
                SignedDistanceField[Index] = GetSignedDistance(WorldPosition);
            }
        }
    }

    // Generate the mesh using surface nets
    FBBSurfaceNetsBuffer MeshBuffer;
    UBBSurfaceNetsLibrary::GenerateSurfaceNets(
        SignedDistanceField,
        Dimensions,
        ChunkVoxelSize,
        bEstimateNormals,
        MeshBuffer
    );

    // Create the procedural mesh section
    TArray<FVector> Vertices = MoveTemp(MeshBuffer.Mesh.Positions);
    TArray<int32> Triangles = MoveTemp(MeshBuffer.Mesh.Indices);
    TArray<FVector> Normals = MoveTemp(MeshBuffer.Mesh.Normals);
    TArray<FVector2D> UV0;
    UV0.SetNum(Vertices.Num());

    // Generate UVs (simple spherical mapping)
    for (int32 i = 0; i < Vertices.Num(); ++i)
    {
        const FVector NormalizedPos = Vertices[i].GetSafeNormal();
        UV0[i] = FVector2D(
            FMath::Atan2(NormalizedPos.Y, NormalizedPos.X) / (2.0f * PI) + 0.5f,
            FMath::Acos(NormalizedPos.Z) / PI
        );
    }

    // Create the mesh section
    MeshComponent->CreateMeshSection_LinearColor(
        Chunk.MeshSectionIndex,
        Vertices,
        Triangles,
        Normals,
        UV0,
        TArray<FLinearColor>(),
        TArray<FProcMeshTangent>(),
        true
    );

    // Set material
    if (PlanetMaterial)
    {
        MeshComponent->SetMaterial(Chunk.MeshSectionIndex, PlanetMaterial);
    }
}

void ABBPlanetActor::GeneratePlanet()
{
    // Clear existing chunks
    Chunks.Empty();
    MeshComponent->ClearAllMeshSections();

    // Create initial chunk at LOD 0
    FPlanetChunk RootChunk;
    RootChunk.Position = FIntVector::ZeroValue;
    RootChunk.LOD = 0;
    RootChunk.bIsVisible = true;
    RootChunk.MeshSectionIndex = 0;

    Chunks.Add(RootChunk);
    GenerateChunk(RootChunk);
} 