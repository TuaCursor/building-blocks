#include "VoxelChunk.h"
#include "ProceduralMeshComponent.h"
#include "SurfaceNets.h"

FVoxelChunk::FVoxelChunk(const FIntVector& InPosition, int32 InLOD)
    : Position(InPosition)
    , LOD(InLOD)
    , bIsGenerated(false)
{
    // Initialize chunk data array
    Data.SetNumZeroed(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);
}

void FVoxelChunk::GenerateData()
{
    // Similar to the Rust noise generation in lod_terrain example
    for (int32 x = 0; x < CHUNK_SIZE; x++)
    {
        for (int32 y = 0; y < CHUNK_SIZE; y++)
        {
            for (int32 z = 0; z < CHUNK_SIZE; z++)
            {
                FVector WorldPos = FVector(
                    (Position.X + x) * (1 << LOD),
                    (Position.Y + y) * (1 << LOD),
                    (Position.Z + z) * (1 << LOD)
                );

                // Generate SDF value using noise
                float NoiseValue = GenerateNoise(WorldPos);
                
                int32 Index = x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE;
                Data[Index].Distance = NoiseValue;
                Data[Index].bIsEmpty = NoiseValue > 0;
            }
        }
    }

    bIsGenerated = true;
}

float FVoxelChunk::GenerateNoise(const FVector& WorldPos) const
{
    const float Frequency = 0.02f;
    const float Scale = 1.0f;
    
    // Basic sphere SDF for testing
    float SphereDist = (WorldPos - FVector::ZeroVector).Size() - 500.0f;
    
    // Add some noise variation
    float NoiseValue = FMath::PerlinNoise3D(WorldPos * Frequency) * Scale;
    
    return SphereDist + NoiseValue;
}

FVoxelData FVoxelChunk::GetVoxel(const FIntVector& LocalPos) const
{
    int32 Index = LocalPos.X + LocalPos.Y * CHUNK_SIZE + LocalPos.Z * CHUNK_SIZE * CHUNK_SIZE;
    return Data[Index];
}

void FVoxelChunk::CreateMesh(UProceduralMeshComponent* MeshComponent)
{
    TArray<FSurfaceVertex> Vertices;
    TArray<int32> Indices;
    
    // Generate mesh using surface nets
    FSurfaceNets::GenerateMesh(Data, FIntVector(CHUNK_SIZE), Vertices, Indices);
    
    // Convert to format needed by UProceduralMeshComponent
    TArray<FVector> VertexPositions;
    TArray<FVector> VertexNormals;
    TArray<FVector2D> UV0;
    TArray<FProcMeshTangent> Tangents;
    
    for (const FSurfaceVertex& Vertex : Vertices)
    {
        // Scale positions by LOD and offset by chunk position
        FVector WorldPos = Vertex.Position * (1 << LOD) + 
            FVector(Position) * CHUNK_SIZE * (1 << LOD);
            
        VertexPositions.Add(WorldPos);
        VertexNormals.Add(Vertex.Normal);
        UV0.Add(FVector2D::ZeroVector);
        Tangents.Add(FProcMeshTangent(1, 0, 0));
    }

    MeshComponent->CreateMeshSection_LinearColor(
        0, // Section index
        VertexPositions,
        Indices,
        VertexNormals,
        UV0,
        TArray<FLinearColor>(), // Vertex colors
        Tangents,
        true // Create collision
    );
} 