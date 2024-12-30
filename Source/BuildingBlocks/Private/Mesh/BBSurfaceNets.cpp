#include "Mesh/BBSurfaceNets.h"

void FBBPosNormMesh::Append(const FBBPosNormMesh& Other)
{
    const int32 BaseIndex = Positions.Num();
    Positions.Append(Other.Positions);
    Normals.Append(Other.Normals);
    
    for (int32 Index : Other.Indices)
    {
        Indices.Add(BaseIndex + Index);
    }
}

void FBBSurfaceNetsBuffer::Reset(int32 ArraySize)
{
    Mesh.Clear();
    SurfacePoints.Empty();
    SurfaceStrides.Empty();
    StrideToIndex.SetNum(ArraySize);
}

void UBBSurfaceNetsLibrary::GetPaddedSurfaceNetsChunkExtent(const FBBExtent& ChunkExtent, FBBExtent& OutExtent)
{
    OutExtent = FBBExtent(
        ChunkExtent.Min - FIntVector(1),
        ChunkExtent.Max + FIntVector(1)
    );
}

namespace
{
    const FIntVector CubeCornerOffsets[8] = {
        FIntVector(0, 0, 0), FIntVector(1, 0, 0),
        FIntVector(0, 1, 0), FIntVector(1, 1, 0),
        FIntVector(0, 0, 1), FIntVector(1, 0, 1),
        FIntVector(0, 1, 1), FIntVector(1, 1, 1)
    };

    struct FCubeEdge
    {
        int32 Corner1;
        int32 Corner2;
    };

    const FCubeEdge CubeEdges[] = {
        {0, 1}, {2, 3}, {4, 5}, {6, 7}, // X edges
        {0, 2}, {1, 3}, {4, 6}, {5, 7}, // Y edges
        {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Z edges
    };

    FVector EstimateSurfaceEdgeIntersection(int32 Corner1, int32 Corner2, float Value1, float Value2)
    {
        const float Interp1 = Value1 / (Value1 - Value2);
        const float Interp2 = 1.0f - Interp1;

        return FVector(
            (Corner1 & 1) * Interp2 + (Corner2 & 1) * Interp1,
            ((Corner1 >> 1) & 1) * Interp2 + ((Corner2 >> 1) & 1) * Interp1,
            ((Corner1 >> 2) & 1) * Interp2 + ((Corner2 >> 2) & 1) * Interp1
        );
    }

    FVector CalculateGradient(const float* Dists, const FVector& S)
    {
        const float Nx = 1.0f - S.X;
        const float Ny = 1.0f - S.Y;
        const float Nz = 1.0f - S.Z;

        const float DxZ0 = Ny * (Dists[1] - Dists[0]) + S.Y * (Dists[3] - Dists[2]);
        const float DxZ1 = Ny * (Dists[5] - Dists[4]) + S.Y * (Dists[7] - Dists[6]);
        const float Dx = Nz * DxZ0 + S.Z * DxZ1;

        const float DyX0 = Nz * (Dists[2] - Dists[0]) + S.Z * (Dists[6] - Dists[4]);
        const float DyX1 = Nz * (Dists[3] - Dists[1]) + S.Z * (Dists[7] - Dists[5]);
        const float Dy = Nx * DyX0 + S.X * DyX1;

        const float DzY0 = Nx * (Dists[4] - Dists[0]) + S.X * (Dists[5] - Dists[1]);
        const float DzY1 = Nx * (Dists[6] - Dists[2]) + S.X * (Dists[7] - Dists[3]);
        const float Dz = Ny * DzY0 + S.Y * DzY1;

        return FVector(Dx, Dy, Dz);
    }

    bool EstimateSurfaceInCube(
        const TArray<float>& SDF,
        const FIntVector& Dimensions,
        float VoxelSize,
        const FIntVector& CubeMinCorner,
        const TArray<int32>& CornerIndices,
        bool bEstimateNormals,
        FBBSurfaceNetsBuffer& Output
    )
    {
        float CornerDists[8];
        int32 NumNegative = 0;

        for (int32 i = 0; i < 8; ++i)
        {
            const float Dist = SDF[CornerIndices[i]];
            CornerDists[i] = Dist;
            if (Dist < 0.0f)
            {
                ++NumNegative;
            }
        }

        if (NumNegative == 0 || NumNegative == 8)
        {
            return false;
        }

        FVector Centroid = FVector::ZeroVector;
        int32 Count = 0;

        for (const FCubeEdge& Edge : CubeEdges)
        {
            const float D1 = CornerDists[Edge.Corner1];
            const float D2 = CornerDists[Edge.Corner2];
            
            if ((D1 < 0.0f) != (D2 < 0.0f))
            {
                ++Count;
                Centroid += EstimateSurfaceEdgeIntersection(Edge.Corner1, Edge.Corner2, D1, D2);
            }
        }

        Centroid /= Count;
        const FVector Position = VoxelSize * (FVector(CubeMinCorner) + Centroid + FVector(0.5f));

        Output.Mesh.Positions.Add(Position);
        if (bEstimateNormals)
        {
            Output.Mesh.Normals.Add(CalculateGradient(CornerDists, Centroid));
        }

        return true;
    }

    void MakeQuad(
        const TArray<float>& SDF,
        const TArray<int32>& StrideToIndex,
        const TArray<FVector>& Positions,
        int32 P1Stride,
        int32 P2Stride,
        int32 AxisBStride,
        int32 AxisCStride,
        TArray<int32>& OutIndices
    )
    {
        const float D1 = SDF[P1Stride];
        const float D2 = SDF[P2Stride];
        const bool bNegativeFace = D1 > 0.0f && D2 < 0.0f;
        if (D1 * D2 >= 0.0f)
        {
            return;
        }

        const int32 V1 = StrideToIndex[P1Stride];
        const int32 V2 = StrideToIndex[P1Stride - AxisBStride];
        const int32 V3 = StrideToIndex[P1Stride - AxisCStride];
        const int32 V4 = StrideToIndex[P1Stride - AxisBStride - AxisCStride];

        const FVector& Pos1 = Positions[V1];
        const FVector& Pos2 = Positions[V2];
        const FVector& Pos3 = Positions[V3];
        const FVector& Pos4 = Positions[V4];

        const bool bSplitShort = FVector::DistSquared(Pos1, Pos4) < FVector::DistSquared(Pos2, Pos3);

        if (bSplitShort)
        {
            if (bNegativeFace)
            {
                OutIndices.Append({ V1, V4, V2, V1, V3, V4 });
            }
            else
            {
                OutIndices.Append({ V1, V2, V4, V1, V4, V3 });
            }
        }
        else
        {
            if (bNegativeFace)
            {
                OutIndices.Append({ V2, V3, V4, V2, V1, V3 });
            }
            else
            {
                OutIndices.Append({ V2, V4, V3, V2, V3, V1 });
            }
        }
    }
}

void UBBSurfaceNetsLibrary::GenerateSurfaceNets(
    const TArray<float>& SignedDistanceField,
    const FIntVector& Dimensions,
    float VoxelSize,
    bool bEstimateNormals,
    FBBSurfaceNetsBuffer& OutBuffer
)
{
    const int32 TotalSize = Dimensions.X * Dimensions.Y * Dimensions.Z;
    OutBuffer.Reset(TotalSize);

    // First pass: find surface points and estimate positions/normals
    for (int32 Z = 0; Z < Dimensions.Z - 1; ++Z)
    {
        for (int32 Y = 0; Y < Dimensions.Y - 1; ++Y)
        {
            for (int32 X = 0; X < Dimensions.X - 1; ++X)
            {
                const FIntVector CubeMin(X, Y, Z);
                TArray<int32> CornerIndices;
                CornerIndices.SetNum(8);

                for (int32 i = 0; i < 8; ++i)
                {
                    const FIntVector Corner = CubeMin + CubeCornerOffsets[i];
                    CornerIndices[i] = Corner.X + Corner.Y * Dimensions.X + Corner.Z * Dimensions.X * Dimensions.Y;
                }

                const int32 CubeStride = X + Y * Dimensions.X + Z * Dimensions.X * Dimensions.Y;
                if (EstimateSurfaceInCube(SignedDistanceField, Dimensions, VoxelSize, CubeMin, CornerIndices, bEstimateNormals, OutBuffer))
                {
                    OutBuffer.StrideToIndex[CubeStride] = OutBuffer.Mesh.Positions.Num() - 1;
                    OutBuffer.SurfacePoints.Add(CubeMin);
                    OutBuffer.SurfaceStrides.Add(CubeStride);
                }
            }
        }
    }

    // Second pass: generate quads
    const int32 StrideX = 1;
    const int32 StrideY = Dimensions.X;
    const int32 StrideZ = Dimensions.X * Dimensions.Y;

    for (int32 i = 0; i < OutBuffer.SurfacePoints.Num(); ++i)
    {
        const FIntVector& P = OutBuffer.SurfacePoints[i];
        const int32 PStride = OutBuffer.SurfaceStrides[i];

        // Skip edges on max boundaries to avoid duplicates
        if (P.X < Dimensions.X - 2 && P.Y > 0 && P.Z > 0)
        {
            MakeQuad(SignedDistanceField, OutBuffer.StrideToIndex, OutBuffer.Mesh.Positions,
                PStride, PStride + StrideX, StrideY, StrideZ, OutBuffer.Mesh.Indices);
        }

        if (P.Y < Dimensions.Y - 2 && P.X > 0 && P.Z > 0)
        {
            MakeQuad(SignedDistanceField, OutBuffer.StrideToIndex, OutBuffer.Mesh.Positions,
                PStride, PStride + StrideY, StrideZ, StrideX, OutBuffer.Mesh.Indices);
        }

        if (P.Z < Dimensions.Z - 2 && P.X > 0 && P.Y > 0)
        {
            MakeQuad(SignedDistanceField, OutBuffer.StrideToIndex, OutBuffer.Mesh.Positions,
                PStride, PStride + StrideZ, StrideX, StrideY, OutBuffer.Mesh.Indices);
        }
    }
} 