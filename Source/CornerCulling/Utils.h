#include "Math/Vector.h"

namespace Utils {
	// Smallest float that is safe to divide by.
	const float MIN_SAFE_LENGTH = 1e-9;
	// Maximum and minum of fast arctangent RELU approxmiation
	const float FAST_ATAN_MAX = 1 / MIN_SAFE_LENGTH;
	const float FAST_ATAN_MIN = -1 / MIN_SAFE_LENGTH;

	// Get the yaw angle between two FVectors.
	// Returns angles in the full range (-PI, PI)
	inline static float GetAngle(const FVector2D& V1, const FVector2D& V2) {
		return atan2f(V1.X * V2.Y - V1.Y * V2.X,
				  	  V1.X * V2.X + V1.Y * V2.Y);
	}

	// Fast RELU approxmiation of arctangent. Not to scale.
	inline static float FastAtan(float X) {
		if (X <= FAST_ATAN_MIN) {
			return FAST_ATAN_MIN;
		}
		else if (X <= FAST_ATAN_MAX) {
			return X;
		}
		else {
			return FAST_ATAN_MAX;
		}
	}

	// Approximate the yaw angle between two FVectors. Angle order remains the same.
	inline static float GetAngleFast(const FVector2D& V1, const FVector2D& V2) {
		// Get the determinant (scaled sine) and dot product (scaled cosine)
		float det = V1.X * V2.Y - V1.Y * V2.X;
		float dot = V1.X * V2.X + V1.Y * V2.Y;
		// For numerical stability, immediately resolve near-right angles.
		// NOTE: This block also catches 0 == det == dot
		if (-MIN_SAFE_LENGTH < dot && dot < MIN_SAFE_LENGTH) {
			// Return right angle with same sign as the determinant
			return (1 - (2 * signbit(det))) * FAST_ATAN_MAX;
		}
		float tan = det / dot;
		if (dot > 0) {
			return FastAtan(tan);
		}
		else {
			if (det > 0) {
				return FastAtan(tan) + FAST_ATAN_MAX;
			}
			else {
				return FastAtan(tan) - FAST_ATAN_MAX;
			}
		}
	}
	
	// Check if two line segments defined by (P1, P1 + V1) and (P2, P2 + V2) intersect.
	// Derivation from Gareth Rees at: 
	// stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect
	inline static bool CheckSegmentsIntersect(const FVector2D& P1, const FVector2D& V1,
											  const FVector2D& P2, const FVector2D& V2) {
		float V1CrossV2 = FVector2D::CrossProduct(V1, V2);
		FVector2D P1ToP2 = P2 - P1;
		// Segments are parallel.
		// Note: A little sketchy, as Cross also depends on the length of both vectors.
		if (abs(V1CrossV2) <= Utils::MIN_SAFE_LENGTH) {
			// Collinear is not considered intersecting.
			return false;
		}
		float T1 = FVector2D::CrossProduct(P1ToP2, V2) / V1CrossV2;
		float T2 = FVector2D::CrossProduct(P1ToP2, V1) / V1CrossV2;
		if ((0.f <= T1) && (T1 <= 1.f) && (0.f <= T2) && (T2 <= 1.f)) {
			return true;
		}
		else {
			return false;
		}
	}
}
