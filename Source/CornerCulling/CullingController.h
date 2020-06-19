// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "EngineMinimal.h"
#include "CornerCullingCharacter.h"
#include "GameFramework/Info.h"
#include "Math/UnrealMathUtility.h"
#include "DrawDebugHelpers.h"
#include "CullingController.generated.h"

// Number of vertices and faces of a cuboid.
# define CUBOID_V 8
# define CUBOID_F 6
// Number of vertices in a face of a cuboid.
# define CUBOID_FACE_V 4

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
	// Return the vertex on face i with perimeter index j.
	FVector GetVertex(int i, int j) {
		return Vertices[Faces[i].Perimeter[j]];
	}
};

// 3D Line segment. Defined by two points.
struct Segment {
	FVector Start, End;
	Segment() {}
	Segment(const FVector& V1, const FVector& V2) {
		Start = V1;
		End = V2;
	}
};

// A volume that bounds a character. Uses a sphere to quickly determine if objects are obviously occluded or hidden.
// If the sphere is only partially occluded, then check against all tight bounding points.
struct CharacterBounds {
	FVector Center;
	// Center of bounding sphere
	float Radius = 20;
	// Divide vertices to skip the bottom half when a payer peeks it from above and vice versa.
	// This halves the work, but can over-cull if the bottom edge of an enemy is not aligned with the top
	// and that bottom edge would stick out out when peeking over a chamfered wall.
	// But most bounding boxes should not vary along the Z axis.
	// We could also apply this idea to the left and right, but it would require a little more code.
	// Vertices in the top half of the bounding volume.
	TArray<FVector> TopVertices;
	// Vertices in bottom half of the bounding volume.
	TArray<FVector> BottomVertices;
	CharacterBounds(FTransform T) {
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

// Four positions that encompass a player's possible peeks on an enemy.
struct PossiblePeeks {
	FVector TopLeft, TopRight, BottomLeft, BottomRight;
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
		TopLeft = PlayerLocation + Horizontal + Vertical;
		TopRight = PlayerLocation - Horizontal + Vertical;
		BottomLeft = PlayerLocation + Horizontal - Vertical;
		BottomRight = PlayerLocation - Horizontal - Vertical;
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

	#define MAX_CHARACTERS 12
	#define CUBOID_CACHE_SIZE 3
	TArray<ACornerCullingCharacter*> Characters;
	TArray<bool> IsAlive;
	// Bounding volumes of all characters.
	TArray<CharacterBounds> Bounds;
	// All occluding cuboids in the map.
	// Cache of cuboids that recently blocked LOS from the first character to the second.
	int CuboidCaches[MAX_CHARACTERS][MAX_CHARACTERS][CUBOID_CACHE_SIZE] = { 0 };
	// Timers that track the last time a cache element culled.
	int CacheTimers[MAX_CHARACTERS][MAX_CHARACTERS][CUBOID_CACHE_SIZE] = { 0 };
	TArray<Cuboid> Cuboids;
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
	int TimerIncrement = 10;
	// Increase the increment when the server is under heavy load.
	int LongTimerIncrement = 30;

	// How many frames pass between each cull.
	int CullingPeriod = 4;
	// Used to calculate short rolling average of frame times.
	float RollingTotalTime = 0;
	float RollingAverageTime = 0;
	// Stores maximum culling time in rolling window.
	int RollingMaxTime = 0;
	// Number of frames in the rolling window.
	int RollingLength = 4 * CullingPeriod;
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
	// For all players, update the visibility of their enemies.
	void UpdateVisibility();
	// Clear all queues for next culling cycle.
	void ClearQueues();
	// Check if a Cuboid blocks all lines of sight between a player's possible peeks and points in an enemy's bounding box,
	// stored in a bundle.
	bool IsBlocking(const Bundle& B, Cuboid& OccludingCuboid);

	// Get all indices of cuboids that could block LOS between the player and enemy in the bundle.
	TArray<int> GetPossibleOccludingCuboids(Bundle B);

	// Send location of character j to character i.
	void SendLocation(int i, int j);

protected:
	void BeginPlay() override;

public:
	ACullingController();
	virtual void Tick(float DeltaTime) override;
	void Cull();
	void BenchmarkCull();

	// Mark a vector. For debugging.
	static inline void MarkFVector(UWorld* World, const FVector& V) {
		DrawDebugLine(World, V, V + FVector(0, 0, 100), FColor::Red, false, 0.1f, 0, 2.f);
	}
	
	// Draw a line between two vectors. For debugging.	
	static inline void ConnectVectors(UWorld* World, const FVector& V1, const FVector& V2, bool Persist = false) {
		float Duration = 0.1f;
		DrawDebugLine(World, V1, V2, FColor::Emerald, Persist, Duration, 0, 2.f);
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
