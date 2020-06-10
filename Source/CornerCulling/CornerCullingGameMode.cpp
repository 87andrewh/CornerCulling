// Copyright Epic Games, Inc. All Rights Reserved.

#include "CornerCullingGameMode.h"
#include "CornerCullingHUD.h"
#include "CornerCullingCharacter.h"
#include "CullingBox.h"
#include "EngineUtils.h"
#include "Utils.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"
#include <stdlib.h>
#include <cmath>
#include <chrono> 
#include <Runtime\Engine\Classes\Kismet\GameplayStatics.h>

ACornerCullingGameMode::ACornerCullingGameMode()
	: Super()
{
	// Enable culling every tick.
	PrimaryActorTick.bCanEverTick = true;

	// Set default pawn class to our Blueprinted character.
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// Use our custom HUD class.
	HUDClass = ACornerCullingHUD::StaticClass();
}

void ACornerCullingGameMode::BeginPlay() {
	// Sort and list actors so we that can iterate over them.
    for (ACornerCullingCharacter* Player : TActorRange<ACornerCullingCharacter>(GetWorld()))
    {
          Players.Add(Player);
    }
    for (AEnemy* Enemy : TActorRange<AEnemy>(GetWorld()))
    {
          Enemies.Add(Enemy);
    }
    for (ACullingBox* Box : TActorRange<ACullingBox>(GetWorld()))
    {
		Boxes.Add(Box);
		IndexInCache.push_back(false);
    }
	TotalTicks = 0;
	TotalTime = 0;
}

// Put index i in the cache, possibly evicting another member.
// Currently, FIFO seems to be good enough, but I think you could get perormance
// with a CLOCK, RANDOM, or LRU.
void ACornerCullingGameMode::PutInCache(int i) {
	BoxIndexCache.push_front(i);
	IndexInCache[i] = true;
	if (BoxIndexCache.size() > MaxCacheSize) {
		IndexInCache[BoxIndexCache.back()] = false;
		BoxIndexCache.pop_back();
	}
}

void ACornerCullingGameMode::MarkFVector(const FVector2D& V) {
	FVector start = FVector(V.X, V.Y, 200);
	FVector end = start + FVector(0, 0, 200);
	DrawDebugLine(GetWorld(), start, end, FColor::Emerald, false, 0.3f, 0, 0.2f);
}

void ACornerCullingGameMode::ConnectVectors(const FVector2D& V1, const FVector2D& V2) {
	DrawDebugLine(GetWorld(), FVector(V1.X, V1.Y, 260), FVector(V2.X, V2.Y, 260), FColor::Emerald, false, 0.3f, 0, 0.2f);
}

// Reveal the enemy to the player.
// Should integrate with networking protocol.
void ACornerCullingGameMode::Reveal(ACornerCullingCharacter* Player, AEnemy* Enemy) {
	Enemy->Reveal();
}

// Check if the VisiblePrism is blocking line of sight
// between both P1LeftToP2Left and P1RightToP2Right.
bool ACornerCullingGameMode::IsBlocking(
	const FVector2D& P1Left,
	const FVector2D& P1Right,
	const FVector2D& P2Left,
	const FVector2D& P2Right,
	const float P1Z,
	const float P2Z,
	VisiblePrism* Prism)
{
	if ((P1Z > Prism->ZTop) || (P2Z > Prism->ZTop)) {
		return false;
	}
	FVector2D P3, P4;
	Prism->GetRelevantCorners(P1Left, P3, P4);
	if (IsBlocking(P1Left, P2Left, P3, P4)) {
		Prism->GetRelevantCorners(P1Right, P3, P4);
		return IsBlocking(P1Right, P2Right, P3, P4);
	}
	return false;
}
  
// Check if line segment between points P3 and P4 blocks line of sight between P1 and P2.
// P3 is on the left of P4.
bool ACornerCullingGameMode::IsBlocking(
	const FVector2D& P1,
	const FVector2D& P2,
	const FVector2D& P3,
	const FVector2D& P4)
{
	#if defined ( LINE_SEGMENT_MODE )
		// Define line segments between PlayerLeft and EnemyLeft and between the two
		// relevant corners of an occluding object. Cull if those segments intersect.
		return Utils::CheckSegmentsIntersect(P1, P2, P3, P4);
	#else
		FVector2D P1ToP3 = P3 - P1;
		FVector2D P2ToP3 = P3 - P2;
		FVector2D P4ToP3 = P3 - P4;
		// Use sign of cross products to determine if P1 and P2 are on opposite sides of P3ToP4
		if (Utils::CrossProductPositive(P1ToP3, P4ToP3)
			== Utils::CrossProductPositive(P2ToP3, P4ToP3)) 
		{
			// Same signs, same side. Not blocking.
			return false;
		}
		FVector2D P1ToP2 = P2 - P1;
		FVector2D P1ToP4 = P4 - P1;
		// Use the sign of cross products to determine if P1ToP2 is between P1ToP3 and P1ToP4
		if (Utils::CrossProductPositive(P1ToP2, P1ToP3)
			|| !Utils::CrossProductPositive(P1ToP2, P1ToP4))
		{
			// P1ToP2 is either to the left of P1ToP3 or the right of P1ToP4
			return false;
		}
			return true;
	#endif
}

// Function for accurately and quickly computing LOS between all pairs of opponents.
// Can be sped up by building on top of PVS checks, or parallelizing checks.
void ACornerCullingGameMode::Cull() {
	int NumBoxes = Boxes.Num();
	for (ACornerCullingCharacter* Player : Players)
	{
		FVector PlayerCenter3D = Player->GetCameraLocation();
		FVector2D PlayerCenter = FVector2D(PlayerCenter3D);
		for (AEnemy* Enemy : Enemies)
		{
			// If the enemy is almost out of linger visibility, we check LOS
			// to prevent flickering. Otherwise, if the enemy still has lingering visibililty,
			// or this current tick is not scheduled to cull, then we can skip this enemy.
			// There is a weird interaction if enemy visibility period and culling period are correlated,
			// but enemy visibility should be random enough to avoid that.
			if (!(Enemy->IsAlmostVisible()) && (Enemy->IsVisible() || ((TotalTicks % CullingPeriod) != 0))) {
				// Skip culling this enemy;
				continue;
			}
			// Call PVS culling between player and enemy. Could be a big speedup.
			// if (!IsPotentiallyVisible(Enemy)) continue;

			FVector EnemyCenter3D = Enemy->GetCenter();
			FVector2D EnemyCenter = FVector2D(EnemyCenter3D);
			FVector2D EnemyLeft, EnemyRight;
			Enemy->GetRelevantCorners(PlayerCenter, EnemyLeft, EnemyRight);
			FVector2D PlayerToEnemy = EnemyCenter - PlayerCenter;
			FVector2D PlayerLeft, PlayerRight, PlayerPerpendicularDisplacement;
			Player->GetPerpendicularDisplacement(PlayerToEnemy, PlayerPerpendicularDisplacement);
			PlayerLeft = PlayerCenter - PlayerPerpendicularDisplacement;
			PlayerRight = PlayerCenter + PlayerPerpendicularDisplacement;

			// Check cache first.
			for (int i : BoxIndexCache) {
				ACullingBox* Box = Boxes[i];
				if (IsBlocking(PlayerLeft, PlayerRight, EnemyLeft, EnemyRight,
						       PlayerCenter3D.Z, EnemyCenter3D.Z, Box))
				{
					goto DONE;
				}
			}
			// Check everything else.
			for (int i = 0; i < NumBoxes; i++) {
				// Current i was in cache. Skip.
				if (IndexInCache[i]) continue;
				ACullingBox* Box = Boxes[i];
				// Call PVS culling between player and box. Could be a big speedup.
				// if (!IsPotentiallyVisible(Box)) continue;
				if (IsBlocking(PlayerLeft, PlayerRight, EnemyLeft, EnemyRight,
						       PlayerCenter3D.Z, EnemyCenter3D.Z, Box))
				{	
					PutInCache(i);
					goto DONE;
				}
			}
			// No occluding object blocked LOS.
			Reveal(Player, Enemy);
			DONE:;
		}
	}
}

void ACornerCullingGameMode::BenchmarkCull() {
	auto Start = std::chrono::high_resolution_clock::now();
	TotalTicks++;
	Cull();
	// This just simulates the cost of updating character bounds in a 5v5 game.
	for (int i = 0; i < 10; i++) { Enemies[0]->UpdateBounds(); }
	auto Stop = std::chrono::high_resolution_clock::now();
	int Delta = std::chrono::duration_cast<std::chrono::microseconds>(Stop - Start).count();
	TotalTime += Delta;
	RollingTotalTime += Delta;
	RollingMaxTime = std::max(RollingMaxTime, Delta);
	if ((TotalTicks % RollingLength) == 0) {
		RollingAverageTime = RollingTotalTime / RollingLength;
		RollingTotalTime = 0;
		RollingMaxTime = 0;
	}
	if (GEngine && (TotalTicks % 30 == 0)) {
		// Remember, 1 cull happens per culling period. Each cull takes period times as long as the average.
		// Make sure that multiple servers are staggered so these spikes do not add up.
		FString Msg = "Average time to cull (microseconds): " + FString::SanitizeFloat(TotalTime / TotalTicks);
		GEngine->AddOnScreenDebugMessage(1, 0.25f, FColor::Yellow, Msg, true, FVector2D(1.5f, 1.5f));
		Msg = "Rolling average time to cull (microseconds): " + FString::SanitizeFloat(RollingAverageTime);
		GEngine->AddOnScreenDebugMessage(2, 0.25f, FColor::Yellow, Msg, true, FVector2D(1.5f, 1.5f));
		Msg = "Rolling max time to cull (microseconds): " + FString::FromInt(RollingMaxTime);
		GEngine->AddOnScreenDebugMessage(3, 0.25f, FColor::Yellow, Msg, true, FVector2D(1.5f, 1.5f));
	}
}

void ACornerCullingGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	BenchmarkCull();
}
