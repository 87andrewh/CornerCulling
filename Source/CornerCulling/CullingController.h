// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "CornerCullingCharacter.h"
#include "GameFramework/Info.h"
#include "DrawDebugHelpers.h"
#include "CullingController.generated.h"

// Number of vertices and faces of a cuboid.
# define CUBOID_V 8
# define CUBOID_F 6
// Number of vertices in a face of a cuboid.
# define CUBOID_FACE_V 4
// Number of peeks checked to account for latency.
// Relevant for PossiblePeeks structs.
# define NUM_PEEKS 4

// Four-sided face of a cuboid.
struct Face {
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
	Face(int i, FVector Vertices[]) {
		switch (i) {
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
	Face(const Face& F) {
		Perimeter[0] = F.Perimeter[0];
		Perimeter[1] = F.Perimeter[1];
		Perimeter[2] = F.Perimeter[2];
		Perimeter[3] = F.Perimeter[3];
		Normal = FVector(F.Normal);
	}
};

// Cuboid defined by 8 vertices.
struct Cuboid {
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
	Cuboid(TArray<FVector> V) {
		if (V.Num() != CUBOID_V) {
			return;
		}
		for (int i = 0; i < CUBOID_V; i++) {
			Vertices[i] = FVector(V[i]);
		}
		for (int i = 0; i < CUBOID_F; i++) {
			Faces[i] = Face(i, Vertices);
		}
	}
	// Copy constructor.
	Cuboid(const Cuboid& C) {
		for (int i = 0; i < CUBOID_V; i++) {
			Vertices[i] = FVector(C.Vertices[i]);
		}
		for (int i = 0; i < CUBOID_F; i++) {
			Faces[i] = Face(C.Faces[i]);
		}
	}
	// Return the vertex on face i with perimeter index j.
	FVector GetVertex(int i, int j) const {
		return Vertices[Faces[i].Perimeter[j]];
	}
};

// A volume that bounds a character. Uses a sphere to quickly determine if objects are obviously occluded or hidden.
// If the sphere is only partially occluded, then check against all tight bounding points.
struct CharacterBounds {
	// Location of character's camera.
	FVector CameraLocation;
	// Center of character and bounding spheres.
	FVector Center;
	// Sphere that circumscribes the bounding box.
	// Can quickly determine if the entire bounding box is occluded.
	float OuterRadius = 105;
	// Sphere that inscribes the bounding box.
	// Can quickly determine if some of the bounding box is exposed.
	float InnerRadius = 16;
	// Divide vertices to skip the bottom half when a payer peeks it from above and vice versa.
	// This halves the work, but can over-cull if the bottom edge of an enemy is not aligned with the top
	// and that bottom edge would stick out out when peeking over a chamfered wall.
	// But most bounding boxes should not vary along the Z axis.
	// We could also apply this idea to the left and right, but it would require a little more code.
	// Vertices in the top half of the bounding volume.
	TArray<FVector> TopVertices;
	// Vertices in bottom half of the bounding volume.
	TArray<FVector> BottomVertices;
	CharacterBounds(FVector CL, FTransform T) {
		CameraLocation = CL;
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
	CharacterBounds() {}
};

// Corners of the rectangle encompassing a player's possible peeks on an enemy.
// Inaccurate on very wide enemies, as the most aggressive angle to peek
// the left of an enemy is actually perpendicular to the leftmost point
// of the enemy, not its center.
// When facing along the vector from player to enemy, Corners are indexed
// starting from the top right, proceeding counter-clockwise.
struct PossiblePeeks {
	TArray<FVector> Corners;
	PossiblePeeks() {}
	PossiblePeeks(
		FVector PlayerLocation,
	    FVector EnemyLocation,
		float MaxDeltaHorizontal,
		float MaxDeltaVertical
	) {
		FVector PlayerToEnemy = (EnemyLocation - PlayerLocation).GetSafeNormal(1e-6);
		// Horizontal vector is parallel to the XY plane and is perpendicular to PlayerToEnemy.
		FVector Horizontal = MaxDeltaHorizontal * FVector(-PlayerToEnemy.Y, PlayerToEnemy.X, 0);
		FVector Vertical = FVector(0, 0, MaxDeltaVertical);
		Corners.Emplace(PlayerLocation + Horizontal + Vertical);
		Corners.Emplace(PlayerLocation - Horizontal + Vertical);
		Corners.Emplace(PlayerLocation - Horizontal - Vertical);
		Corners.Emplace(PlayerLocation + Horizontal - Vertical);
	}
};

// Bundle representing all lines of sight between a player's possible peeks and an enemy's bounds.
struct Bundle {
	unsigned char PlayerI;
	unsigned char EnemyI;
	Bundle(int i, int j) {
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

	// Maximum number of characters in a game.
	#define MAX_CHARACTERS 12
	// Number of cuboids in each entry of the cuboid cache array.
	#define CUBOID_CACHE_SIZE 3
	// Keeps track of playable characters.
	TArray<ACornerCullingCharacter*> Characters;
	// Tracks if each character is alive.
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
	// Queue of bundles needing to be culled.
	// First queue is fed into CullWithCache; leftovers are culled from second.
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
	int MaxTimerIncrement = 20;
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

	// Update bounding volumes of characters according to their transformation.
	// Can be changed to include changes in gun length.
	void UpdateCharacterBounds();
	// Calculate all LOS bundles and add them to the queue.
	void PopulateBundles();
	// Cull all bundles using each player's cache of boxes.
	void CullWithCache();
	// Cull remaining bundles in the queue.
	void CullRemaining();
	// Get all faces that sit between a player and an enemy and have a normal pointing outward
	// toward the player, thus skipping redundant back faces.
	void GetFacesBetween(
		const FVector& PlayerCameraLocation,
		const FVector& EnemyCenter,
		const Cuboid& OccludingCuboid,
		TArray<Face>& FacesBetween
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

	// Check if a Cuboid blocks all lines of sight between a player's possible
	// peeks and points in an enemy's bounding box, stored in a bundle.
	bool IsBlocking(const Bundle& B, Cuboid& OccludingCuboid);

	// Check if all points are in the frustum defined by the planes.
	bool InFrustum(
		const TArray<FVector>& Points,
		const TArray<FPlane>& Planes
	);

	// Get all indices of cuboids that could block LOS between the player and enemy in the bundle.
	TArray<int> GetPossibleOccludingCuboids(Bundle B);

	// For all pairs of characters, if character i should be visible to j,
	// then send j's location to i.
	void SendLocations();

	// Send location of character j to character i.
	void SendLocation(int i, int j);

protected:
	void BeginPlay() override;

public:
	ACullingController();
	virtual void Tick(float DeltaTime) override;
	// Cull visibility for all player, enemy pairs.
	void Cull();
	// Cull while gathering and reporting runtime statistics.
	// Tracking runtime also enables load-adaptive culling.
	void BenchmarkCull();

	// Mark a vector. For debugging.
	static inline void MarkFVector(UWorld* World, const FVector& V) {
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

	static inline int ArgMin(int A[], int Length) {
		int Min = INT_MAX;
		int MinI = 0;
		for (int i = 0; i < Length; i++) {
			if (A[i] < Min) {
				Min = A[i];
				MinI = i;
			}
		}
		return MinI;
	}
};
