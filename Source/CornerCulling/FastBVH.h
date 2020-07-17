#pragma once
#include "FastBVH/BBox.h"
#include "FastBVH/BVH.h"
#include "FastBVH/BuildStrategy.h"
#include "FastBVH/BuildStrategy1.h"
#include "FastBVH/Config.h"
#include "FastBVH/Intersection.h"
#include "FastBVH/Iterable.h"
#include "FastBVH/Ray.h"
#include "FastBVH/Traverser.h"
#include "FastBVH/Vector3.h"
#include "GeometricPrimitives.h"

// Cuboid BVH API.
namespace
{
    using std::vector;
    using namespace FastBVH;

    // Used to calculate the axis-aligned bounding boxes of cuboids.
    class CuboidBoxConverter final
    {
        public:
            BBox<float> operator()(const Cuboid& C) const noexcept
            {
                float MinX = std::numeric_limits<float>::infinity();
                float MinY = std::numeric_limits<float>::infinity();
                float MinZ = std::numeric_limits<float>::infinity();
                float MaxX = - std::numeric_limits<float>::infinity();
                float MaxY = - std::numeric_limits<float>::infinity();
                float MaxZ = - std::numeric_limits<float>::infinity();
                for (int i = 0; i < CUBOID_V; i++)
                {
                    MinX = std::min(MinX, C.Vertices[i].X);
                    MinY = std::min(MinY, C.Vertices[i].Y);
                    MinZ = std::min(MinZ, C.Vertices[i].Z);
                    MaxX = std::max(MaxX, C.Vertices[i].X);
                    MaxY = std::max(MaxY, C.Vertices[i].Y);
                    MaxZ = std::max(MaxZ, C.Vertices[i].Z);
                }
                auto MinVector = Vector3<float>{MinX, MinY, MinZ};
                auto MaxVector = Vector3<float>{MaxX, MaxY, MaxZ};
                return BBox<float>(MinVector, MaxVector);
            }
    };
    
    // Used to calculate the intersection between rays and cuboids.
    class CuboidIntersector final 
    {
        public:
            Intersection<float, Cuboid> operator()(
                const Cuboid& C, const OptSegment& Segment) const noexcept
            {
                float Time = IntersectionTime(C, Segment.Start, Segment.Delta);
                if (Time > 0)
                {
                    return Intersection<float, Cuboid> { Time, &C };
                }
                else
                {
                    return Intersection<float, Cuboid> {};
                }
            }
    };
}
