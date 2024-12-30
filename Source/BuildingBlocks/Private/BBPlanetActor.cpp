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
    
    // Ensure we have a valid initial state
    if (MeshComponent)
    {
        MeshComponent->ClearAllMeshSections();
        Chunks.Empty();
    }
    
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
    
    // Base sphere SDF (negative inside, positive outside)
    float Distance = DistanceFromCenter - Radius;

    // Add noise
    if (NoiseAmplitude > 0.0f && DistanceFromCenter > SMALL_NUMBER)
    {
        const FVector NormalizedPos = Position / DistanceFromCenter;
        const float NoiseValue = SimpleNoise3D(
            NormalizedPos.X * NoiseScale,
            NormalizedPos.Y * NoiseScale,
            NormalizedPos.Z * NoiseScale
        );
        
        // Scale noise by distance to maintain spherical shape at distance
        const float NoiseScale = FMath::Min(1.0f, Radius / DistanceFromCenter);
        Distance += NoiseValue * NoiseAmplitude * NoiseScale;
    }

    return Distance;
}

void ABBPlanetActor::UpdateLOD()
{
    // Get camera position
    APlayerCameraManager* CameraManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
    if (!CameraManager || !MeshComponent)
    {
        return;
    }

    FVector CameraLocation = CameraManager->GetCameraLocation();
    
    // Create view sphere for LOD calculation
    const float ViewRadius = ChunkSize * LODDistanceMultiplier * FMath::Pow(2.0f, MaxLODLevels);
    FBBSphere ViewSphere(CameraLocation, ViewRadius);

    // Create root node for octree
    const int32 RootSize = FMath::RoundUpToPowerOfTwo(Resolution) * VoxelSize;
    FOctreeNode RootNode(FIntVector::ZeroValue, RootSize, 0);

    // Build octree
    UBBOctreeLibrary::BuildOctree(RootNode, ViewSphere, MaxLODLevels, LODDistanceMultiplier);

    // Get visible chunks
    TArray<FOctreeNode> VisibleNodes;
    UBBOctreeLibrary::GetVisibleChunks(RootNode, ViewSphere, VisibleNodes);

    // Track which chunks are still needed
    TSet<int32> ActiveChunks;

    // Update chunks
    for (const FOctreeNode& Node : VisibleNodes)
    {
        FPlanetChunk NewChunk;
        NewChunk.Position = Node.Position / VoxelSize; // Convert to grid coordinates
        NewChunk.LOD = Node.LOD;
        NewChunk.bIsVisible = true;

        bool bChunkExists = false;
        for (int32 i = 0; i < Chunks.Num(); ++i)
        {
            FPlanetChunk& ExistingChunk = Chunks[i];
            if (ExistingChunk.Position == NewChunk.Position && ExistingChunk.LOD == NewChunk.LOD)
            {
                ExistingChunk.bIsVisible = true;
                ActiveChunks.Add(i);
                bChunkExists = true;
                break;
            }
        }

        if (!bChunkExists)
        {
            NewChunk.MeshSectionIndex = Chunks.Num();
            ActiveChunks.Add(NewChunk.MeshSectionIndex);
            Chunks.Add(NewChunk);
            GenerateChunk(NewChunk);
        }
    }

    // Hide or remove chunks that are no longer visible
    for (int32 i = 0; i < Chunks.Num(); ++i)
    {
        if (!ActiveChunks.Contains(i))
        {
            MeshComponent->SetMeshSectionVisible(i, false);
        }
        else
        {
            MeshComponent->SetMeshSectionVisible(i, true);
        }
    }
}

void ABBPlanetActor::GenerateChunk(const FPlanetChunk& Chunk)
{
    const int32 ChunkResolution = Resolution >> Chunk.LOD;
    const float ChunkVoxelSize = VoxelSize * (1 << Chunk.LOD);
    
    // Create the signed distance field for this chunk
    const FIntVector Dimensions(ChunkResolution + 1, ChunkResolution + 1, ChunkResolution + 1);
    const float HalfSize = (ChunkResolution * ChunkVoxelSize) * 0.5f;
    TArray<float> SignedDistanceField;
    SignedDistanceField.SetNum(Dimensions.X * Dimensions.Y * Dimensions.Z);

    // Calculate chunk world position
    const FVector ChunkWorldPos = FVector(Chunk.Position) * ChunkVoxelSize;

    // Fill the SDF
    for (int32 Z = 0; Z < Dimensions.Z; ++Z)
    {
        for (int32 Y = 0; Y < Dimensions.Y; ++Y)
        {
            for (int32 X = 0; X < Dimensions.X; ++X)
            {
                const FVector LocalPosition(
                    X * ChunkVoxelSize - HalfSize,
                    Y * ChunkVoxelSize - HalfSize,
                    Z * ChunkVoxelSize - HalfSize
                );

                const FVector WorldPosition = LocalPosition + ChunkWorldPos;
                const int32 Index = X + Y * Dimensions.X + Z * Dimensions.X * Dimensions.Y;
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

    // Skip if no vertices were generated
    if (MeshBuffer.Mesh.Positions.Num() == 0)
    {
        return;
    }

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