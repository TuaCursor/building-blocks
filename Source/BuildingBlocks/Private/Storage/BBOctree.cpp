#include "Storage/BBOctree.h"

void UBBOctreeLibrary::BuildOctree(
    FOctreeNode& Root,
    const FBBSphere& ViewSphere,
    int32 MaxLOD,
    float LODDistanceMultiplier
)
{
    SubdivideNode(Root, ViewSphere, MaxLOD, LODDistanceMultiplier);
}

void UBBOctreeLibrary::GetVisibleChunks(
    const FOctreeNode& Root,
    const FBBSphere& ViewSphere,
    TArray<FOctreeNode>& OutChunks
)
{
    if (!IsNodeVisible(Root, ViewSphere))
    {
        return;
    }

    if (Root.bIsLeaf)
    {
        OutChunks.Add(Root);
        return;
    }

    for (const FOctreeNode& Child : Root.Children)
    {
        GetVisibleChunks(Child, ViewSphere, OutChunks);
    }
}

void UBBOctreeLibrary::SubdivideNode(
    FOctreeNode& Node,
    const FBBSphere& ViewSphere,
    int32 MaxLOD,
    float LODDistanceMultiplier
)
{
    if (!ShouldSubdivide(Node, ViewSphere, MaxLOD, LODDistanceMultiplier))
    {
        return;
    }

    Node.bIsLeaf = false;
    const int32 ChildSize = Node.Size / 2;
    const int32 NextLOD = Node.LOD + 1;

    // Create 8 children in octree pattern
    for (int32 Z = 0; Z < 2; ++Z)
    {
        for (int32 Y = 0; Y < 2; ++Y)
        {
            for (int32 X = 0; X < 2; ++X)
            {
                FIntVector ChildPos = Node.Position + FIntVector(X, Y, Z) * ChildSize;
                FOctreeNode Child(ChildPos, ChildSize, NextLOD);
                
                Node.Children.Add(Child);
                SubdivideNode(Node.Children.Last(), ViewSphere, MaxLOD, LODDistanceMultiplier);
            }
        }
    }
}

bool UBBOctreeLibrary::ShouldSubdivide(
    const FOctreeNode& Node,
    const FBBSphere& ViewSphere,
    int32 MaxLOD,
    float LODDistanceMultiplier
)
{
    if (Node.LOD >= MaxLOD)
    {
        return false;
    }

    // Calculate distance from view sphere center to node center
    FVector NodeCenter = FVector(Node.Position) + FVector(Node.Size / 2);
    float Distance = FVector::Distance(NodeCenter, FVector(ViewSphere.Center));

    // Calculate minimum distance for this LOD level
    float MinDistance = Node.Size * LODDistanceMultiplier;
    return Distance < MinDistance;
}

bool UBBOctreeLibrary::IsNodeVisible(
    const FOctreeNode& Node,
    const FBBSphere& ViewSphere
)
{
    // Create a sphere that encompasses the node
    FVector NodeCenter = FVector(Node.Position) + FVector(Node.Size / 2);
    float NodeRadius = FVector(Node.Size).Size() * 0.5f;
    FBBSphere NodeSphere(NodeCenter, NodeRadius);

    return ViewSphere.Intersects(NodeSphere);
} 