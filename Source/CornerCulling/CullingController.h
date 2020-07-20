/**
    @author Andrew Huang (87andrewh)
*/

#pragma once
#include "CoreMinimal.h"
#include "CornerCullingCharacter.h"
#include "GameFramework/Info.h"
#include "DrawDebugHelpers.h"
#include "GeometricPrimitives.h"
#include "FastBVH.h"
#include <vector>
#include "CullingController.generated.h"


constexpr int SERVER_TICKRATE = 120;
// Simulated latency in ticks.
constexpr int CULLING_SIMULATED_LATENCY = 12;

// Number of peeks in each Bundle.
constexpr char NUM_PEEKS = 4;
// Maximum number of characters in a game.
constexpr char MAX_CHARACTERS = 100;
// Number of cuboids in each entry of the cuboid cache array.
constexpr char CUBOID_CACHE_SIZE = 3;

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
    // This shortcut assumes that top and bottom vertices share X and Y coordinates.
	// We could also apply this idea to the left and right,
    // but it would require more work to calculate rotations.
	std::vector<FVector> TopVertices;
	std::vector<FVector> BottomVertices;
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
	}
};

// Bundle representing lines of sight between a player's possible peeks
// and an enemy's bounds. Bounds are stored in a separate field of
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

/**
 *  Controls all occlusion culling logic.
 */
UCLASS()
class ACullingController : public AInfo
{
	GENERATED_BODY()
	// Keeps track of playable characters.
	std::vector<ACornerCullingCharacter*> Characters;
	// Tracks if each character is alive.
    // TODO:
    //  Integrate with Character class to update status.
	std::vector<bool> IsAlive;
	// Tracks team of each character.
	std::vector<char> Teams;
	// Bounding volumes of all characters.
	std::vector<CharacterBounds> Bounds;
	// Bounding volumes of all characters at past times.
    // Used to simulate latency in testing.
	std::deque<std::vector<CharacterBounds>> PastBounds;
	// Cache of pointers to cuboids that recently blocked LOS from
    // player i to enemy j. Accessed by CuboidCaches[i][j].
	const Cuboid* CuboidCaches[MAX_CHARACTERS][MAX_CHARACTERS][CUBOID_CACHE_SIZE] = { 0 };
	// Timers that track the last time a cache element culled.
	int CacheTimers[MAX_CHARACTERS][MAX_CHARACTERS][CUBOID_CACHE_SIZE] = { 0 };
	// All occluding cuboids in the map.
	std::vector<Cuboid> Cuboids;
    // Bounding volume hierarchy containing cuboids.
    std::unique_ptr<FastBVH::BVH<float, Cuboid>> CuboidBVH{};
    CuboidIntersector Intersector;
    // Note: Could be nice to use std::optional with C++17.
    std::unique_ptr
        <Traverser<float, Cuboid, decltype(Intersector)>>
        CuboidTraverser {};
	// All occluding spheres in the map.
	std::vector<Sphere> Spheres;
	// Queues of line-of-sight bundles needing to be culled.
	std::vector<Bundle> BundleQueue;

	// How many frames pass between each cull.
	int CullingPeriod = 4;
	// Stores how many ticks character j remains visible to character i for.
	int VisibilityTimers[MAX_CHARACTERS][MAX_CHARACTERS] = {0};
	// How many ticks an enemy stays visible for after being revealed.
	int VisibilityTimerMax = CullingPeriod * 2;
	// Used to calculate short rolling average of frame times.
	float RollingTotalTime = 0;
	float RollingAverageTime = 0;
	// Stores maximum culling time in rolling window.
	int RollingMaxTime = 0;
	// Number of ticks in the rolling window.
	int RollingWindowLength =  SERVER_TICKRATE;
	// Total ticks since game start.
	int TotalTicks = 0;
	// Stores total culling time to calculate an overall average.
	int TotalTime = 0;

	// Cull visibility for all player, enemy pairs.
	void Cull();
    // Updates the bounding volumes of characters.
    void UpdateCharacterBounds();
	// Calculates all bundles of lines of sight between characters,
    // adding them to the BundleQueue for culling.
	void PopulateBundles();
	// Culls all bundles with each player's cache of occluders.
	void CullWithCache();
	// Culls queued bundles with occluding spheres.
	void CullWithSpheres();
	// Culls queued bundles with occluding cuboids.
	void CullWithCuboids();
	// Gets pointers of cuboids that could block LOS between the player and enemy
    // in the Bundle.
	std::vector<const Cuboid*> GetPossibleOccludingCuboids(const Bundle& B);
    // Gets corners of the rectangle encompassing a player's possible peeks
    // on an enemy--in the plane normal to the line of sight.
    // When facing along the vector from player to enemy, Corners are indexed
    // starting from the top right, proceeding counter-clockwise.
    // NOTE:
    //   Inaccurate on very wide enemies, as the most aggressive angle to peek
    //   the left of an enemy is actually perpendicular to the leftmost point
    //   of the enemy, not its center.
    static std::vector<FVector> GetPossiblePeeks(
        const FVector& PlayerCameraLocation,
        const FVector& EnemyLocation,
		float MaxDeltaHorizontal,
		float MaxDeltaVertical);
    // Gets the estimated latency of player i in seconds.
    float GetLatency(int i);
	// Checks if a Sphere blocks all lines of sight between a player's
    // possible peeks and an enemy.
	bool IsBlocking(const Bundle& B, const Sphere& OccludingSphere);
	// Checks if a Cuboid blocks all lines of sight between a player's
    // possible peeks and an enemy.
	bool IsBlocking(const Bundle& B, const Cuboid* OccludingCuboid);
    // Converts culling results into changes in in-game visibility.
	void UpdateVisibility();
	// Sends character j's location to character i.
	void SendLocation(int i, int j);

protected:
	void BeginPlay() override;

public:
	ACullingController();
	virtual void Tick(float DeltaTime) override;
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
