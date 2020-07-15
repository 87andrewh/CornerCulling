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
    for (AOccludingCuboid* Occluder : TActorRange<AOccludingCuboid>(GetWorld()))
    {
		Cuboids.Add(Cuboid(Occluder->Vectors));
    }
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
	SendLocations();
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
			GEngine->AddOnScreenDebugMessage(1, 1.0f, FColor::Yellow, Msg, true, FVector2D(1.5f, 1.5f));
			Msg = "Rolling average time to cull (microseconds): " + FString::FromInt(int(RollingAverageTime));
			GEngine->AddOnScreenDebugMessage(2, 1.0f, FColor::Yellow, Msg, true, FVector2D(1.5f, 1.5f));
			Msg = "Rolling max time to cull (microseconds): " + FString::FromInt(RollingMaxTime);
			GEngine->AddOnScreenDebugMessage(3, 1.0f, FColor::Yellow, Msg, true, FVector2D(1.5f, 1.5f));
		}
		if (RollingMaxTime > TimerLoadThreshold)
        {
			TimerIncrement = MaxTimerIncrement;
		}
		else
        {
			TimerIncrement = MinTimerIncrement;
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
	// SendLocations();  // Moved to BenchmarkCull to not affect benchmarks.
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
                                20,  // Maximum horizontal displacement
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
	BundleQueue2.Reset();
	for (Bundle B : BundleQueue)
    {
		bool Blocked = false;
		for (int k = 0; k < CUBOID_CACHE_SIZE; k++)
        {
			if (IsBlocking(B, Cuboids[CuboidCaches[B.PlayerI][B.EnemyI][k]]))
            {
				Blocked = true;
				CacheTimers[B.PlayerI][B.EnemyI][k] = TotalTicks;
				break;
			}
		}
		if (!Blocked)
        {
			BundleQueue2.Emplace(B);
		}
	}
}

void ACullingController::CullWithSpheres()
{
	BundleQueue.Reset();
	for (Bundle B : BundleQueue2)
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
			BundleQueue.Emplace(B);
		}
	}
}

void ACullingController::CullWithCuboids()
{
	for (Bundle B : BundleQueue)
    {
		bool Blocked = false;
		for (int CuboidI : GetPossibleOccludingCuboids(B))
{
			if (IsBlocking(B, Cuboids[CuboidI]))
            {
				Blocked = true;
				int MinI =
                    ArgMin(CacheTimers[B.PlayerI][B.EnemyI], CUBOID_CACHE_SIZE);
				CuboidCaches[B.PlayerI][B.EnemyI][MinI] = CuboidI;
				CacheTimers[B.PlayerI][B.EnemyI][MinI] = TotalTicks;
				break;
			}
		}
		if (!Blocked)
        {
			// Random offset spreads out culling when all characters become
			// visible to each other at the same time, such as when walls fall.
			VisibilityTimers[B.PlayerI][B.EnemyI] = (
				TimerIncrement + (FMath::Rand() % 3)
			);
		}
	}
}

// Checks if the Cuboid blocks visibility between a bundle's player and enemy,
// returning true if and only if all lines of sights from all peeking positions
// are blocked.
bool ACullingController::IsBlocking(const Bundle& B, const Cuboid& OccludingCuboid)
{
    // Unpack constant variables outside of loop for performance.
    const CharacterBounds& EnemyBounds = Bounds[B.EnemyI];
    const FVector& EnemyCenter = EnemyBounds.Center;
    const TArray<FVector>& Peeks = B.PossiblePeeks;
    const float EnemyRadius = EnemyBounds.BoundingSphereRadius;
    // Shadow frustum for each possible peek.
    TArray<FPlane> ShadowFrustums[NUM_PEEKS] = { TArray<FPlane>() };
    for (int i = 0; i < NUM_PEEKS; i++)
    {
        // If cuboid does not block the bundle if it fails to block any peek.
        if (!Intersects(Peeks[i], EnemyCenter, OccludingCuboid))
        {
            return false;
        }
        // Get the faces of the cuboid that are visible to the player at
        // Peeks[i] and in between the player at Peeks[i] and the enemy.
        TArray<Face> FacesBetween = GetFacesBetween(
            Peeks[i], EnemyCenter, OccludingCuboid
        );
        if (FacesBetween.Num() > 0)
        {
            GetShadowFrustum(Peeks[i], OccludingCuboid, FacesBetween, ShadowFrustums[i]);
            // Planes of the shadow frustum that define a half space that
            // does not contain the enemy bounding sphere, and thus must
            // be checked against the enemy bounding box.
            TArray<FPlane> PlanesToCheck;
            // Try to determine visibility with quick bounding sphere checks.
            for (FPlane& P : ShadowFrustums[i])
            {
                // Signed distance from enemy to plane.
                // The direction of the plane's normal vector is negative.
                float EnemyDistanceToPlane = -P.PlaneDot(EnemyCenter);
                // The bounding sphere is not completely contained by the plane,
                // so we must check it against the bounding box.
                if (EnemyDistanceToPlane < EnemyRadius)
                {
                    PlanesToCheck.Emplace(P);
                }
            }
            // Check if all vertices of the enemy bounding box are in half spaces
            // defined by the planes that failed to contain the bounding sphere.
            // Because each bottom vertex is directly below a top vertex,
            // we do not need to check bottom vertices when peeking from above.
            // Likewise for top vertices.
            if (   i < 2
                && !InHalfSpaces(EnemyBounds.TopVertices, PlanesToCheck))
            {
                return false;
            }
            else if (   i >= 2
                     && !InHalfSpaces(EnemyBounds.BottomVertices, PlanesToCheck))
            {
                return false;
            }
            // There are no faces between the player and enemy.
            // Thus the cuboid cannot block LOS.
        }
        // No faces between the player and enemy. The cuboid cannot block LOS.
        else
        {
            return false;
        }
    }
	return true;
}

// Checks if a line segment intersects a cuboid.
// Implements Cyrus-Beck line clipping algorithm.
bool ACullingController::Intersects(
    const FVector& Start, const FVector& End, const Cuboid& C)
{
    float TimeEnter = 0;
    float TimeExit = 1;
    FVector StartToEnd = End - Start;
    for (int i = 0; i < CUBOID_F; i++)
    {
        // Numerator of a plane/line intersection test.
        const FVector& Normal = C.Faces[i].Normal;
        float Num = (Normal | (C.GetVertex(i, 0) - Start));
        float Denom = StartToEnd | Normal;
        if (Denom == 0)
        {
            // Start is outside of the plane,
            // so it cannot intersect the Cuboid.
            if (Num < 0)
            {
                return false;
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
                return false;
            }
        }
    }
    return true;
}

// Gets all faces between player and enemy that have a normal pointing
// toward the player, thus ignoring non-visible back faces.
TArray<Face> ACullingController::GetFacesBetween(
	const FVector& PlayerCameraLocation,
	const FVector& EnemyCenter,
	const Cuboid& OccludingCuboid)
{
    TArray<Face> FacesBetween;
	for (int i = 0; i < CUBOID_F; i++)
    {
		Face F = OccludingCuboid.Faces[i];
		FVector FaceV = OccludingCuboid.GetVertex(i, 0);
		FVector PlayerToFace = FaceV - PlayerCameraLocation;
		FVector EnemyToFace = FaceV - EnemyCenter;
		if (   ((PlayerToFace | F.Normal) < 0)
			&& ((EnemyToFace | F.Normal) > 0))
        {
			FacesBetween.Emplace(F);
		}
	}
    return FacesBetween;
}

// Gets the shadow frustum's planes, which are defined by three points:
// the player's camera location and the endpoints of an occluding, exterior edge
// of the occluding surface formed by all faces in FacesBetween.
// Edge (i, j) is an occluding, exterior edge if two conditions hold:
//   1) It is an edge of the perimeter of a face in FacesBetween
//   2) Edge (j, i) is not.
// This trick relies on fact that faces of a polyhedron have outward normals,
// and perimeter edges of faces wrap counter-clockwise by the right-hand rule.
// Thus, when two faces share an edge, that edge is included in the set of
// their edges as (i, j) from the left face and (j, i) from the right.
// Thus, interior edges of the occluding surface are identified and omitted.
void ACullingController::GetShadowFrustum(
	const FVector& PlayerCameraLocation,
	const Cuboid& OccludingCuboid,
	const TArray<Face>& FacesBetween,
	TArray<FPlane>& ShadowFrustum)
{
	// Reset the edge set.
	memset(EdgeSet, false, 64);
	// Add all perimeter edges of all faces to the EdgeSet.
	for (const Face& F : FacesBetween)
    {
		EdgeSet[F.Perimeter[0]][F.Perimeter[1]] = true;
		EdgeSet[F.Perimeter[1]][F.Perimeter[2]] = true;
		EdgeSet[F.Perimeter[2]][F.Perimeter[3]] = true;
		EdgeSet[F.Perimeter[3]][F.Perimeter[0]] = true;
	}
	// For all unpaired, occluding edges, create a corresponding
	// shadow frustum plane, and add it to the ShadowFrustum.
	for (const Face& F : FacesBetween)
    {
		// Indices of vertices that define the perimeter of the face.
		int V0 = F.Perimeter[0];
		int V1 = F.Perimeter[1];
		int V2 = F.Perimeter[2];
		int V3 = F.Perimeter[3];
		// If edge (j, i) is not present, create a plane with (i, j).
		if (!EdgeSet[V1][V0])
        {
			ShadowFrustum.Emplace(FPlane(
				PlayerCameraLocation,
				OccludingCuboid.Vertices[V0],
				OccludingCuboid.Vertices[V1]
			));
		}
		if (!EdgeSet[V2][V1])
        {
			ShadowFrustum.Emplace(FPlane(
				PlayerCameraLocation,
				OccludingCuboid.Vertices[V1],
				OccludingCuboid.Vertices[V2]
			));
		}
		if (!EdgeSet[V3][V2])
        {
			FVector u = OccludingCuboid.Vertices[V2] + FVector(0, 0, 10);
			ShadowFrustum.Emplace(FPlane(
				PlayerCameraLocation,
				OccludingCuboid.Vertices[V2],
				OccludingCuboid.Vertices[V3]
			));
		}
		if (!EdgeSet[V0][V3])
        {
			ShadowFrustum.Emplace(FPlane(
				PlayerCameraLocation,
				OccludingCuboid.Vertices[V3],
				OccludingCuboid.Vertices[V0]
			));
		}
	}
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


// For each plane, defines a half-space by the set of all points
// with a positive dot product with its normal vector.
// Check that every point is within all half-spaces.
bool ACullingController::InHalfSpaces(
	const TArray<FVector>& Points,
	const TArray<FPlane>& Planes)
{
	for (const FVector& Point : Points)
    {
		for (const FPlane& Plane : Planes)
        {
			// The point is on the outer side of the plane.
			if (Plane.PlaneDot(Point) > 0)
            {
				return false;
			}
		}
	}
	return true;
}

// Currently just returns all cuboids.
// TODO: Implement search through Bounding Volume Hierarchy.
TArray<int> ACullingController::GetPossibleOccludingCuboids(const Bundle& B)
{
	TArray<int> PossibleOccluders;
	for (int i = 0; i < Cuboids.Num(); i++)
    {
		PossibleOccluders.Emplace(i);
	}
	return PossibleOccluders;
}

void ACullingController::SendLocations()
{
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
// TODO: Integrate server location-sending API when deploying to a game.
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
