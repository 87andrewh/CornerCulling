#pragma once

#include "FastBVH/BVH.h"
#include "GeometricPrimitives.h"
#include <vector>

namespace FastBVH {

    //! \brief Used for traversing a BVH and checking for ray-primitive intersections.
    //! \tparam Float The floating point type used by vector components.
    //! \tparam Intersector The type of the primitive intersector.
    template <
        typename Float,
        typename Intersector>
    class Traverser final
    {
        const BVH<Float, Cuboid>& bvh;
        Intersector intersector;

    public:
        //! Constructs a new BVH traverser.
        //! \param bvh_ The BVH to be traversed.
        constexpr Traverser(const BVH<Float, Cuboid>& bvh_, const Intersector& intersector_) noexcept
            : bvh(bvh_), intersector(intersector_) {}
        // Traces single ray through the BVH, returning true if that ray
        // intersects a cuboid that blocks LOS between peeks and the verticies
        // of an enemy bounding box.
        const Cuboid* traverse(
            const OptSegment& segment,
            const std::vector<FVector>& peeks,
            const CharacterBounds& Bounds);
    };

    //! \brief Contains implementation details for the @ref Traverser class.
    namespace TraverserImpl {

        //! \brief Node for storing state information during traversal.
        template <typename Float>
        struct Traversal final
        {
            //! The index of the node to be traversed.
            uint32_t i;

            //! Minimum hit time for this node.
            Float mint;

            //! Constructs an uninitialized instance of a traversal context.
            constexpr Traversal() noexcept {}

            //! Constructs an initialized traversal context.
            //! \param i_ The index of the node to be traversed.
            constexpr Traversal(int i_, Float mint_) noexcept : i(i_), mint(mint_) {}
        };

    }  // namespace TraverserImpl

    template <
        typename Float,
        typename Intersector
    >
    const Cuboid*
    Traverser<Float, Intersector>::traverse(
        const OptSegment& segment,
        const std::vector<FVector>& peeks,
        const CharacterBounds& bounds)
    {
    using Traversal = TraverserImpl::Traversal<Float>;

    // Bounding box min-t/max-t for left/right children at some point in the tree
    Float bbhits[4];
    int32_t closer, other;

    // Working set
    // WARNING : The working set size is relatively small here, should be made dynamic or template-configurable
    Traversal todo[64];
    int32_t stackptr = 0;

    // "Push" on the root node to the working set
    todo[stackptr].i = 0;
    todo[stackptr].mint = -9999999.f;

    const auto nodes = bvh.getNodes();

    auto build_prims = bvh.getPrimitives();

    while (stackptr >= 0)
    {
        // Pop off the next node to work on.
        int ni = todo[stackptr].i;
        Float near = todo[stackptr].mint;
        stackptr--;
        const auto& node(nodes[ni]);

        // Is leaf -> Intersect
        if (node.isLeaf())
        {
            for (uint32_t o = 0; o < node.primitive_count; ++o)
            {
                const auto& obj = build_prims[node.start + o];
                Intersection<float> current = intersector(*obj, segment);
                if (current)
                {
                    if (
                        IsBlocking(
                            peeks,
                            bounds,
                            current.IntersectedP))
                    {
                        return current.IntersectedP;
                    }
                }
            }
        }
        else
        {  // Not a leaf

            bool hitc0 = nodes[ni + 1].bbox.intersect(segment, bbhits, bbhits + 1);
            bool hitc1 = nodes[ni + node.right_offset].bbox.intersect(segment, bbhits + 2, bbhits + 3);

            // Did we hit both nodes?
            if (hitc0 && hitc1)
            {
                // We assume that the left child is a closer hit...
                closer = ni + 1;
                other = ni + node.right_offset;

                // ... If the right child was actually closer, swap the relevant values.
                if (bbhits[2] < bbhits[0])
                {
                    std::swap(bbhits[0], bbhits[2]);
                    std::swap(bbhits[1], bbhits[3]);
                    std::swap(closer, other);
                }

                // It's possible that the nearest object is still in the other side, but
                // we'll check the further-away node later...

                // Push the farther first
                todo[++stackptr] = Traversal(other, bbhits[2]);

                // And now the closer (with overlap test)
                todo[++stackptr] = Traversal(closer, bbhits[0]);
            }

            else if (hitc0)
            {
                todo[++stackptr] = Traversal(ni + 1, bbhits[0]);
            }

            else if (hitc1)
            {
                todo[++stackptr] = Traversal(ni + node.right_offset, bbhits[2]);
            }
        }
    }
    return NULL;
    }
}  // namespace FastBVH
