// Fill out your copyright notice in the Description page of Project Settings.

#include "CullingController.h"
#include "CornerCullingCharacter.h"
#include "Occluder.h"
#include "EngineUtils.h" // TActorRange
#include <chrono> 

ACullingController::ACullingController()
	: Super()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
}

void ACullingController::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

    for (ACornerCullingCharacter* Player : TActorRange<ACornerCullingCharacter>(GetWorld()))
    {
		Characters.Emplace(Player);
		IsAlive.Emplace(true);
    }
	// Acquire the prisms of occluding objects.
    for (AOccluder* Occluder : TActorRange<AOccluder>(GetWorld()))
    {
		// Try profiling emplace.
		Cuboids.Emplace(Cuboid(Occluder->Vectors));
    }
}

void ACullingController::UpdateCharacterBounds()
{
	Bounds.Reset(Characters.Num());
	for (int i = 0; i < Characters.Num(); i++) {
		if (IsAlive[i]) {
			Bounds.Emplace(CharacterBounds(Characters[i]->GetActorTransform()));
		}
	}
}

void ACullingController::PopulateBundles()
{
	BundleQueue.Reset();
	for (int i = 0; i < Characters.Num(); i++) {
		// TESTING
		if (i == 0) { continue; }
		// END TESTING
		if (IsAlive[i]) {
			int TeamI = Characters[i]->Team;
			for (int j = 0; j < Characters.Num(); j++) {
				if (IsAlive[j] && (TeamI != Characters[j]->Team) && (VisibilityTimers[i][j] == 0)) {
					BundleQueue.Emplace(Bundle(i, j));
				}
			}
		}
	}
}

void ACullingController::CullWithCache()
{
	BundleQueue2.Reset();
	for (Bundle B : BundleQueue) {
		bool Blocked = false;
		for (int k = 0; k < CUBOID_CACHE_SIZE; k++) {
			if (IsBlocking(B, Cuboids[CuboidCaches[B.PlayerI][B.EnemyI][k]])) {
				Blocked = true;
				CacheTimers[B.PlayerI][B.EnemyI][k] = TotalTicks;
			}
		}
		if (!Blocked) {
			// Try emplace later.
			BundleQueue2.Add(B);
		}
	}
}

void ACullingController::CullRemaining()
{
	for (Bundle B : BundleQueue2) {
		bool Blocked = false;
		for (int CuboidI : GetPossibleOccludingCuboids(B)) {
			if (IsBlocking(B, Cuboids[CuboidI])) {
				Blocked = true;
				int MinI = ArgMin(CacheTimers[B.PlayerI][B.EnemyI], CUBOID_CACHE_SIZE);
				CuboidCaches[B.PlayerI][B.EnemyI][MinI] = CuboidI;
				CacheTimers[B.PlayerI][B.EnemyI][MinI] = TotalTicks;
				break;
			}
		}
		if (!Blocked) {
			VisibilityTimers[B.PlayerI][B.EnemyI] += TimerIncrement;
		}
	}
}

void ACullingController::UpdateVisibility()
{
	for (int i = 0; i < Characters.Num(); i++) {
		if (IsAlive[i]) {
			for (int j = 0; j < Characters.Num(); j++) {
				if (IsAlive[j] && (VisibilityTimers[i][j] > 0)) {
					SendLocation(i, j);
					VisibilityTimers[i][j] -= 1;
				}
			}
		}
	}
}

bool ACullingController::IsBlocking(const Bundle& B, Cuboid& OccludingCuboid)
{
	bool blocked = false;
	// Faces of the cuboid that are between the player and enemy
	// and have a normal pointing toward the player--thus skipping back faces.
	TArray<Face> FacesBetween;
	FVector& PlayerCenter = Bounds[B.PlayerI].Center;
	FVector& EnemyCenter = Bounds[B.EnemyI].Center;
	float EnemyInnerRadius = Bounds[B.EnemyI].InnerRadius;
	float EnemyOuterRadius = Bounds[B.EnemyI].OuterRadius;
	for (int i = 0; i < CUBOID_F; i++) {
		Face& F = OccludingCuboid.Faces[i];
		FVector FaceV = OccludingCuboid.GetVertex(i, 0);
		FVector PlayerToFace = FaceV - PlayerCenter;
		FVector EnemyToFace = FaceV - EnemyCenter;
		if (
			(FVector::DotProduct(PlayerToFace, F.Normal) < 0) &&
			(FVector::DotProduct(EnemyToFace, F.Normal) > 0)
		) {
			// Try add later.
			FacesBetween.Emplace(F);
		}
	}
	if (FacesBetween.Num() > 0) {
		// Array of planes defined by the player center and a non-duplicate
		// edge from the perimeters of faces in FacesBetween.
		// By the magic of consistent hand convention, all duplicate,
		// interior edges (i, j) will have a pair of the form (j, i).
		TArray<FPlane> ShadowFrustumPlanes;
		for (Face& F : FacesBetween) {
			// Unrolled for speed. Could try optimizing further.
			EdgeSet[F.Perimeter[0]][F.Perimeter[1]] = true;
			EdgeSet[F.Perimeter[1]][F.Perimeter[2]] = true;
			EdgeSet[F.Perimeter[2]][F.Perimeter[3]] = true;
			EdgeSet[F.Perimeter[3]][F.Perimeter[0]] = true;
		}
		for (Face& F : FacesBetween) {
			// Indices of vertices of the face.
			int V0 = F.Perimeter[0];
			int V1 = F.Perimeter[1];
			int V2 = F.Perimeter[2];
			int V3 = F.Perimeter[3];
			// If the reverse edge is not present, then it boarders
			// the surface formed by combining all relevant faces.
			// Thus it defines a plane of the shadow frustum.
			if (!EdgeSet[V1][V0]) {
				FVector u = OccludingCuboid.Vertices[V0] + FVector(0, 0, 10);
				FVector v = OccludingCuboid.Vertices[V1] + FVector(0, 0, 10);
				DrawDebugDirectionalArrow(GetWorld(), u, v, 100.f, FColor::Red, false, 0.2f);
				ShadowFrustumPlanes.Emplace(FPlane(
					PlayerCenter,
					OccludingCuboid.Vertices[V0],
					OccludingCuboid.Vertices[V1]
				));
				FPlane P = FPlane(
					PlayerCenter,
					OccludingCuboid.Vertices[V0],
					OccludingCuboid.Vertices[V1]
				);
				FVector Start = (OccludingCuboid.Vertices[V0] + OccludingCuboid.Vertices[V1])/2;
				DrawDebugDirectionalArrow(GetWorld(), Start, Start + 100 * P, 100.f, FColor::Red, false, 0.2f);
			}
			if (!EdgeSet[V2][V1]) {
				FVector u = OccludingCuboid.Vertices[V1] + FVector(0, 0, 10);
				FVector v = OccludingCuboid.Vertices[V2] + FVector(0, 0, 10);
				DrawDebugDirectionalArrow(GetWorld(), u, v, 100.f, FColor::Red, false, 0.2f);
				ShadowFrustumPlanes.Emplace(FPlane(
					PlayerCenter,
					OccludingCuboid.Vertices[V1],
					OccludingCuboid.Vertices[V2]
				));
				FPlane P = FPlane(
					PlayerCenter,
					OccludingCuboid.Vertices[V1],
					OccludingCuboid.Vertices[V2]
				);
				FVector Start = (OccludingCuboid.Vertices[V1] + OccludingCuboid.Vertices[V2])/2;
				DrawDebugDirectionalArrow(GetWorld(), Start, Start + 100 * P, 100.f, FColor::Red, false, 0.2f);
			}
			if (!EdgeSet[V3][V2]) {
				FVector u = OccludingCuboid.Vertices[V2] + FVector(0, 0, 10);
				FVector v = OccludingCuboid.Vertices[V3] + FVector(0, 0, 10);
				DrawDebugDirectionalArrow(GetWorld(), u, v, 100.f, FColor::Red, false, 0.2f);
				ShadowFrustumPlanes.Emplace(FPlane(
					PlayerCenter,
					OccludingCuboid.Vertices[V2],
					OccludingCuboid.Vertices[V3]
				));
				FPlane P = FPlane(
					PlayerCenter,
					OccludingCuboid.Vertices[V2],
					OccludingCuboid.Vertices[V3]
				);
				FVector Start = (OccludingCuboid.Vertices[V2] + OccludingCuboid.Vertices[V3])/2;
				DrawDebugDirectionalArrow(GetWorld(), Start, Start + 100 * P, 100.f, FColor::Red, false, 0.2f);
			}
			if (!EdgeSet[V0][V3]) {
				FVector u = OccludingCuboid.Vertices[V3] + FVector(0, 0, 10);
				FVector v = OccludingCuboid.Vertices[V0] + FVector(0, 0, 10);
				DrawDebugDirectionalArrow(GetWorld(), u, v, 100.f, FColor::Red, false, 0.2f);
				ShadowFrustumPlanes.Emplace(FPlane(
					PlayerCenter,
					OccludingCuboid.Vertices[V3],
					OccludingCuboid.Vertices[V0]
				));
				FPlane P = FPlane(
					PlayerCenter,
					OccludingCuboid.Vertices[V3],
					OccludingCuboid.Vertices[V0]
				);
				FVector Start = (OccludingCuboid.Vertices[V0] + OccludingCuboid.Vertices[V3])/2;
				DrawDebugDirectionalArrow(GetWorld(), Start, Start + 100 * P, 100.f, FColor::Red, false, 0.2f);
			}
		}
		int i = 10;
		//GEngine->AddOnScreenDebugMessage(9, 0.25f, FColor::Yellow, FString::FromInt(ShadowFrustumPlanes.Num()), true, FVector2D(1.5f, 1.5f));
		for (FPlane& P : ShadowFrustumPlanes) {
			// Signed distance from enemy to plane. The direction of the plane's normal vector is positive.
			float EnemyDistanceToPlane = -P.PlaneDot(EnemyCenter);
			GEngine->AddOnScreenDebugMessage(i++, 0.25f, FColor::Yellow, FString::SanitizeFloat(EnemyDistanceToPlane), true, FVector2D(1.5f, 1.5f));
			if (EnemyDistanceToPlane > EnemyOuterRadius) {
				continue;
			} else if (EnemyDistanceToPlane < EnemyInnerRadius) {
				blocked = false;
			} else {
				GEngine->AddOnScreenDebugMessage(6, 0.25f, FColor::Yellow, "REEEE", true, FVector2D(1.5f, 1.5f));
			}
		}
		blocked = true;
		// Reset the edge set.
		memset(EdgeSet, false, 64);
	}
	return blocked;
}

// Get all indices of cuboids that could block LOS between the player and enemy in the bundle.
// Currently just returns all cuboids. I'll implement a bounding volume hierarchy soon.
TArray<int> ACullingController::GetPossibleOccludingCuboids(Bundle B) {
	TArray<int> Possible;
	for (int i = 0; i < Cuboids.Num(); i++) {
		Possible.Emplace(i);
	}
	return Possible;
}

// Send location of character j to character i.
// Currently draw a line from character i to j.
void ACullingController::SendLocation(int i, int j)
{
	ConnectVectors(
		GetWorld(),
		Bounds[i].Center + FVector(0, 0, -20),
		Bounds[j].Center + FVector(0, 0, -10)
	);
}


void ACullingController::Tick(float DeltaTime)
{
	TotalTicks++;
	BenchmarkCull();
}

// Function for accurately and quickly computing LOS between all pairs of opponents.
// Can be sped up by building on top of PVS checks, or parallelizing checks.
void ACullingController::Cull() {
	UpdateCharacterBounds();
	PopulateBundles();
	CullWithCache();
	CullRemaining();
	UpdateVisibility();
}

void ACullingController::BenchmarkCull() {
	auto Start = std::chrono::high_resolution_clock::now();
	Cull();
	auto Stop = std::chrono::high_resolution_clock::now();
	DrawDebugSphere(GetWorld(), Bounds[0].Center, Bounds[0].OuterRadius, 20, FColor::Red, false, 0.f);
	int Delta = std::chrono::duration_cast<std::chrono::microseconds>(Stop - Start).count();
	TotalTime += Delta;
	RollingTotalTime += Delta;
	RollingMaxTime = std::max(RollingMaxTime, Delta);
	if ((TotalTicks % RollingLength) == 0) {
		RollingAverageTime = RollingTotalTime / RollingLength;
		RollingTotalTime = 0;
		RollingMaxTime = 0;
	}
	if (GEngine && (TotalTicks - 1) % RollingLength == 0) {
		// Remember, 1 cull happens per culling period. Each cull takes period times as long as the average.
		// Make sure that multiple servers are staggered so these spikes do not add up.
		FString Msg = "Average time to cull (microseconds): " + FString::FromInt(int(TotalTime / TotalTicks));
		GEngine->AddOnScreenDebugMessage(1, 0.25f, FColor::Yellow, Msg, true, FVector2D(1.5f, 1.5f));
		Msg = "Rolling average time to cull (microseconds): " + FString::FromInt(int(RollingAverageTime));
		GEngine->AddOnScreenDebugMessage(2, 0.25f, FColor::Yellow, Msg, true, FVector2D(1.5f, 1.5f));
		Msg = "Rolling max time to cull (microseconds): " + FString::FromInt(RollingMaxTime);
		GEngine->AddOnScreenDebugMessage(3, 0.25f, FColor::Yellow, Msg, true, FVector2D(1.5f, 1.5f));
	}
}

