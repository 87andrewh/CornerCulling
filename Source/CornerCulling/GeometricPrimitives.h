/**
    @author Andrew Huang (87andrewh)
*/

#pragma once

#include "Containers/Array.h"
#include "Math/Vector.h"
#include <algorithm>

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
	//	To reiterate, 1 is in front, and we continue counterclockwise.
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
		).GetSafeNormal(1e-6);
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
	FVector Vertices [CUBOID_V];
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

// Optimized line segment that stores starting point and 1 / (End - Start)
struct OptSegment
{
    FVector Start;
    FVector Reciprocal;
    OptSegment() {}
    OptSegment(FVector Start, FVector End)
    {
        this->Start = Start;
        Reciprocal = (End - Start).Reciprocal();
    }
};

// Axis-Aligned Bounding Box.;
struct AABB
{
    FVector Min;
    FVector Max;
    AABB() {}
    AABB(const FVector& Min, const FVector& Max)
    {
        this->Min = Min;
        this->Max = Max;
    }
    float GetHalfSurfaceArea()
    {
        FVector Diagonal = Max - Min;
        return (
              Diagonal.X * Diagonal.Y
            + Diagonal.X * Diagonal.Z
            + Diagonal.Y * Diagonal.Z
        );
    }
    // Checks if the AABB intersects the line segment
    // between Start and End. Uses Slab method.
    // Code adapted from:
    // https://tavianator.com/cgit/dimension.git/tree/libdimension/bvh/bvh.c#n196
    bool Intersects(OptSegment Segment)
    {
        float TimeX1 = (Min.X - Segment.Start.X) * Segment.Reciprocal.X;
        float TimeX2 = (Max.X - Segment.Start.X) * Segment.Reciprocal.X;
        float TimeMin = std::min(TimeX1, TimeX2);
        float TimeMax = std::min(TimeX1, TimeX2);

        float TimeY1 = (Min.Y - Segment.Start.Y) * Segment.Reciprocal.Y;
        float TimeY2 = (Max.Y - Segment.Start.Y) * Segment.Reciprocal.Y;
        TimeMin = std::max(TimeMin, std::min(TimeY1, TimeY2));
        TimeMax = std::min(TimeMax, std::max(TimeY1, TimeY2));

        float TimeZ1 = (Min.Z - Segment.Start.Z) * Segment.Reciprocal.Z;
        float TimeZ2 = (Max.Z - Segment.Start.Z) * Segment.Reciprocal.Z;
        TimeMin = std::max(TimeMin, std::min(TimeZ1, TimeZ2));
        TimeMax = std::min(TimeMax, std::max(TimeZ1, TimeZ2));

        return (TimeMax >= std::max(0.0f, TimeMin)) && (TimeMin < 1);
    }
};
