#pragma once

#include "FastBVH/Vector3.h"
#include "GeometricPrimitives.h"

#include <limits>

namespace FastBVH {

//! \brief Stores information regarding a ray intersection with a primitive.
//! \tparam Float The floating point type used for vector components.
template <typename Float>
struct Intersection final {
  /// A simple type definition for 3D vector.
  using Vec3 = Vector3<Float>;

  //! The scale at which the ray reaches the primitive.
  Float t = std::numeric_limits<Float>::infinity();

  // Pointer to the intersected object.
  const Cuboid* IntersectedP = NULL;

  //! Gets the position at the ray hit the object.
  //! \param ray_pos The ray position.
  //! \param ray_dir The ray direction.
  //! \return The position at which the intersection occurred at.
  Vec3 getHitPosition(const Vec3& ray_pos, const Vec3& ray_dir) const noexcept { return ray_pos + (ray_dir * t); }

  //! Indicates whether or not the intersection is valid.
  //! \return True if the intersection is valid, false otherwise.
  operator bool() const noexcept { return t != std::numeric_limits<Float>::infinity(); }
};

//! \brief Gets the closest of two intersections.
//! \returns A copy of either @p a or @p b, depending on which one is closer.
template <typename Float, typename Primitive>
Intersection<Float> closest(
    const Intersection<Float>& a,
    const Intersection<Float>& b) noexcept
{
  return (a.t < b.t) ? a : b;
}

}  // namespace FastBVH
