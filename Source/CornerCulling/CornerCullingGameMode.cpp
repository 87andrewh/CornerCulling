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
void ACornerCullingGameMode::AngleCull() {
	int NumBoxes = Boxes.Num();
	for (ACornerCullingCharacter* Player : Players)
	{
		PlayerLocation3D = Player->GetCameraLocation();
		PlayerLocation = FVector2D(PlayerLocation3D);
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
			Enemy->GetRelevantCorners(PlayerLocation, EnemyCenter, EnemyLeft, EnemyRight);
			PlayerToEnemy = EnemyCenter - PlayerLocation;
			PlayerToEnemyLeft = EnemyLeft - PlayerLocation;
			PlayerToEnemyRight = EnemyRight - PlayerLocation;

			// NOTE: Could precompute relevant boxes with PVS
			for (box_i = 0; box_i < NumBoxes; box_i++) {
				ACullingBox* Box = Boxes[(box_i + RandomOffset) % NumBoxes];
				// Rough solution for considering Z axis.
				if ((PlayerLocation3D.Z > Box->ZTop) || (EnemyCenter3D.Z > Box->ZTop)) {
					continue;
				}
				// Get vectors used to determine if the corner is between player and enemy.
				BoxCenter = FVector2D(Box->GetActorLocation());
				Box->GetRelevantCorners(PlayerLocation, BoxCenter, BoxLeft, BoxRight);
				PlayerToBoxLeft = BoxLeft - PlayerLocation;
				EnemyLeftToBoxLeft = BoxLeft - EnemyLeft;
				BoxRightToBoxLeft = BoxLeft - BoxRight;

				// Use cross products to determine player and enemy are on opposite sides of the box.
				Cross1Z = FVector2D::CrossProduct(PlayerToBoxLeft, BoxRightToBoxLeft);
				Cross2Z = FVector2D::CrossProduct(EnemyLeftToBoxLeft, BoxRightToBoxLeft);
				// If signs are the same, player and enemy are on the same side of the wall.
				if ((Cross1Z > 0) == (Cross2Z > 0)) {
					continue;
				}

				// Calculate the angles between BoxLeft and EnemyLeft to determine if the
				// right side of the enemy peeks out. Same for right angles.
				PlayerToBoxRight = BoxRight - PlayerLocation;
				EnemyRightToBoxRight = BoxRight - EnemyRight;
				AngleLeft = Utils::GetAngle(PlayerToEnemyLeft, PlayerToBoxLeft);
				AngleRight = Utils::GetAngle(PlayerToEnemyRight, PlayerToBoxRight);
				// The left or right side of the enemy peeks out from the respective side of the box.
				if ((AngleLeft > 0) || (AngleRight < 0)) {
					continue;
				}

				Blocked = true;
				break;
			}
			if (!Blocked) {
				Reveal(Player, Enemy);
			} else {
				// For demo purposes, remove in production.
				Enemy->SetInvisible();
			}
		}
	}
}

// Alternative function for quickly computing LOS between all pairs of opponents.
// Define the line segments between players and enemy and
// between the two relevant corners as parametric vectors.
// Cull if those vectors intersect.
void ACornerCullingGameMode::LineCull()
{
	int NumBoxes = Boxes.Num();
	for (ACornerCullingCharacter* Player : Players)
	{
		PlayerLocation3D = Player->GetCameraLocation();
		PlayerLocation = FVector2D(PlayerLocation3D);
		for (AEnemy* Enemy : Enemies)
		{
			if (!(Enemy->IsAlmostVisible()) && (Enemy->IsVisible() || ((TotalTicks % CullingPeriod) != 0))) {
				continue;
			}

			Blocked = false;
			EnemyCenter3D = Enemy->GetActorLocation();
			EnemyCenter = FVector2D(EnemyCenter3D);
			Enemy->GetRelevantCorners(PlayerLocation, EnemyCenter, EnemyLeft, EnemyRight);
			PlayerToEnemyLeft = EnemyLeft - PlayerLocation;
			PlayerToEnemyRight = EnemyRight - PlayerLocation;

				//FVector start = PlayerLocation3D + FVector(0, 0, -15);
				//FVector end = PlayerLocation3D + FVector(PlayerToEnemyLeft.X, PlayerToEnemyLeft.Y, 0);
				//DrawDebugLine(GetWorld(), start, end, FColor::Red, false, 0.1f, 0, 0.2f);
				//end = PlayerLocation3D + FVector(PlayerToEnemyRight.X, PlayerToEnemyRight.Y, 0);
				//DrawDebugLine(GetWorld(), start, end, FColor::Red, false, 0.1f, 0, 0.2f);

			for (box_i = 0; box_i < NumBoxes; box_i++) {
				ACullingBox* Box = Boxes[(box_i + RandomOffset) % NumBoxes];
				if ((PlayerLocation3D.Z > Box->ZTop) || (EnemyCenter3D.Z > Box->ZTop)) {
					continue;
				}
				// Set pointers to the left and right corners
				BoxCenter = FVector2D(Box->GetActorLocation());
				Box->GetRelevantCorners(PlayerLocation, BoxCenter, BoxLeft, BoxRight);

				BoxRightToBoxLeft = BoxLeft - BoxRight;
				// PlayerToEnemyLeft and PlayerToEnemyRight are both blocked by the box, so the enemy is hidden.
				if (Utils::CheckSegmentsIntersect(PlayerLocation, PlayerToEnemyRight, BoxRight, BoxRightToBoxLeft)
					&& Utils::CheckSegmentsIntersect(PlayerLocation, PlayerToEnemyLeft, BoxRight, BoxRightToBoxLeft)) {
					Blocked = true;
					break;
				}
			}
			if (!Blocked) {
				Reveal(Player, Enemy);
			}
		}
	}
}

void ACornerCullingGameMode::CornerCull() {
	auto Start = std::chrono::high_resolution_clock::now();
	TotalTicks++;
	for (AEnemy* Enemy : Enemies) { Enemy->ClearCache(); }
	for (ACullingBox* Box : Boxes) { Box->ClearCache(); }
	LineCull();
	auto Stop = std::chrono::high_resolution_clock::now();
	double Delta = std::chrono::duration_cast<std::chrono::microseconds>(Stop - Start).count();
	RollingTotalTime += Delta;

	if ((TotalTicks % RollingLength) == 0) {
		RollingAverageTime = RollingTotalTime / RollingLength;
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
	CornerCull();
}
