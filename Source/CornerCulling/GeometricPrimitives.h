#pragma once

#include "Containers/Array.h"
#include "Math/Vector.h"
#include <algorithm>
#include <vector>

// Number of vertices and faces of a cuboid.
constexpr char CUBOID_V = 8;
constexpr char CUBOID_F = 6;
// Number of vertices in a face of a cuboid.
constexpr char CUBOID_FACE_V = 4;

// Maps a Face with index i's j-th vertex onto a Cuboid vertex index.
constexpr char FaceCuboidMap[6][4] =
{
    0, 1, 2, 3,
    2, 6, 7, 3,
    0, 3, 7, 4,
    0, 4, 5, 1,
    1, 5, 6, 2,
    4, 7, 6, 5
};

// Quadrilateral face of a cuboid.
struct Face
{
	FVector Normal;
    // Index of the face in its the Cuboid;
	//	   .+---------+  
	//	 .' |  0    .'|  
	//	+---+-----+'  |  
	//	|   |    3|   |  
	//	| 4 |     | 2 |  
	//	|   |1    |   |  
	//	|  ,+-----+---+  
	//	|.'    5  | .'   
	//	+---------+'    
	//	1 is in front.
	char Index;
	Face() {}
	Face(int i, FVector Vertices[])
    {
		Normal = FVector::CrossProduct(
			Vertices[FaceCuboidMap[i][1]] - Vertices[FaceCuboidMap[i][0]],
			Vertices[FaceCuboidMap[i][2]] - Vertices[FaceCuboidMap[i][0]]
		).GetSafeNormal(1e-9);
	}
	Face(const Face& F)
    {
        Index = F.Index;
		Normal = FVector(F.Normal);
	}
};

// A six-sided polyhedron defined by 8 vertices.
// A valid configuration of vertices is user-enforced.
// For example, all vertices of a face should be coplanar.
struct Cuboid
{
	Face Faces[CUBOID_F];
	FVector Vertices[CUBOID_V];
	Cuboid () {}
	// Constructs a cuboid from a list of vertices.
	// Vertices are ordered and indexed as such:
	//	    .1------0
	//	  .' |    .'|
	//	 2---+--3'  |
	//	 |   |  |   |
	//	 |  .5--+---4
	//	 |.'    | .'
	//	 6------7'
	Cuboid(TArray<FVector> V)
    {
		if (V.Num() != CUBOID_V)
        {
			return;
		}
		for (int i = 0; i < CUBOID_V; i++)
        {
			Vertices[i] = FVector(V[i]);
		}
		for (int i = 0; i < CUBOID_F; i++)
        {
			Faces[i] = Face(i, Vertices);
		}
	}
	Cuboid(const Cuboid& C)
    {
		for (int i = 0; i < CUBOID_V; i++)
        {
			Vertices[i] = FVector(C.Vertices[i]);
		}
		for (int i = 0; i < CUBOID_F; i++)
        {
			Faces[i] = Face(C.Faces[i]);
		}
	}
	// Return the vertex on face i with perimeter index j.
	const FVector& GetVertex(int i, int j) const
    {
		return Vertices[FaceCuboidMap[i][j]];
	}
};

// Checks if a Cuboid intersects a line segment between Start and
// Start + Direction * MaxTime.
// If there is an intersection, returns the time of the point of intersection,
// measured as a the fractional distance along the line segment.
// Otherwise, returns NaN.
// Implements Cyrus-Beck line clipping algorithm from:
// http://geomalgorithms.com/a13-_intersect-4.html
inline float IntersectionTime(
    const Cuboid* C,
    const FVector& Start,
    const FVector& Direction,
    const float MaxTime = 1)
{
    float TimeEnter = 0;
    float TimeExit = MaxTime;
    for (int i = 0; i < CUBOID_F; i++)
    {
        // Numerator of a plane/line intersection test.
        const FVector& Normal = C->Faces[i].Normal;
        float Num = (Normal | (C->GetVertex(i, 0) - Start));
        float Denom = Normal | Direction;
        if (Denom == 0)
        {
            // Start is outside of the plane,
            // so it cannot intersect the Cuboid.
            if (Num < 0)
            {
                return std::numeric_limits<double>::quiet_NaN();
            }
        }
        else
        {
            float t = Num / Denom;
            // The segment is entering the face.
            if (Denom < 0)
            {
                TimeEnter = std::max(TimeEnter, t);
            }
            else
            {
                TimeExit = std::min(TimeExit, t);
            }
            // The segment exits before entering,
            // so it cannot intersect the cuboid.
            if (TimeEnter > TimeExit)
            {
                return std::numeric_limits<double>::quiet_NaN();
            }
        }
    }
    return TimeEnter;
}

// Checks if a Cuboid intersects all line segments between Starts[i]
// and Ends[i]
// Implements Cyrus-Beck line clipping algorithm from:
// http://geomalgorithms.com/a13-_intersect-4.html
// Uses SIMD for 8x throughput.
inline bool IntersectsAll(
    const Cuboid* C,
    __m256 StartXs,
    __m256 StartYs,
    __m256 StartZs,
    __m256 EndXs,
    __m256 EndYs,
    __m256 EndZs)
{
    __m256 Zero = _mm256_set1_ps(0);
    __m256 EnterTimes = Zero;
    __m256 ExitTimes = _mm256_set1_ps(1);
    for (int i = 0; i < CUBOID_F; i++)
    {
        const FVector& Normal = C->Faces[i].Normal;
        __m256 NormalXs = _mm256_set1_ps(Normal.X);
        __m256 NormalYs = _mm256_set1_ps(Normal.Y);
        __m256 NormalZs = _mm256_set1_ps(Normal.Z);
        const FVector& Vertex = C->GetVertex(i, 0);
        __m256 Nums =
            _mm256_add_ps(
                _mm256_mul_ps(
                    _mm256_sub_ps(_mm256_set1_ps(Vertex.X), StartXs),
                    NormalXs),
                _mm256_add_ps(
                    _mm256_mul_ps(
                        _mm256_sub_ps(_mm256_set1_ps(Vertex.Y), StartYs),
                        NormalYs),
                    _mm256_mul_ps(
                        _mm256_sub_ps(_mm256_set1_ps(Vertex.Z), StartZs),
                        NormalZs)));
        __m256 Denoms =
            _mm256_add_ps(
                _mm256_mul_ps(_mm256_sub_ps(EndXs, StartXs), NormalXs),
                _mm256_add_ps(
                    _mm256_mul_ps(_mm256_sub_ps(EndYs, StartYs), NormalYs),
                    _mm256_mul_ps(_mm256_sub_ps(EndZs, StartZs), NormalZs)));
        int MissMask = _mm256_movemask_ps(
            _mm256_and_ps(
                _mm256_cmp_ps(Denoms, Zero, _CMP_EQ_OQ),
                _mm256_cmp_ps(Nums, Zero, _CMP_LE_OQ)));
        if (MissMask != 0)
            return false;
        __m256 Times = _mm256_div_ps(Nums, Denoms);
        __m256 PositiveMask = _mm256_cmp_ps(Denoms, Zero, _CMP_GT_OS);
        __m256 NegativeMask = _mm256_cmp_ps(Denoms, Zero, _CMP_LT_OS);
        EnterTimes = _mm256_blendv_ps(
            EnterTimes,
            _mm256_max_ps(EnterTimes, Times),
            NegativeMask);
        ExitTimes = _mm256_blendv_ps(
            ExitTimes,
            _mm256_min_ps(ExitTimes, Times),
            PositiveMask);
        MissMask = _mm256_movemask_ps(
            _mm256_cmp_ps(EnterTimes, ExitTimes, _CMP_GT_OS));
        if (MissMask != 0)
            return false;
    }
    return true;
}

struct Sphere
{
    FVector Center;
    float Radius;
    Sphere() { }
    Sphere(FVector Loc, float R)
    {
        Center = Loc;
        Radius = R;
    }
    Sphere(const Sphere& S)
    {
        Center = S.Center;
        Radius = S.Radius;
    }
};

// Optimized line segment that stores:
//   Start: The start position of the line segment.
//   Delta: The displacement vector from Start to End.
//   Reciprocal: The element-wise reciprocal of the displacement vector.
struct OptSegment
{
    FVector Start;
    FVector Reciprocal;
    FVector Delta;
    OptSegment() {}
    OptSegment(FVector Start, FVector End)
    {
        this->Start = Start;
        Delta = End - Start;
        Reciprocal = Delta.Reciprocal();
    }
};
