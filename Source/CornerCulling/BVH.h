/**
    @author Andrew Huang (87andrewh)
*/

#pragma once

#include "GeometricPrimitives.h"

/**
 *  Bounding Volume Hierarchy node. Used to accelerate ray trace intersection.
 */
class BVH
{
    // Indices of the cuboids in this BVH node.
    TArray<int> CuboidIs;
    // AABB of the BVH node.
    AABB Box;

public:
	BVH();
    // Constructs Bounding Volume Hierarchy from list of Cuboids.
    // List must be the same as the one used in CullingController
    // so that indices line up.
	BVH(const TArray<Cuboid>& Cuboids);
	// Gets indices of cuboids that could intersect the line segment
    // between Start and End.
	TArray<int> GetIntersectingCuboids(FVector Start, FVector End);
    // Checks if the node is a leaf.
    bool IsLeaf();
};
