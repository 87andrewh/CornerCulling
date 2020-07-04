// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "CornerCullingCharacter.h"
#include "GameFramework/Info.h"
#include "DrawDebugHelpers.h"
#include "CullingController.generated.h"

// Number of vertices and faces of a cuboid.
constexpr char CUBOID_V = 8;
constexpr char CUBOID_F = 6;
// Number of vertices in a face of a cuboid.
constexpr char CUBOID_FACE_V = 4;
// Number of peeks checked to account for latency.
constexpr char NUM_PEEKS = 4;
// Maximum number of characters in a game.
constexpr char MAX_CHARACTERS = 12;
// Number of cuboids in each entry of the cuboid cache array.
constexpr char CUBOID_CACHE_SIZE = 3;

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

// Cuboid defined by 8 vertices.
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
    Sphere()
    {
        Center = FVector();
        Radius = 100;
    }
    Sphere(FVector Loc, float R)
    {
        Center = Loc;
        Radius = R;
    }
};

// A volume that bounds a character. Uses a sphere to quickly determine if objects are obviously occluded or hidden.
// If the sphere is only partially occluded, then check against all tight bounding points.
struct CharacterBounds
{
	// Location of character's camera.
	FVector CameraLocation;
	// Center of character and bounding spheres.
	FVector Center;
	// Bounding sphere that circumscribes the bounding box.
	// Can quickly determine if the entire bounding box is occluded.
	float BoundingSphereRadius = 105;
	// Divide vertices to skip the bottom half when a payer peeks it from above and vice versa.
	// This halves the work, but can over-cull if the bottom edge of an enemy is not aligned with the top
	// and that bottom edge would stick out out when peeking over a chamfered wall.
	// But most bounding boxes should not vary along the Z axis.
	// We could also apply this idea to the left and right, but it would require a little more code.
	// Vertices in the top half of the bounding volume.
	TArray<FVector> TopVertices;
	// Vertices in bottom half of the bounding volume.
	TArray<FVector> BottomVertices;
	CharacterBounds(FVector CameraLocation, FTransform T)
    {
		this->CameraLocation = CameraLocation;
		Center = T.GetTranslation();
		TopVertices.Emplace(T.TransformPositionNoScale(FVector(30, 15, 100)));
		TopVertices.Emplace(T.TransformPositionNoScale(FVector(30, -15, 100)));
		TopVertices.Emplace(T.TransformPositionNoScale(FVector(-30, 15, 100)));
		TopVertices.Emplace(T.TransformPositionNoScale(FVector(-30, -15, 100)));
		BottomVertices.Emplace(T.TransformPositionNoScale(FVector(30, 15, -100)));
		BottomVertices.Emplace(T.TransformPositionNoScale(FVector(30, -15, -100)));
		BottomVertices.Emplace(T.TransformPositionNoScale(FVector(-30, 15, -100)));
		BottomVertices.Emplace(T.TransformPositionNoScale(FVector(-30, -15, -100)));
	}
	CharacterBounds() { }
};

// Bundle representing lines of sight between a player's possible peeks
// and an enemy's bounds.
struct Bundle
{
	unsigned char PlayerI;
	unsigned char EnemyI;
	Bundle(int i, int j)
    {
		PlayerI = i;
		EnemyI = j;
	}
};

/**
 *  Controls all occlusion culling logic.
 */
UCLASS()
class ACullingController : public AInfo
{
	GENERATED_BODY()
	// Keeps track of playable characters.
	TArray<ACornerCullingCharacter*> Characters;
	// Tracks if each character is alive.
    // TODO:
    //  Integrate with Character class to update status.
	TArray<bool> IsAlive;
	// Tracks team of each character.
	TArray<char> Teams;
	// Bounding volumes of all characters.
	TArray<CharacterBounds> Bounds;
	// Cache of cuboids that recently blocked LOS from one character to another.
	int CuboidCaches[MAX_CHARACTERS][MAX_CHARACTERS][CUBOID_CACHE_SIZE] = { 0 };
	// Timers that track the last time a cache element culled.
	int CacheTimers[MAX_CHARACTERS][MAX_CHARACTERS][CUBOID_CACHE_SIZE] = { 0 };
	// All occluding cuboids in the map.
	TArray<Cuboid> Cuboids;
	// All occluding spheres in the map.
	TArray<Sphere> Spheres;
	// Queues of line-of-sight bundles needing to be culled.
    // The two queues alternate as input and output of a CullWith...() function.
    // For example, CullWithCache takes input from BundleQueue,
    // and outputs unculled bundles into BundleQueue2.
    // Then CullWithSpheres takes input from BundleQueue2, etc.
	TArray<Bundle> BundleQueue;
	TArray<Bundle> BundleQueue2;
	// Used to find duplicate edges when merging faces.
	// Not perfectly space efficient, but fast and simple.
	bool EdgeSet[CUBOID_V][CUBOID_V] =
	{
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
	};

	// Stores how much longer the second character is visible to the first.
	int VisibilityTimers[MAX_CHARACTERS][MAX_CHARACTERS] = {0};
	// How many culling cycles an enemy stays visible for.
	// An enemy stays visible for TimerIncrement * CullingPeriod ticks.
	int MinTimerIncrement = 10;
	// Bigger increment for when the server is under heavy load.
	int MaxTimerIncrement = 18;
	int TimerIncrement = MinTimerIncrement;
	// If the rolling max time to cull exceeds the threshold, set TimerIncrement to
	// MaxTimerIncrement. Else set it to MinTimerIncrement.
	int TimerLoadThreshold = 1000;

	// How many frames pass between each cull.
	int CullingPeriod = 4;
	// Used to calculate short rolling average of frame times.
	float RollingTotalTime = 0;
	float RollingAverageTime = 0;
	// Stores maximum culling time in rolling window.
	int RollingMaxTime = 0;
	// Number of ticks in the rolling window.
	int RollingWindowLength =  CullingPeriod * 20;
	// Total tick counter
	int TotalTicks = 0;
	// Store overall average culling time (in microseconds)
	int TotalTime = 0;

	// Calculates all LOS bundles and add them to the queue.
	void PopulateBundles();
	// Culls all bundles with each player's cache of occluders.
	void CullWithCache();
	// Culls queued bundles with occluding spheres.
	void CullWithSpheres();
	// Culls queued bundles with occluding cuboids.
	void CullWithCuboids();
	// Gets indices of cuboids that could block LOS between the player and enemy
    // in the Bundle.
	TArray<int> GetPossibleOccludingCuboids(Bundle B);
    // Gets corners of the rectangle encompassing a player's possible peeks
    // on an enemy--in the plane normal to the line of sight.
    // When facing along the vector from player to enemy, Corners are indexed
    // starting from the top right, proceeding counter-clockwise.
    // NOTE:
    //   Inaccurate on very wide enemies, as the most aggressive angle to peek
    //   the left of an enemy is actually perpendicular to the leftmost point
    //   of the enemy, not its center.
    TArray<FVector> GetPossiblePeeks(
        FVector PlayerCameraLocation,
        FVector EnemyLocation,
		float MaxDeltaHorizontal,
		float MaxDeltaVertical
    );
	// Gets all faces that sit between a player and an enemy and have a normal pointing outward
	// toward the player, thus skipping redundant back faces.
	TArray<Face> GetFacesBetween(
		const FVector& PlayerCameraLocation,
		const FVector& EnemyCenter,
		const Cuboid& OccludingCuboid
	);
	// Get the shadow frustum. Given a point light shining on a polyhedron,
	// the shadow frustum is comprised of all planes bordering the dark region.
	// Formally, the shadow frustum is comprised of all planes defined by the
	// player camera location and two endpoints of a perimeter edge of the
	// surface formed by connecting all player-facing planes between a player
	// camera and an enemy.
	//              /--       
	//           /--           
	//        /-+-------+             
	//     /--  |       |     
	//  P--     |       |   E   
	//     \--  |       |             
	//        \-+-------+            
	//           \--           
	//              \--      
	// 2D example. P for player camera, E for enemy.
	void GetShadowFrustum(
		const FVector& PlayerCameraLocation,
		const Cuboid& OccludingCuboid,
		const TArray<Face>& FacesBetween,
		TArray<FPlane>& ShadowFrustum
	);
	// Checks if a Sphere blocks all lines of sight between a player's possible
	// peeks and points in an enemy's bounding box, stored in a bundle.
	bool IsBlocking(const Bundle& B, Sphere& OccludingSphere);
	// Checks if a Cuboid blocks all lines of sight between a player's possible
	// peeks and points in an enemy's bounding box, stored in a bundle.
	bool IsBlocking(const Bundle& B, Cuboid& OccludingCuboid);
    // For each plane, define a half-space by the set of all points
    // with a positive dot product with its normal vector.
    // Checks that every point is within all half-spaces.
	bool InHalfSpaces(const TArray<FVector>& Points, const TArray<FPlane>& Planes);
	// Sends character j's location to character i for all (i, j) pairs
    // if character j is visible to character i.
	void SendLocations();
	// Sends character j's location to character i.
	void SendLocation(int i, int j);

protected:
	void BeginPlay() override;

public:
	ACullingController();
	virtual void Tick(float DeltaTime) override;
	// Cull visibility for all player, enemy pairs.
	void Cull();
	// Cull while gathering and reporting runtime statistics.
	void BenchmarkCull();

	// Mark a vector. For debugging.
	static inline void MarkFVector(UWorld* World, const FVector& V)
    {
		DrawDebugLine(World, V, V + FVector(0, 0, 100), FColor::Red, false, 0.1f, 0, 2.f);
	}
	
	// Draw a line between two vectors. For debugging.	
	static inline void ConnectVectors(
		UWorld* World,
		const FVector& V1,
		const FVector& V2,
		bool Persist = false,
		float Lifespan = 0.1f,
		float Thickness = 2.0f,
		FColor Color = FColor::Emerald)
	{
		DrawDebugLine(World, V1, V2, Color, Persist, Lifespan, 0, Thickness);
	}

    // Get the index of the minimum element in an array.
	static inline int ArgMin(int A[], int Length)
    {
		int Min = INT_MAX;
		int MinI = 0;
		for (int i = 0; i < Length; i++)
        {
			if (A[i] < Min)
            {
				Min = A[i];
				MinI = i;
			}
		}
		return MinI;
	}
};
