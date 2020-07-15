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
#include "CullingController.generated.h"

// Number of peeks in each Bundle.
constexpr char NUM_PEEKS = 4;
// Maximum number of characters in a game.
constexpr char MAX_CHARACTERS = 12;
// Number of cuboids in each entry of the cuboid cache array.
constexpr char CUBOID_CACHE_SIZE = 3;

// A volume that bounds a character.
// Uses a bounding sphere to quickly check visibility.
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
	TArray<FVector> TopVertices;
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
};

// Bundle representing lines of sight between a player's possible peeks
// and an enemy's bounds. Bounds are stored in a separate field of
// the CullingController to prevent data duplication.
struct Bundle
{
	unsigned char PlayerI;
	unsigned char EnemyI;
    TArray<FVector> PossiblePeeks;
	Bundle(int i, int j, const TArray<FVector>& Peeks)
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
	// If the rolling max time to cull exceeds the threshold, set TimerIncrement
	// to MaxTimerIncrement. Else set it to MinTimerIncrement.
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
	TArray<int> GetPossibleOccludingCuboids(const Bundle& B);
    // Gets corners of the rectangle encompassing a player's possible peeks
    // on an enemy--in the plane normal to the line of sight.
    // When facing along the vector from player to enemy, Corners are indexed
    // starting from the top right, proceeding counter-clockwise.
    // NOTE:
    //   Inaccurate on very wide enemies, as the most aggressive angle to peek
    //   the left of an enemy is actually perpendicular to the leftmost point
    //   of the enemy, not its center.
    TArray<FVector> GetPossiblePeeks(
        const FVector& PlayerCameraLocation,
        const FVector& EnemyLocation,
		float MaxDeltaHorizontal,
		float MaxDeltaVertical
    );
	// Gets all faces that sit between a player and an enemy and have a normal
	// pointing out toward the player, thus skipping redundant back faces.
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
	// Checks if a Sphere blocks all lines of sight between a player's
    // possible peeks and an enemy.
	bool IsBlocking(const Bundle& B, const Sphere& OccludingSphere);
	// Checks if a Cuboid blocks all lines of sight between a player's
    // possible peeks and an enemy.
	bool IsBlocking(const Bundle& B, const Cuboid& OccludingCuboid);
    // Checks if a Cuboid intersects a line segment.
    bool Intersects(const FVector& Start, const FVector& End, const Cuboid& OccludingCuboid);
    // For each plane, define a half-space by the set of all points
    // with a positive (dot product with its normal vector.
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
