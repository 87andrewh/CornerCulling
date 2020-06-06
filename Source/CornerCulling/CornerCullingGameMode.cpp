// Copyright Epic Games, Inc. All Rights Reserved.

#include "CornerCullingGameMode.h"
#include "CornerCullingHUD.h"
#include "CornerCullingCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "CullingBox.h"
#include "EngineUtils.h"
#include <cmath>
#include "Utils.h"
#include "DrawDebugHelpers.h"
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
	TArray<FVector> CornerLocations;
	// Index of corners
	int CornerLeftI;
	int CornerRightI;
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
		PlayerLocation = Player->GetActorLocation();
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

			// NOTE: Could precompute relevant boxes per PVS region, or ordering boxes by relevance.
			// NOTE: Could try to cache relevant corners across enemies.
			for (ACullingBox* Box : Boxes) {
				CornerLocations = Box->CornerLocations;
				// Get get indicies of the leftmost and rightmost corners.
				Box->GetRelevantCorners(PlayerLocation, CornerLeftI, CornerRightI);
				// Get vectors used to determine if the corner is between player and enemy.
				PlayerToCornerLeft = CornerLocations[CornerLeftI] - PlayerLocation;
				PlayerToCornerRight = CornerLocations[CornerRightI] - PlayerLocation;
				EnemyToCornerLeft = CornerLocations[CornerLeftI] - EnemyLocation;
				EnemyToCornerRight = CornerLocations[CornerRightI] - EnemyLocation;
				CornerLeftToCenter = Box->CornerToCenter[CornerLeftI];
				CornerRightToCenter = Box->CornerToCenter[CornerRightI];
				// Use cross products to determine if the corner is between player and enemy.
				Cross1Z = PlayerToCornerLeft.X * CornerLeftToCenter.Y
						  - PlayerToCornerLeft.Y * CornerLeftToCenter.X;
				Cross2Z = EnemyToCornerLeft.X * CornerLeftToCenter.Y
						  - EnemyToCornerLeft.Y * CornerLeftToCenter.X;
				// If the signs differ, then the corner sits between the player and the enemy.
				CornerLeftBetween = (Cross1Z > 0) ^ (Cross2Z > 0);
				Cross1Z = PlayerToCornerRight.X * CornerRightToCenter.Y
						  - PlayerToCornerRight.Y * CornerRightToCenter.X;
				Cross2Z = EnemyToCornerRight.X * CornerRightToCenter.Y
						  - EnemyToCornerRight.Y * CornerRightToCenter.X;
				CornerRightBetween = (Cross1Z > 0) ^ (Cross2Z > 0);
				if (CornerLeftBetween || CornerRightBetween) {
					PlayerToCornerRight = CornerLocations[CornerRightI] - PlayerLocation;
					// Normalize vectors needed to calculate angles.
					// TODO: Handle the case when the vectors are 0.
					PlayerToCenter = Box->Center - PlayerLocation;

					AngleLeft = Utils::GetAngle(PlayerToCenter, PlayerToCornerLeft);
					AngleRight = Utils::GetAngle(PlayerToCenter, PlayerToCornerRight);
					AngleEnemy = Utils::GetAngle(PlayerToCenter, PlayerToEnemy);

					PeekingLeft = (AngleLeft > AngleEnemy - EnemyHalfAngularWidth);
					PeekingRight = (AngleRight < AngleEnemy + EnemyHalfAngularWidth);
				
					// Enemy is peeking neither left nor right. This box blocks LOS.
					if (!(PeekingLeft || PeekingRight)) {
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
	// Benchmark
	auto stop = std::chrono::high_resolution_clock::now();
	double delta = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
	count++;
	totaltime += delta;
	FString TimeMessage = "Average Time to cull (microseconds): " + FString::SanitizeFloat(totaltime / count);
	if(GEngine && (count % 60 == 0))
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, TimeMessage, true);
}

void ACornerCullingGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	CornerCull();
}
