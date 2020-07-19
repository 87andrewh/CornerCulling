#pragma once

#include "FastBVH/Vector3.h"
#include "GeometricPrimitives.h"

#include <cstdint>
#include <utility>

namespace FastBVH {

//! \brief Represents an axis-aligned bounding box.
//! This could also be called an AABB.
//! It's used to represent the space occupied
//! by a primitive.
//! \tparam Float The floating point type used
//! by the minimum and maximum point components.
template <typename Float>
struct BBox final {
  //! A simple type definition for a 3D vector.
  using Vec3 = Vector3<Float>;

  //! The minimum point of the bounding box.
  Vec3 min;

  //! The maximum point of the bounding box.
  Vec3 max;

  //! The difference between the max and min
  //! points of the bounding box.
  Vec3 extent;

  //! Constructs an uninitialized bounding box.
  constexpr BBox() noexcept {}

  //! Constructs a bounding box with
  //! a specified minimum and maximum.
  constexpr BBox(const Vec3& min, const Vec3& max) noexcept : min(min), max(max), extent(max - min) {}

  //! Constructs a bounding box around
  //! a single point. The volume occupied
  //! by the box after using this construction
  //! will always be zero.
  constexpr BBox(const Vec3& p) noexcept : BBox(p, p) {}

  //! Expands the volume of the bounding box to fit a new point.
  //! \param p The point to expand the volume for.
  void expandToInclude(const Vec3& p) noexcept {
    min = FastBVH::min(min, p);
    max = FastBVH::max(max, p);
    extent = max - min;
  }

  //! Expands the volume of the bounding box to fit the space of another box.
  //! \param b The box to expand the volume for.
  void expandToInclude(const BBox& b) noexcept {
    min = FastBVH::min(min, b.min);
    max = FastBVH::max(max, b.max);
    extent = max - min;
  }

  //! Gets the center of the bounding box.
  //! \return The center of the bounding box.
  Vec3 getCenter() const noexcept { return (max + min) * Float(0.5); }

  //! Checks for intersection between a ray and the box.
  //! \param ray The ray being traced.
  //! \param tnear The scale to the nearest box hit.
  //! \param tfar The scale to the farthest box hit.
  //! \return True if the ray hits the box, false otherwise.
  bool intersect(const OptSegment& Segment, Float* tnear, Float* tfar) const noexcept;

  //! Determines the index of the dimension
  //! that has the largest space between the
  //! minimum and maximum points of the box.
  //! \return The index of the dimension that
  //! has the most amount of space between the
  //! minimum and maximum box points. A value of
  //! zero indicates the X-axis, a value of one
  //! indicates the Y-axis, and a value of two
  //! indicates the Z-axis.
  uint32_t maxDimension() const noexcept;

  //! Calculates the surface area of the box.
  //! \return The surface area of the bounding box.
  constexpr Float surfaceArea() const noexcept {
    return Float(2) * ((extent.x * extent.z) + (extent.x * extent.y) + (extent.y * extent.z));
  }
};


// Checks if the AABB intersects the line segment
// between Start and End. Uses Slab method.
// Code adapted from:
// https://tavianator.com/cgit/dimension.git/tree/libdimension/bvh/bvh.c#n196
template <typename Float>
bool BBox<Float>::intersect(
    const OptSegment& Segment,
    Float* tnear,
    Float* tfar) const noexcept 
{
    Float tx1 = (min.x - Segment.Start.X) * Segment.Reciprocal.X;
    Float tx2 = (max.x - Segment.Start.X) * Segment.Reciprocal.X;
    Float tmin = std::min(tx1, tx2);
    Float tmax = std::max(tx1, tx2);
    if (tmin > tmax || tmax < 0 || tmin > 1)
        return false;

    Float ty1 = (min.y - Segment.Start.Y) * Segment.Reciprocal.Y;
    Float ty2 = (max.y - Segment.Start.Y) * Segment.Reciprocal.Y;
    tmin = std::max(tmin, std::min(ty1, ty2));
    tmax = std::min(tmax, std::max(ty1, ty2));
    if (tmin > tmax || tmax < 0 || tmin > 1)
        return false;

    Float tz1 = (min.z - Segment.Start.Z) * Segment.Reciprocal.Z;
    Float tz2 = (max.z - Segment.Start.Z) * Segment.Reciprocal.Z;
    tmin = std::max(tmin, std::min(tz1, tz2));
    tmax = std::min(tmax, std::max(tz1, tz2));
    if (tmin > tmax || tmax < 0 || tmin > 1)
        return false;

    *tnear = tmin;
    *tfar = tmax;
    return true;
}

template <typename Float>
uint32_t BBox<Float>::maxDimension() const noexcept {
  // Assume X axis is longest first
  uint32_t result = 0;
  if (extent[1] > extent[result])
      result = 1;
  if (extent[2] > extent[result])
      result = 2;
  return result;
}
}  // namespace FastBVH
