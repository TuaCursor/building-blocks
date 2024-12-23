#include "SurfaceNets.h"
#include "VoxelChunk.h"

// Helper function to get distance value at a point
static float GetDistance(const TArray<FVoxelData>& VoxelData, const FIntVector& Position, const FIntVector& ChunkSize)
{
    int32 Index = Position.X + Position.Y * ChunkSize.X + Position.Z * ChunkSize.X * ChunkSize.Y;
    if (Index >= 0 && Index < VoxelData.Num())
    {
        return VoxelData[Index].Distance;
    }
    return 0.0f;
}

// Helper function to generate triangles from vertices
static void GenerateTriangles(
    const TArray<FCellVertex>& CellVertices,
    const FIntVector& ChunkSize,
    TArray<int32>& OutIndices)
{
    // For each cell that has a vertex
    for (int32 z = 0; z < ChunkSize.Z - 1; z++)
    {
        for (int32 y = 0; y < ChunkSize.Y - 1; y++)
        {
            for (int32 x = 0; x < ChunkSize.X - 1; x++)
            {
                // Find vertices for current cell
                int32 CellIndex = x + y * (ChunkSize.X - 1) + z * (ChunkSize.X - 1) * (ChunkSize.Y - 1);
                if (CellIndex >= CellVertices.Num()) continue;

                // Generate triangles connecting to neighboring cells
                if (x < ChunkSize.X - 2 && y < ChunkSize.Y - 2 && z < ChunkSize.Z - 2)
                {
                    int32 NextX = CellIndex + 1;
                    int32 NextY = CellIndex + (ChunkSize.X - 1);
                    int32 NextZ = CellIndex + (ChunkSize.X - 1) * (ChunkSize.Y - 1);

                    if (NextX < CellVertices.Num() && NextY < CellVertices.Num() && NextZ < CellVertices.Num())
                    {
                        // Add triangles
                        OutIndices.Add(CellVertices[CellIndex].Index);
                        OutIndices.Add(CellVertices[NextX].Index);
                        OutIndices.Add(CellVertices[NextY].Index);

                        OutIndices.Add(CellVertices[NextY].Index);
                        OutIndices.Add(CellVertices[NextX].Index);
                        OutIndices.Add(CellVertices[NextZ].Index);
                    }
                }
            }
        }
    }
}

void FSurfaceNets::GenerateMesh(
    const TArray<FVoxelData>& VoxelData,
    const FIntVector& ChunkSize,
    TArray<FSurfaceVertex>& OutVertices,
    TArray<int32>& OutIndices)
{
    struct FCellVertex
    {
        FVector Position;
        int32 Index;
    };

    TArray<FCellVertex> CellVertices;
    
    // First pass: Generate vertices for each cell crossing the surface
    for (int32 z = 0; z < ChunkSize.Z - 1; z++)
    {
        for (int32 y = 0; y < ChunkSize.Y - 1; y++)
        {
            for (int32 x = 0; x < ChunkSize.X - 1; x++)
            {
                // Check cell corners
                float Corners[8];
                bool bHasCrossing = false;
                bool bAllPositive = true;
                bool bAllNegative = true;
                
                for (int32 i = 0; i < 8; i++)
                {
                    int32 cx = x + (i & 1);
                    int32 cy = y + ((i >> 1) & 1);
                    int32 cz = z + ((i >> 2) & 1);
                    
                    int32 Index = cx + cy * ChunkSize.X + cz * ChunkSize.X * ChunkSize.Y;
                    Corners[i] = VoxelData[Index].Distance;
                    
                    if (Corners[i] < 0) bAllPositive = false;
                    if (Corners[i] > 0) bAllNegative = false;
                }
                
                if (!bAllPositive && !bAllNegative)
                {
                    // Surface crosses this cell - calculate vertex position
                    FVector VertexPos = CalculateVertexPosition(VoxelData, FIntVector(x,y,z));
                    
                    FCellVertex CellVertex;
                    CellVertex.Position = VertexPos;
                    CellVertex.Index = OutVertices.Num();
                    
                    CellVertices.Add(CellVertex);
                    
                    FSurfaceVertex Vertex;
                    Vertex.Position = VertexPos;
                    Vertex.Normal = CalculateNormal(VoxelData, FIntVector(x,y,z));
                    
                    OutVertices.Add(Vertex);
                }
            }
        }
    }
    
    // Second pass: Generate triangles
    GenerateTriangles(CellVertices, ChunkSize, OutIndices);
}

FVector FSurfaceNets::CalculateVertexPosition(
    const TArray<FVoxelData>& VoxelData,
    const FIntVector& Position)
{
    // Calculate vertex position using weighted average of zero crossings
    FVector Sum = FVector::ZeroVector;
    float WeightSum = 0;
    
    // For each edge of the cell
    static const int32 EdgeTable[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0},  // Bottom face
        {4,5}, {5,6}, {6,7}, {7,4},  // Top face
        {0,4}, {1,5}, {2,6}, {3,7}   // Vertical edges
    };
    
    const FIntVector ChunkSize(32, 32, 32); // Using CHUNK_SIZE constant
    
    for (int32 Edge = 0; Edge < 12; Edge++)
    {
        const int32 V0Index = EdgeTable[Edge][0];
        const int32 V1Index = EdgeTable[Edge][1];
        
        // Get positions and values at edge endpoints
        FVector P0 = FVector(
            Position.X + (V0Index & 1),
            Position.Y + ((V0Index >> 1) & 1),
            Position.Z + ((V0Index >> 2) & 1)
        );
        
        FVector P1 = FVector(
            Position.X + (V1Index & 1),
            Position.Y + ((V1Index >> 1) & 1),
            Position.Z + ((V1Index >> 2) & 1)
        );
        
        float Value0 = GetDistance(VoxelData, FIntVector(P0), ChunkSize);
        float Value1 = GetDistance(VoxelData, FIntVector(P1), ChunkSize);
        
        if (Value0 * Value1 < 0)  // Edge crosses surface
        {
            float T = Value0 / (Value0 - Value1);
            FVector CrossingPoint = FMath::Lerp(P0, P1, T);
            float Weight = FMath::Abs(1.0f / (Value0 - Value1));
            
            Sum += CrossingPoint * Weight;
            WeightSum += Weight;
        }
    }
    
    return WeightSum > 0 ? Sum / WeightSum : FVector(
        Position.X + 0.5f,
        Position.Y + 0.5f,
        Position.Z + 0.5f
    );
}

FVector FSurfaceNets::CalculateNormal(
    const TArray<FVoxelData>& VoxelData,
    const FIntVector& Position)
{
    // Calculate normal using central differences
    const float H = 1.0f;
    const FIntVector ChunkSize(32, 32, 32); // Using CHUNK_SIZE constant
    
    float DX = (GetDistance(VoxelData, Position + FIntVector(1,0,0), ChunkSize) -
                GetDistance(VoxelData, Position - FIntVector(1,0,0), ChunkSize)) / (2*H);
                
    float DY = (GetDistance(VoxelData, Position + FIntVector(0,1,0), ChunkSize) -
                GetDistance(VoxelData, Position - FIntVector(0,1,0), ChunkSize)) / (2*H);
                
    float DZ = (GetDistance(VoxelData, Position + FIntVector(0,0,1), ChunkSize) -
                GetDistance(VoxelData, Position - FIntVector(0,0,1), ChunkSize)) / (2*H);
                
    return FVector(DX, DY, DZ).GetSafeNormal();
} 