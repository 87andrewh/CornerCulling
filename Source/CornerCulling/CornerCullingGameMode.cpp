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
// Designed to prevent wallhacks in games with many players, high-tickrate, and slow movement.
// Currently only works for the vertical corners of cubes.
// Can be sped up by building on top of PVS checks.
// Note: If more even speed is desired, the algorithm is straightforward to parallelize
void ACornerCullingGameMode::CornerCull() {
	// Benchmark
	auto start = std::chrono::high_resolution_clock::now();
	// Benchmark

	FVector PlayerLocation;
	FVector EnemyLocation;
	FVector PlayerToEnemy;
	float EnemyDistance;
	// Pointers to left and right relevant corners
	FVector CornerLeft = FVector();
	FVector CornerRight = FVector();
	// Vectors from point A to B.
	FVector PlayerToCornerLeft;
	FVector PlayerToCornerRight;
	FVector PlayerToCenter;
	FVector EnemyToCornerLeft;
	FVector EnemyToCornerRight;
	FVector CornerLeftToCenter;
	FVector CornerRightToCenter;
	// Used to store Z componets of various cross products.
	// Used to determine relative positions and angles.
	float Cross1Z;
	float Cross2Z;
	// Enables accurate culling at all distances.
	float EnemyHalfAngularWidth;
	// Angle between PlayerToCenter and PlayerToCornerLeft/Right
	float AngleLeft;
	float AngleRight;
	// Angle between PlayerToCenter and PlayerToEnemy
	float AngleEnemy;
	// Tracks if LOS between player and enemy is blocked.
	bool Blocked;
	// Tracks if each corner is between the player and the enemy.
	bool CornerLeftBetween;
	bool CornerRightBetween;
	// Tracks if the enemy is peeking out of the left or right corners.
	bool PeekingLeft;
	bool PeekingRight;
	for (ACornerCullingCharacter* Player : Players)
	{
		PlayerLocation = Player->GetCameraLocation();
		for (AEnemy* Enemy : Enemies)
		{
			Blocked = false;
			// Call PVS culling between player and enemy. Should be a big speedup.
			// if (!IsPotentiallyVisible(Enemy)) continue;
			// Note: Consider small +speed -accuracy tadeoff showing recently revealed enemies.
			EnemyLocation = Enemy->GetActorLocation();
			PlayerToEnemy = EnemyLocation - PlayerLocation;
			EnemyDistance = PlayerToEnemy.Size2D();
			EnemyHalfAngularWidth = Enemy->GetHalfAngularWidth(PlayerToEnemy, EnemyDistance);
			PlayerToEnemy = PlayerToEnemy.GetSafeNormal2D(Utils::MIN_SAFE_LENGTH);

			// NOTE: Could precompute relevant boxes per PVS region or pair of regions.
			for (ACullingBox* Box : Boxes) {
				// Rough solution for considering Z axis.
				if (PlayerLocation.Z > Box->TopZ) {
					continue;
				}
				// Set pointers to the left and right corners
				Box->GetRelevantCorners(PlayerLocation, CornerLeft, CornerRight);
				// Get vectors used to determine if the corner is between player and enemy.
				PlayerToCornerLeft = CornerLeft - PlayerLocation;
				PlayerToCornerRight = CornerRight - PlayerLocation;
				EnemyToCornerLeft = CornerLeft - EnemyLocation;
				CornerLeftToCenter = Box->Center - CornerLeft;
				// Use cross products to determine if the left corner is between player and enemy.
				Cross1Z = PlayerToCornerLeft.X * CornerLeftToCenter.Y
						  - PlayerToCornerLeft.Y * CornerLeftToCenter.X;
				Cross2Z = EnemyToCornerLeft.X * CornerLeftToCenter.Y
						  - EnemyToCornerLeft.Y * CornerLeftToCenter.X;
				// If the signs differ, then the corner sits between the player and the enemy.
				CornerLeftBetween = (Cross1Z > 0) ^ (Cross2Z > 0);
				// Need to check the right corner.
				if (!CornerLeftBetween) {
					EnemyToCornerRight = CornerRight - EnemyLocation;
					CornerRightToCenter = Box->Center - CornerRight;
					Cross1Z = PlayerToCornerRight.X * CornerRightToCenter.Y
							  - PlayerToCornerRight.Y * CornerRightToCenter.X;
					Cross2Z = EnemyToCornerRight.X * CornerRightToCenter.Y
							  - EnemyToCornerRight.Y * CornerRightToCenter.X;
					CornerRightBetween = (Cross1Z > 0) ^ (Cross2Z > 0);
				}
				if (CornerLeftBetween || CornerRightBetween) {
					PlayerToCenter = Box->Center - PlayerLocation;
					AngleEnemy = Utils::GetAngle(PlayerToCenter, PlayerToEnemy);
					AngleLeft = Utils::GetAngle(PlayerToCenter, PlayerToCornerLeft);
					PeekingLeft = (AngleLeft > AngleEnemy - EnemyHalfAngularWidth);
					if (!PeekingLeft) {
						AngleRight = Utils::GetAngle(PlayerToCenter, PlayerToCornerRight);
						PeekingRight = (AngleRight < AngleEnemy + EnemyHalfAngularWidth);
						// Enemy is peeking neither left nor right. This box blocks LOS.
						if (!PeekingRight) {
							Blocked = true;
							break;
						}
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
	// Benchmark
	auto stop = std::chrono::high_resolution_clock::now();
	double delta = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
	count++;
	totaltime += delta;
	FString AverageMessage = "Average time to cull (microseconds): " + FString::SanitizeFloat(totaltime / count);
	FString DeltaMessage = "This tick time to cull (microseconds): " + FString::SanitizeFloat(delta);
	if (GEngine && (count % 60 == 0)) {
		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, AverageMessage, true, FVector2D(1.5f, 1.5f));
		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, DeltaMessage, true, FVector2D(1.5f, 1.5f));
	}
}

void ACornerCullingGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	CornerCull();
}
