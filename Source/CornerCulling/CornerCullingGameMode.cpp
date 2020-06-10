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
    }
	TotalTicks = 0;
	TotalTime = 0;
}

// Reveal the enemy to the player.
// Should integrate with networking protocol.
void ACornerCullingGameMode::Reveal(ACornerCullingCharacter* Player, AEnemy* Enemy) {
	Enemy->Reveal();
}

// Function for accurately and quickly computing LOS between all pairs of opponents.
// Can be sped up by building on top of PVS checks, or parallelizing checks.
void ACornerCullingGameMode::Cull() {
	int NumBoxes = Boxes.Num();
	for (ACornerCullingCharacter* Player : Players)
	{
		PlayerCenter3D = Player->GetCameraLocation();
		PlayerCenter = FVector2D(PlayerCenter3D);
		for (AEnemy* Enemy : Enemies)
		{
			if (!(Enemy->IsAlmostVisible()) && (Enemy->IsVisible() || ((TotalTicks % CullingPeriod) != 0))) {
				continue;
			}
			Blocked = false;
			// Call PVS culling between player and enemy. Should be a big speedup.
			// if (!IsPotentiallyVisible(Enemy)) continue;
			EnemyCenter3D = Enemy->GetActorLocation();
			EnemyCenter = FVector2D(EnemyCenter3D);
			Enemy->GetRelevantCorners(PlayerCenter, EnemyCenter, EnemyLeft, EnemyRight);
			PlayerToEnemy = EnemyCenter - PlayerCenter;
			Player->GetPerpendicularDisplacement(PlayerToEnemy, PlayerPerpendicularDisplacement);
			PlayerLeft = PlayerCenter - PlayerPerpendicularDisplacement;
			PlayerRight = PlayerCenter + PlayerPerpendicularDisplacement;
			PlayerLeftToEnemyLeft = EnemyLeft - PlayerLeft;
			PlayerRightToEnemyRight = EnemyRight - PlayerRight;

			// NOTE: Could precompute relevant boxes with PVS
			for (box_i = 0; box_i < NumBoxes; box_i++) {
				ACullingBox* Box = Boxes[(box_i + RandomOffset) % NumBoxes];
				// Rough solution for considering Z axis.
				if ((PlayerCenter3D.Z > Box->ZTop) || (EnemyCenter3D.Z > Box->ZTop)) {
					continue;
				}
				// Get vectors used to determine if the corner is between player and enemy.
				BoxCenter = FVector2D(Box->GetActorLocation());

				#if defined ( LINE_SEGMENT_MODE )
					// Define line segments between PlayerLeft and EnemyLeft and between the two relevant corners of an occluding object.
					// Cull if those segments intersect.
					Box->GetRelevantCorners(PlayerLeft, BoxCenter, BoxLeft, BoxRight);
					BoxRightToBoxLeft = BoxLeft - BoxRight;
					FVector start;
					FVector end;
					start = PlayerCenter3D + FVector(0, 0, -10);
					//DrawDebugLine(GetWorld(), start, end. FColor::Red, false, 0.3f, 0, 0.2f);
					if (!Utils::CheckSegmentsIntersect(PlayerLeft, PlayerLeftToEnemyLeft, BoxRight, BoxRightToBoxLeft)) {
						continue;
					}
					Box->GetRelevantCorners(PlayerRight, BoxCenter, BoxLeft, BoxRight);
					BoxRightToBoxLeft = BoxLeft - BoxRight;
					if (!Utils::CheckSegmentsIntersect(PlayerRight, PlayerRightToEnemyRight, BoxRight, BoxRightToBoxLeft))
					{
						continue;
					}
					Blocked = true;
					break;
				#else
					PlayerLeftToBoxLeft = BoxLeft - PlayerLeft;
					EnemyLeftToBoxLeft = BoxLeft - EnemyLeft;
					// Use cross products to determine if player and enemy are on opposite sides of the box.
					// If signs are the same, player and enemy are on the same side of the wall.
					if (Utils::CrossProductPositive(PlayerLeftToBoxLeft, BoxRightToBoxLeft)
						== Utils::CrossProductPositive(EnemyLeftToBoxLeft, BoxRightToBoxLeft)) 
					{
						continue;
					}

					PlayerRightToBoxRight = BoxRight - PlayerCenter;
					EnemyRightToBoxRight = BoxRight - EnemyRight;
					// Calculate the sign of the cross product between PlayerToEnemyRight and PlayerToBoxRight
					// to determine if the right of the enemy peeks out from the right of the box. Same for the left.
					// If the signs confuse you, fellow math nerd, it's because UE4 uses left-handed coordinates
					// due to historical reasons.
					if (Utils::CrossProductPositive(PlayerLeftToEnemyLeft, PlayerLeftToBoxLeft)
						|| !Utils::CrossProductPositive(PlayerRightToEnemyRight, PlayerRightToBoxRight))
					{
						continue;
					}
					Blocked = true;
					break;
				#endif
			}
			if (!Blocked) {
				Reveal(Player, Enemy);
			}
		}
	}
}

void ACornerCullingGameMode::BenchmarkCull() {
	TotalTicks++;
	auto Start = std::chrono::high_resolution_clock::now();
	Cull();
	auto Stop = std::chrono::high_resolution_clock::now();
	int Delta = std::chrono::duration_cast<std::chrono::microseconds>(Stop - Start).count();
	RollingTotalTime += Delta;

	if ((TotalTicks % RollingLength) == 0) {
		RollingAverageTime = RollingTotalTime / RollingLength;
		// If we are having bad frame times, permute the order in which we iterate through occluding objects.
		// This optimization might end up being unnecessary in a deployed setting using PVS.
		// The logical extension of this idea is to iterate through occluding objects
		// in the order that they were most recently used.
		if (RollingAverageTime > RandomizationThreshold) {
			RandomOffset = rand() % Boxes.Num();
		}
		RollingTotalTime = 0;
	}
	// This just simulates the cost of updating character bounds in a 5v5 game.
	for (int i = 0; i < 10; i++) { Enemies[0]->UpdateBounds(); }
	// There are two deltas because the first one is part of the algorithm,
	// and the second one benchmakrs the entire algorithm.
	Stop = std::chrono::high_resolution_clock::now();
	Delta = std::chrono::duration_cast<std::chrono::microseconds>(Stop - Start).count();
	TotalTime += Delta;
	if (GEngine && (TotalTicks % 30 == 0)) {
		FString Msg = "Average time to cull (microseconds): " + FString::SanitizeFloat(TotalTime / TotalTicks);
		GEngine->AddOnScreenDebugMessage(-1, 0.25f, FColor::Yellow, Msg, true, FVector2D(1.5f, 1.5f));
		Msg = "Rolling average time to cull (microseconds): " + FString::SanitizeFloat(RollingAverageTime);
		GEngine->AddOnScreenDebugMessage(-1, 0.25f, FColor::Yellow, Msg, true, FVector2D(1.5f, 1.5f));
		Msg = "Breaking at box: " + FString::FromInt(box_i);
		GEngine->AddOnScreenDebugMessage(-1, 0.25f, FColor::Yellow, Msg, true, FVector2D(1.5f, 1.5f));
	}
}

void ACornerCullingGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	BenchmarkCull();
}
