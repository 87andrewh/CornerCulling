#include "CullingController.h"
#include "OccludingCuboid.h"
#include "OccludingSphere.h"
#include "EngineUtils.h"
#include <chrono> 

ACullingController::ACullingController()
	: Super()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
}

void ACullingController::BeginPlay()
{
	Super::BeginPlay();
    for (ACornerCullingCharacter* Player : TActorRange<ACornerCullingCharacter>(GetWorld()))
    {
		Characters.Add(Player);
		IsAlive.Emplace(true);
		Teams.Emplace(Player->Team);
    }
    int MaxRenderedCuboids = 100;
    for (AOccludingCuboid* Occluder : TActorRange<AOccludingCuboid>(GetWorld()))
    {
        if (MaxRenderedCuboids > 0)
        {
            Occluder->DrawEdges(true);
            MaxRenderedCuboids--;
        }
        const Cuboid& C = Cuboid(Occluder->Vectors);
		Cuboids.emplace_back(C);
    }
    FastBVH::BuildStrategy<float, 1> BuildStrategy;
    CuboidBoxConverter Converter;
    CuboidBVH = std::make_unique
        <FastBVH::BVH<float, Cuboid>>
        (BuildStrategy(Cuboids, Converter));
    CuboidTraverser = std::make_unique
        <Traverser<float, Cuboid, decltype(Intersector)>>
        (*CuboidBVH.get(), Intersector);
    for (AOccludingSphere* Occluder : TActorRange<AOccludingSphere>(GetWorld()))
    {
        Spheres.Add(Sphere(Occluder->GetActorLocation(), Occluder->Radius));
    }
}

void ACullingController::BenchmarkCull()
{
	auto Start = std::chrono::high_resolution_clock::now();
	Cull();
	auto Stop = std::chrono::high_resolution_clock::now();
	UpdateVisibility();
	int Delta = std::chrono::duration_cast<std::chrono::microseconds>(Stop - Start).count();
	TotalTime += Delta;
	RollingTotalTime += Delta;
	RollingMaxTime = std::max(RollingMaxTime, Delta);
	if ((TotalTicks % RollingWindowLength) == 0)
    {
		RollingAverageTime = RollingTotalTime / RollingWindowLength;
		if (GEngine)
        {
			// One cull happens every CullingPeriod frames.
            // TODO:
            //   When running multiple servers per CPU,
            //   stagger culling periods so that lag spikes do not build up.
			FString Msg = "Average time to cull (microseconds): " + FString::FromInt(int(TotalTime / TotalTicks));
			GEngine->AddOnScreenDebugMessage(1, 1.1f, FColor::Yellow, Msg, true, FVector2D(1.5f, 1.5f));
			Msg = "Rolling average time to cull (microseconds): " + FString::FromInt(int(RollingAverageTime));
			GEngine->AddOnScreenDebugMessage(2, 1.1f, FColor::Yellow, Msg, true, FVector2D(1.5f, 1.5f));
			Msg = "Rolling max time to cull (microseconds): " + FString::FromInt(RollingMaxTime);
			GEngine->AddOnScreenDebugMessage(3, 1.1f, FColor::Yellow, Msg, true, FVector2D(1.5f, 1.5f));
		}
		RollingTotalTime = 0;
		RollingMaxTime = 0;
	}
}

void ACullingController::Cull()
{
	if ((TotalTicks % CullingPeriod) == 0)
    {
		PopulateBundles();
		CullWithCache();
		CullWithSpheres();
		CullWithCuboids();
	}
}

void ACullingController::PopulateBundles()
{
    // First update character bounds.
    Bounds.Reset();
    for (int i = 0; i < Characters.Num(); i++)
    {
        if (IsAlive[i])
        {
			Bounds.EmplaceAt(i, CharacterBounds(
				Characters[i]->GetFirstPersonCameraComponent()->GetComponentLocation(),
				Characters[i]->GetActorTransform()
			));
        }
    }
    // Then update bundles.
	BundleQueue.Reset();
	for (int i = 0; i < Characters.Num(); i++)
    {
		if (IsAlive[i])
        {
			for (int j = 0; j < Characters.Num(); j++)
            {
                // Decrement visibility timers.
				if (VisibilityTimers[i][j] > 0)
                {
					VisibilityTimers[i][j]--;
				}
				if (   VisibilityTimers[i][j] == 0
                    && IsAlive[j]
                    && (Teams[i] != Teams[j]))
                {   
                    // TODO:
                    //   Make displacement a function of latency and game state.
					BundleQueue.Emplace(
                        Bundle(
                            i,
                            j,
                            GetPossiblePeeks(
                                Bounds[i].CameraLocation,
                                Bounds[j].Center,
                                15,  // Maximum horizontal displacement
                                10   // Maximum vertical displacement
                            )
                        )
                    );
				}
			}
		}
	}
}

TArray<FVector> ACullingController::GetPossiblePeeks(
    const FVector& PlayerCameraLocation,
    const FVector& EnemyLocation,
    float MaxDeltaHorizontal,
    float MaxDeltaVertical)
{
    TArray<FVector> Corners;
    FVector PlayerToEnemy =
        (EnemyLocation - PlayerCameraLocation).GetSafeNormal(1e-6);
	// Displacement parallel to the XY plane and perpendicular to PlayerToEnemy.
	FVector Horizontal =
        MaxDeltaHorizontal * FVector(-PlayerToEnemy.Y, PlayerToEnemy.X, 0);
	FVector Vertical = FVector(0, 0, MaxDeltaVertical);
	Corners.Emplace(PlayerCameraLocation + Horizontal + Vertical);
	Corners.Emplace(PlayerCameraLocation - Horizontal + Vertical);
	Corners.Emplace(PlayerCameraLocation - Horizontal - Vertical);
	Corners.Emplace(PlayerCameraLocation + Horizontal - Vertical);
    return Corners;
}

void ACullingController::CullWithCache()
{
	TArray<Bundle> Remaining;
	for (Bundle B : BundleQueue)
    {
		bool Blocked = false;
		for (int k = 0; k < CUBOID_CACHE_SIZE; k++)
        {
            if (CuboidCaches[B.PlayerI][B.EnemyI][k] != NULL)
            {
			    if (IsBlocking(B, CuboidCaches[B.PlayerI][B.EnemyI][k]))
                {
			    	Blocked = true;
			    	CacheTimers[B.PlayerI][B.EnemyI][k] = TotalTicks;
			    	break;
			    }
            }
		}
		if (!Blocked)
        {
			Remaining.Emplace(B);
		}
	}
    BundleQueue = Remaining;
}

void ACullingController::CullWithSpheres()
{
	TArray<Bundle> Remaining;
	for (Bundle B : BundleQueue)
    {
		bool Blocked = false;
        for (Sphere S: Spheres)
        {
			if (IsBlocking(B, S))
            {
				Blocked = true;
				break;
			}
		}
		if (!Blocked)
        {
			Remaining.Emplace(B);
		}
	}
    BundleQueue = Remaining;
}

void ACullingController::CullWithCuboids()
{
	TArray<Bundle> Remaining;
	for (Bundle B : BundleQueue)
    {
		bool Blocked = false;
        //auto intersection = Traverser.traverse();
        std::vector<const Cuboid*> Occluders = GetPossibleOccludingCuboids(B);
		for (const Cuboid* CuboidP : Occluders)
        {
			if (IsBlocking(B, CuboidP))
            {
				Blocked = true;
				int MinI =
                    ArgMin(CacheTimers[B.PlayerI][B.EnemyI], CUBOID_CACHE_SIZE);
				CuboidCaches[B.PlayerI][B.EnemyI][MinI] = CuboidP;
				CacheTimers[B.PlayerI][B.EnemyI][MinI] = TotalTicks;
				break;
			}
		}
		if (!Blocked)
        {
			Remaining.Emplace(B);
		}
	}
    BundleQueue = Remaining;
}

// Checks if the Cuboid blocks visibility between a bundle's player and enemy,
// returning true if and only if all lines of sights from all peeking positions
// are blocked.
bool ACullingController::IsBlocking(const Bundle& B, const Cuboid* C)
{
    const CharacterBounds& EnemyBounds = Bounds[B.EnemyI];
    const TArray<FVector>& Peeks = B.PossiblePeeks;
    // The cuboid does not block the bundle if it fails to block any peek.
    for (const FVector& V : EnemyBounds.TopVertices)
    {
        if (std::isnan(IntersectionTime(C, Peeks[0], V - Peeks[0])))
            return false;
        if (std::isnan(IntersectionTime(C, Peeks[1], V - Peeks[1])))
            return false;
    }
    for (const FVector& V : EnemyBounds.BottomVertices)
    {
        if (std::isnan(IntersectionTime(C, Peeks[2], V - Peeks[2])))
            return false;
        if (std::isnan(IntersectionTime(C, Peeks[3], V - Peeks[3])))
            return false;
    }
	return true;
}

// Checks sphere intersection for all line segments between
// a player's possible peeks and the vertices of an enemy's bounding box.
// Uses sphere and line segment intersection with formula from:
// http://paulbourke.net/geometry/circlesphere/index.html#linesphere
bool ACullingController::IsBlocking(const Bundle& B, const Sphere& OccludingSphere)
{
    // Unpack constant variables outside of loop for performance.
    const CharacterBounds& EnemyBounds = Bounds[B.EnemyI];
    const TArray<FVector>& Peeks = B.PossiblePeeks;
    const FVector SphereCenter = OccludingSphere.Center;
    const float RadiusSquared = OccludingSphere.Radius * OccludingSphere.Radius;
    for (int i = 0; i < NUM_PEEKS; i++)
    {
        FVector PlayerToSphere = SphereCenter - Peeks[i];
        const TArray<FVector>* Vertices;
        if (i < 2)
        {
            Vertices = &EnemyBounds.TopVertices;
        }
        else
        {
            Vertices = &EnemyBounds.BottomVertices;
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

// Gets all cuboids that could occlude th bundle.
// Search through the cuboid bounding volume hierarchy.
std::vector<const Cuboid*>
ACullingController::GetPossibleOccludingCuboids(const Bundle& B)
{
    // Functional sometimes. Nondeterministically fails to return all occluders.
    return CuboidTraverser.get()->traverse(
        OptSegment(
            Bounds[B.PlayerI].CameraLocation,
            Bounds[B.EnemyI].Center));
}

// Increments visibility timers of bundles that were not culled,
// and reveals enemies with positive visibility timers.
void ACullingController::UpdateVisibility()
{
	for (Bundle B : BundleQueue)
    {
        VisibilityTimers[B.PlayerI][B.EnemyI] = TimerIncrement;
	}
	for (int i = 0; i < Characters.Num(); i++)
    {
		if (IsAlive[i])
        {
			for (int j = 0; j < Characters.Num(); j++)
            {
				if (IsAlive[j] && (VisibilityTimers[i][j] > 0))
                {
					SendLocation(i, j);
				}
			}
		}
	}
}

// Draws a line from character i to j, simulating the sending of a location.
// TODO:
//   This method is currently just a visualization placeholder,
//   so integrate server location-sending API when deploying to a game.
void ACullingController::SendLocation(int i, int j)
{
	// Only draw sight lines of team 0.
	if (Teams[i] == 0)
    {
		ConnectVectors(
			GetWorld(),
			Bounds[i].Center + FVector(0, 0, 10),
			Bounds[j].Center,
			false,
			0.015,
			3,
			FColor::Green);
	}
}

void ACullingController::Tick(float DeltaTime)
{
	TotalTicks++;
	BenchmarkCull();
}
