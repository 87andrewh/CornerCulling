/**
    @author Andrew Huang (87andrewh)
*/

#pragma once

#include "CoreMinimal.h"
#include "GeometricPrimitives.h"

/**
 *  Bounding Volume Hierarchy to accelerate ray trace intersection tests.
 */
class BVH
{
    // Indices of the cuboids in this BVH node.
    TArray<int> CuboidIs;
public:
	BVH();
};
