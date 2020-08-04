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

struct Sphere
{
    FVector Center;
    float Radius;
    Sphere() {}
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

// Bundle representing lines of sight between a player's possible peeks
// and an enemy's bounds. Bounds are stored in a field of
// the CullingController to prevent data duplication.
struct Bundle
{
	unsigned char PlayerI;
	unsigned char EnemyI;
    std::vector<FVector> PossiblePeeks;
	Bundle(int i, int j, const std::vector<FVector>& Peeks)
    {
		PlayerI = i;
		EnemyI = j;
        PossiblePeeks = Peeks;
	}
};

// A volume that bounds a character.
// Has a bounding sphere to quickly check visibility.
// Uses the vertices of a bounding box to accurately check visibility.
struct CharacterBounds
{
    // Location of character's camera.
    FVector CameraLocation;
    // Center of character and bounding spheres.
    FVector Center;
    float BoundingSphereRadius = 105;
    // Divide vertices into top and bottom to skip the bottom half when
    // a player peeks it from above, and vice versa for peeks from below.
    // This computational shortcut assumes that each bottom vertex is
    // directly below a corresponding top vertex.
    std::vector<FVector> TopVertices;
    std::vector<FVector> BottomVertices;
    // We also precalculate and store representations optimized for SIMD.
    __m256 TopVerticesXs;
    __m256 TopVerticesYs;
    __m256 TopVerticesZs;
    __m256 BottomVerticesXs;
    __m256 BottomVerticesYs;
    __m256 BottomVerticesZs;
    CharacterBounds(FVector CameraLocation, FTransform T)
    {
        this->CameraLocation = CameraLocation;
        Center = T.GetTranslation();
        TopVertices.emplace_back(T.TransformPositionNoScale(FVector(30, 15, 100)));
        TopVertices.emplace_back(T.TransformPositionNoScale(FVector(30, -15, 100)));
        TopVertices.emplace_back(T.TransformPositionNoScale(FVector(-30, 15, 100)));
        TopVertices.emplace_back(T.TransformPositionNoScale(FVector(-30, -15, 100)));
        BottomVertices.emplace_back(T.TransformPositionNoScale(FVector(30, 15, -100)));
        BottomVertices.emplace_back(T.TransformPositionNoScale(FVector(30, -15, -100)));
        BottomVertices.emplace_back(T.TransformPositionNoScale(FVector(-30, 15, -100)));
        BottomVertices.emplace_back(T.TransformPositionNoScale(FVector(-30, -15, -100)));
        TopVerticesXs = _mm256_set_ps(
            TopVertices[0].X, TopVertices[1].X, TopVertices[2].X, TopVertices[3].X, 
            TopVertices[0].X, TopVertices[1].X, TopVertices[2].X, TopVertices[3].X);
        TopVerticesYs = _mm256_set_ps(
            TopVertices[0].Y, TopVertices[1].Y, TopVertices[2].Y, TopVertices[3].Y, 
            TopVertices[0].Y, TopVertices[1].Y, TopVertices[2].Y, TopVertices[3].Y);
        TopVerticesZs = _mm256_set_ps(
            TopVertices[0].Z, TopVertices[1].Z, TopVertices[2].Z, TopVertices[3].Z, 
            TopVertices[0].Z, TopVertices[1].Z, TopVertices[2].Z, TopVertices[3].Z);
        BottomVerticesXs = _mm256_set_ps(
            BottomVertices[0].X, BottomVertices[1].X, BottomVertices[2].X, BottomVertices[3].X, 
            BottomVertices[0].X, BottomVertices[1].X, BottomVertices[2].X, BottomVertices[3].X);
        BottomVerticesYs = _mm256_set_ps(
            BottomVertices[0].Y, BottomVertices[1].Y, BottomVertices[2].Y, BottomVertices[3].Y, 
            BottomVertices[0].Y, BottomVertices[1].Y, BottomVertices[2].Y, BottomVertices[3].Y);
        BottomVerticesZs = _mm256_set_ps(
            BottomVertices[0].Z, BottomVertices[1].Z, BottomVertices[2].Z, BottomVertices[3].Z, 
            BottomVertices[0].Z, BottomVertices[1].Z, BottomVertices[2].Z, BottomVertices[3].Z);
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
    const __m256 Zero = _mm256_set1_ps(0);
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
            _mm256_fmadd_ps(
                _mm256_sub_ps(_mm256_set1_ps(Vertex.X), StartXs),
                NormalXs,
                _mm256_fmadd_ps(
                    _mm256_sub_ps(_mm256_set1_ps(Vertex.Y), StartYs),
                    NormalYs,
                    _mm256_mul_ps(
                        _mm256_sub_ps(_mm256_set1_ps(Vertex.Z), StartZs),
                        NormalZs)));
        __m256 Denoms =
            _mm256_fmadd_ps(
                _mm256_sub_ps(EndXs, StartXs),
                NormalXs,
                _mm256_fmadd_ps(
                    _mm256_sub_ps(EndYs, StartYs),
                    NormalYs,
                    _mm256_mul_ps(_mm256_sub_ps(EndZs, StartZs), NormalZs)));
        // A line segment is parallel to and outside of a face.
        if (0 !=
            _mm256_movemask_ps(
                _mm256_and_ps(
                    _mm256_cmp_ps(Denoms, Zero, _CMP_EQ_OQ),
                    _mm256_cmp_ps(Nums, Zero, _CMP_LE_OQ))))
        {
            return false;
        }
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
        if (0 !=
            _mm256_movemask_ps(_mm256_cmp_ps(EnterTimes, ExitTimes, _CMP_GT_OS)))
        {
            return false;
        }
    }
    return true;
}

// Checks if the Cuboid blocks visibility between a player and enemy,
// returning true if and only if all lines of sights from the player's possible
// peeks are blocked.
// Assumes that the BottomVerticies of the enemy bounding box are directly below
// the TopVerticies.
inline bool IsBlocking(
    const std::vector<FVector>& Peeks,
    const CharacterBounds& Bounds,
    const Cuboid* C)
{
    __m256 StartXs = _mm256_set_ps(
        Peeks[0].X, Peeks[0].X, Peeks[0].X, Peeks[0].X,
        Peeks[1].X, Peeks[1].X, Peeks[1].X, Peeks[1].X);
    __m256 StartYs = _mm256_set_ps(
        Peeks[0].Y, Peeks[0].Y, Peeks[0].Y, Peeks[0].Y,
        Peeks[1].Y, Peeks[1].Y, Peeks[1].Y, Peeks[1].Y);
    __m256 StartZs = _mm256_set_ps(
        Peeks[0].Z, Peeks[0].Z, Peeks[0].Z, Peeks[0].Z,
        Peeks[1].Z, Peeks[1].Z, Peeks[1].Z, Peeks[1].Z);
    if (
        !IntersectsAll(
            C,
            StartXs, StartYs, StartZs,
            Bounds.TopVerticesXs, Bounds.TopVerticesYs, Bounds.TopVerticesZs))
    {
        return false;
    }
    else
    {
        StartXs = _mm256_set_ps(
            Peeks[2].X, Peeks[2].X, Peeks[2].X, Peeks[2].X,
            Peeks[3].X, Peeks[3].X, Peeks[3].X, Peeks[3].X);
        StartYs = _mm256_set_ps(
            Peeks[2].Y, Peeks[2].Y, Peeks[2].Y, Peeks[2].Y,
            Peeks[3].Y, Peeks[3].Y, Peeks[3].Y, Peeks[3].Y);
        StartZs = _mm256_set_ps(
            Peeks[2].Z, Peeks[2].Z, Peeks[2].Z, Peeks[2].Z,
            Peeks[3].Z, Peeks[3].Z, Peeks[3].Z, Peeks[3].Z);
        return IntersectsAll(
            C,
            StartXs, StartYs, StartZs,
            Bounds.BottomVerticesXs, Bounds.BottomVerticesYs, Bounds.BottomVerticesZs);
    }
}

// Checks sphere intersection for all line segments between
// a player's possible peeks and the vertices of an enemy's bounding box.
// Uses sphere and line segment intersection with formula from:
// http://paulbourke.net/geometry/circlesphere/index.html#linesphere
inline bool IsBlocking(
    const std::vector<FVector>& Peeks,
    const CharacterBounds& Bounds,
    const Sphere& OccludingSphere)
{
    // Unpack constant variables outside of loop for performance.
    const FVector SphereCenter = OccludingSphere.Center;
    const float RadiusSquared = OccludingSphere.Radius * OccludingSphere.Radius;
    for (int i = 0; i < Peeks.size(); i++)
    {
        FVector PlayerToSphere = SphereCenter - Peeks[i];
        const std::vector<FVector>* Vertices;
        if (i < 2)
        {
            Vertices = &Bounds.TopVertices;
        }
        else
        {
            Vertices = &Bounds.BottomVertices;
        }
        for (FVector V : *Vertices)
        {
            FVector PlayerToEnemy = V - Peeks[i];
            float u = (PlayerToEnemy | PlayerToSphere) / (PlayerToEnemy | PlayerToEnemy);
            // The point on the line between player and enemy that is closest to
            // the center of the occluding sphere lies between player and enemy.
            // Thus the sphere might intersect the line segment.
            if ((0 < u) && (u < 1))
            {
                FVector ClosestPoint = Peeks[i] + u * PlayerToEnemy;
                // The point lies within the radius of the sphere,
                // so the sphere intersects the line segment.
                if ((SphereCenter - ClosestPoint).SizeSquared() > RadiusSquared)
                {
                    return false;
                }
            }
            // The sphere does not intersect the line segment.
            else
            {
                return false;
            }
        }
    }
    return true;
}

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
