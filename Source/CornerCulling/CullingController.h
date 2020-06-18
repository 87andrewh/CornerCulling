// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "EngineMinimal.h"
#include "CornerCullingCharacter.h"
#include "GameFramework/Info.h"
#include "Math/UnrealMathUtility.h"
#include "CullingController.generated.h"

// Number of vertices and faces of a cuboid.
# define CUBOID_V 8
# define CUBOID_F 6
// Number of edges in a face of a cuboid.
# define CUBOID_E 4
// Cuboid defined by 8 points in space.
// TODO: Test cache alignment.

USTRUCT()
struct FCuboid {
	GENERATED_BODY()
	struct Face {
		FVector Normal;
		// Indexes of vertices that define the perimeter. Counter-clockwise from outside view.
		unsigned char Perimeter [CUBOID_E];
		Face() {}
		Face(unsigned char i, FCuboid* C) {
			switch (i) {
				case 0:
					Perimeter[0] = 0;
					Perimeter[1] = 1;
					Perimeter[2] = 2;
					Perimeter[3] = 3;
				case 1:
					Perimeter[0] = 2;
					Perimeter[1] = 6;
					Perimeter[2] = 7;
					Perimeter[3] = 3;
				case 2:
					Perimeter[0] = 0;
					Perimeter[1] = 3;
					Perimeter[2] = 7;
					Perimeter[3] = 4;
				case 3:
					Perimeter[0] = 0;
					Perimeter[1] = 4;
					Perimeter[2] = 5;
					Perimeter[3] = 1;
				case 4:
					Perimeter[0] = 1;
					Perimeter[1] = 5;
					Perimeter[2] = 6;
					Perimeter[3] = 2;
				case 5:
					Perimeter[0] = 4;
					Perimeter[1] = 7;
					Perimeter[2] = 6;
					Perimeter[3] = 5;
				Normal = FVector::CrossProduct(
					C->Vertices[Perimeter[1]] - C->Vertices[Perimeter[0]],
					C->Vertices[Perimeter[2]] - C->Vertices[Perimeter[0]]
				);
			}
		}
	};
	Face Faces[CUBOID_F];
	UPROPERTY(EditAnywhere)
	FVector Vertices [CUBOID_V];
	FCuboid () {}
	FCuboid(FVector V[]) {
		for (unsigned char i = 0; i < CUBOID_V; i++) {
			Vertices[i] = FVector(V[i]);
		}
		for (unsigned char i = 0; i < CUBOID_F; i++) {
			Faces[i] = Face(i, this);
		}
	}
};

// Vertical infinite wall. Defined by two 2D points.
struct ZWall {
	FVector2D P1, P2;
};

// 3D Line segment. Defined by two points.
struct Segment {
	FVector Start, End;
};

// A volume that bounds a character. Uses a sphere to quickly determine if objects are obviously occluded or hidden.
// If the sphere is only partially occluded, then check against all tight bounding points.
struct CharacterBounds {
	FVector Center;
	// Center of bounding sphere
	float Radius;
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
		TopVertices.Emplace(T.TransformVectorNoScale(FVector(10, 10, 10)));
		TopVertices.Emplace(T.TransformVectorNoScale(FVector(10, -10, 10)));
		TopVertices.Emplace(T.TransformVectorNoScale(FVector(-10, 10, 10)));
		TopVertices.Emplace(T.TransformVectorNoScale(FVector(-10, -10, 10)));
		BottomVertices.Emplace(T.TransformVectorNoScale(FVector(10, 10, -10)));
		BottomVertices.Emplace(T.TransformVectorNoScale(FVector(10, -10, -10)));
		BottomVertices.Emplace(T.TransformVectorNoScale(FVector(-10, 10, -10)));
		BottomVertices.Emplace(T.TransformVectorNoScale(FVector(-10, -10, -10)));
	}
	CharacterBounds() {}
};

// Four positions that encompass the player's possible peeks on an enemy.
struct PossiblePeeks {
	FVector TopLeft, TopRight, BottomLeft, BottomRight;
};

// Bundle representing all lines of sight between a player's possible peeks and an enemy's bounds.
struct Bundle {
	unsigned char PlayerI;
	unsigned char EnemyI;
	CharacterBounds* Bounds;
	PossiblePeeks* Peeks;
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
	TArray<FCuboid> OccludingCuboids;
	// Cache of cuboids that recently blocked LOS from the first character to the second.
	unsigned short CuboidCaches[MAX_CHARACTERS][MAX_CHARACTERS][CUBOID_CACHE_SIZE] = { 0 };
	TArray<FCuboid> CuboidQueue;
	TArray<Bundle> BundleQueue;
	TArray<Bundle> BundleQueue2;
	// Stores how much longer the second character is visible to the first.
	unsigned short VisibilityTimers[MAX_CHARACTERS][MAX_CHARACTERS] = {0};
	// How many culling cycles an enemy stays visible for.
	// An enemy stays visible for TimerIncrement * CullingPeriod ticks.
	unsigned short TimerIncrement = 10;
	// Increase the increment when the server is under heavy load.
	unsigned short LongTimerIncrement = 30;

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
	// Store overall average culling time.
	float TotalTime = 0;

	// Update bounding volumes of players according to transformation and children.
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
	bool CheckAllLOS(const Bundle& B, FCuboid& OccludingCuboid);

	// On the plane normal to the line between a play and an enemy, get the locations of corners of the
	// rectangle that encompasses all possible peeks.
	// The Magnitude is ideally a function of culling period, server latency, player kinematics, and game events.
	void GetPossiblePeeks(int PlayerI, int EnemyI, PossiblePeeks Possibilities);

protected:
	void BeginPlay() override;

public:
	ACullingController();
	virtual void Tick(float DeltaTime) override;
	void Cull();
	void BenchmarkCull();
};