#include "BVH.h"

BVH::BVH() {}

// Construct Bounding Volume Hierarchy with top-down recursive splits,
// estimating best splits with surface area heuristic.
BVH::BVH(const TArray<Cuboid>& Cuboids)
{
}

// Recursively search through children of the BVH node,
// returning the list of the indices of cuboids in leaf nodes
// that intersect the line segment between Start and End.
TArray<int> BVH::GetIntersectingCuboids(FVector Start, FVector End)
{
    OptSegment Segment = OptSegment(Start, End);
    return TArray<int>();
}

bool BVH::IsLeaf()
{
    return CuboidIs.Num() > 0;
}
