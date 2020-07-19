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

// Quadrilateral face of a cuboid.
struct Face
{
	FVector Normal;
	// Indexes of vertices on the perimeter. Counter-clockwise from outside perspective.
	unsigned char Perimeter [CUBOID_FACE_V];
	Face() {}
	// Faces are ordered
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
	Face(int i, FVector Vertices[])
    {
		switch (i)
        {
			case 0:
				Perimeter[0] = 0;
				Perimeter[1] = 1;
				Perimeter[2] = 2;
				Perimeter[3] = 3;
				break;
			case 1:
				Perimeter[0] = 2;
				Perimeter[1] = 6;
				Perimeter[2] = 7;
				Perimeter[3] = 3;
				break;
			case 2:
				Perimeter[0] = 0;
				Perimeter[1] = 3;
				Perimeter[2] = 7;
				Perimeter[3] = 4;
				break;
			case 3:
				Perimeter[0] = 0;
				Perimeter[1] = 4;
				Perimeter[2] = 5;
				Perimeter[3] = 1;
				break;
			case 4:
				Perimeter[0] = 1;
				Perimeter[1] = 5;
				Perimeter[2] = 6;
				Perimeter[3] = 2;
				break;
			case 5:
				Perimeter[0] = 4;
				Perimeter[1] = 7;
				Perimeter[2] = 6;
				Perimeter[3] = 5;
				break;
		}
		Normal = FVector::CrossProduct(
			Vertices[Perimeter[1]] - Vertices[Perimeter[0]],
			Vertices[Perimeter[2]] - Vertices[Perimeter[0]]
		).GetSafeNormal(1e-9);
	}
	Face(const Face& F)
    {
		Perimeter[0] = F.Perimeter[0];
		Perimeter[1] = F.Perimeter[1];
		Perimeter[2] = F.Perimeter[2];
		Perimeter[3] = F.Perimeter[3];
		Normal = FVector(F.Normal);
	}
};

// A six-sided polyhedron defined by 8 vertices.
// A valid configuration of vertices is  not strictly enforced.
// A face could contain non-coplanar vertices.
struct Cuboid
{
	Face Faces[CUBOID_F];
	FVector Vertices[CUBOID_V];
	Cuboid () {}
	// Construct a cuboid from a list of vertices.
	// vertices should be ordered
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
	// Copy constructor.
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
	FVector GetVertex(int i, int j) const
    {
		return Vertices[Faces[i].Perimeter[j]];
	}
};

// Checks if a Cuboid intersects a line segment between Start and
// Start + Direction * MaxTime.
// If there is an intersection, returns the time of the point of intersection,
// measured as a the fractional distance along the line segment.
// Otherwise, returns NaN.
// Implements Cyrus-Beck line clipping algorithm.
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
        float Denom = Direction | Normal;
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
