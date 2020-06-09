// Copyright Epic Games, Inc. All Rights Reserved.

#include "CornerCullingGameMode.h"
#include "CornerCullingHUD.h"
#include "CornerCullingCharacter.h"
#include "CullingBox.h"
#include "EngineUtils.h"
#include "Utils.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"
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
	count = 0;
	totaltime = 0;
}

// Reveal the enemy to the player.
// Should integrate with networking protocol.
void ACornerCullingGameMode::Reveal(ACornerCullingCharacter* Player, AEnemy* Enemy) {
	Enemy->SetVisible();
}

// Function for accurately and quickly computing LOS between all pairs of opponents.
// Can be sped up by building on top of PVS checks, or parallelizing checks.
void ACornerCullingGameMode::AngleCull() {
	for (ACornerCullingCharacter* Player : Players)
	{
		PlayerLocation3D = Player->GetCameraLocation();
		PlayerLocation = FVector2D(PlayerLocation3D);
		for (AEnemy* Enemy : Enemies)
		{
			Blocked = false;
			// Call PVS culling between player and enemy. Should be a big speedup.
			// if (!IsPotentiallyVisible(Enemy)) continue;
			// Note: Consider small +speed -accuracy tadeoff showing recently revealed enemies.
			EnemyCenter3D = Enemy->GetActorLocation();
			EnemyCenter = FVector2D(EnemyCenter3D);
			Enemy->GetRelevantCorners(PlayerLocation, EnemyCenter, EnemyLeft, EnemyRight);
			PlayerToEnemy = EnemyCenter - PlayerLocation;
			PlayerToEnemyLeft = EnemyLeft - PlayerLocation;
			PlayerToEnemyRight = EnemyRight - PlayerLocation;

			// NOTE: Could precompute relevant boxes per PVS region or pair of regions.
			for (ACullingBox* Box : Boxes) {
				// Rough solution for considering Z axis.
				if ((PlayerLocation3D.Z > Box->ZTop) || (EnemyCenter3D.Z > Box->ZTop)) {
					continue;
				}
				// Set pointers to the left and right corners
				// Get vectors used to determine if the corner is between player and enemy.
				BoxCenter = FVector2D(Box->GetActorLocation());
				Box->GetRelevantCorners(PlayerLocation, BoxCenter, BoxLeft, BoxRight);
				PlayerToBox = BoxCenter - PlayerLocation;
				PlayerToBoxLeft = BoxLeft - PlayerLocation;
				EnemyLeftToBoxLeft = BoxLeft - EnemyLeft;
				BoxRightToBoxLeft = BoxLeft - BoxRight;
				// Use cross products to determine player and enemy are on opposite sides of the box.
				Cross1Z = FVector2D::CrossProduct(PlayerToBoxLeft, BoxRightToBoxLeft);
				Cross2Z = FVector2D::CrossProduct(EnemyLeftToBoxLeft, BoxRightToBoxLeft);
				// If signs differ, player and enemy are on opposite sides.

				if ((Cross1Z > 0) ^ (Cross2Z > 0)) {
					PlayerToBoxRight = BoxRight - PlayerLocation;
					EnemyRightToBoxRight = BoxRight - EnemyRight;
					AngleLeft = Utils::GetAngle(PlayerToEnemyLeft, PlayerToBoxLeft);
					AngleRight = Utils::GetAngle(PlayerToEnemyRight, PlayerToBoxRight);
					// The enemy is entirely between both sides of the box.
					if ((AngleLeft < -CullingThreshold) && (AngleRight > CullingThreshold)) {
						Blocked = true;
						break;
					}
				}
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
	for (ACornerCullingCharacter* Player : Players)
	{
		PlayerLocation3D = Player->GetCameraLocation();
		PlayerLocation = FVector2D(PlayerLocation3D);
		for (AEnemy* Enemy : Enemies)
		{
			Blocked = false;
			EnemyCenter3D = Enemy->GetActorLocation();
			EnemyCenter = FVector2D(EnemyCenter3D);
			Enemy->GetRelevantCorners(PlayerLocation, EnemyCenter, EnemyLeft, EnemyRight);
			PlayerToEnemy = EnemyCenter - PlayerLocation;
			PlayerToEnemyLeft = EnemyLeft - PlayerLocation;
			PlayerToEnemyRight = EnemyRight - PlayerLocation;

				//FVector start = PlayerLocation3D + FVector(0, 0, -15);
				//FVector end = PlayerLocation3D + FVector(PlayerToEnemyLeft.X, PlayerToEnemyLeft.Y, 0);
				//DrawDebugLine(GetWorld(), start, end, FColor::Red, false, 0.1f, 0, 0.2f);
				//end = PlayerLocation3D + FVector(PlayerToEnemyRight.X, PlayerToEnemyRight.Y, 0);
				//DrawDebugLine(GetWorld(), start, end, FColor::Red, false, 0.1f, 0, 0.2f);

			for (ACullingBox* Box : Boxes) {
				if ((PlayerLocation3D.Z > Box->ZTop) || (EnemyCenter3D.Z > Box->ZTop)) {
					continue;
				}
				// Set pointers to the left and right corners
				BoxCenter = FVector2D(Box->GetActorLocation());
				Box->GetRelevantCorners(PlayerLocation, BoxCenter, BoxLeft, BoxRight);

				BoxRightToBoxLeft = BoxLeft - BoxRight;
				PlayerToBoxRight = BoxRight - PlayerLocation;
				PlayerToBoxLeft = BoxLeft - PlayerLocation;
				EnemyRightToBoxRight = BoxRight - EnemyRight;
				EnemyLeftToBoxLeft = BoxLeft - EnemyLeft;
				// PlayerToEnemyLeft and PlayerToEnemyRight are both blocked by the box, so the enemy is hidden.
				if (Utils::CheckSegmentsIntersect(PlayerLocation, PlayerToEnemyRight, BoxRight, BoxRightToBoxLeft)
					&& Utils::CheckSegmentsIntersect(PlayerLocation, PlayerToEnemyLeft, BoxRight, BoxRightToBoxLeft)) {
					Blocked = true;
					break;
				}
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

void ACornerCullingGameMode::CornerCull() {
	AngleCull();
	for (AEnemy* Enemy : Enemies) { Enemy->ClearCache(); }
	for (ACullingBox* Box : Boxes) { Box->ClearCache(); }

	// Benchmark
	auto start = std::chrono::high_resolution_clock::now();
	// Benchmark

	for (AEnemy* Enemy : Enemies) { Enemy->UpdateBounds(); }

	// Benchmark
	auto stop = std::chrono::high_resolution_clock::now();
	double delta = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
	count++;
	totaltime += delta;
	if (GEngine && (count % 60 == 0)) {
		FString AverageMessage = "Average time to cull (microseconds): " + FString::SanitizeFloat(totaltime / count);
		FString DeltaMessage = "This tick time to cull (microseconds): " + FString::SanitizeFloat(delta);
		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, AverageMessage, true, FVector2D(1.5f, 1.5f));
		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, DeltaMessage, true, FVector2D(1.5f, 1.5f));
	}
	// Benchmark End
}

void ACornerCullingGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	CornerCull();
}
