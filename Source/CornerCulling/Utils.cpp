#include "Utils.h"
#include <cmath>

namespace Utils {
	// Get the angle between two FVectors of the form (X, Y, 0)
	// Returns angles in the full range (-PI, PI)
	/**
	inline float GetAngle(FVector V1, FVector V2) {
		float Dot = FVector::DotProduct(V1, V2);
		float Det = FVector::CrossProduct(V1, V2).Z;
		return atan2f(Det, Dot);
	}
	*/
}
