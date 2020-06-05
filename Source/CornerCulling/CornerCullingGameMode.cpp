// Copyright Epic Games, Inc. All Rights Reserved.

#include "CornerCullingGameMode.h"
#include "CornerCullingHUD.h"
#include "CornerCullingCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "CullingBox.h"
#include "EngineUtils.h"
//#include "atan.cpp"
#include <cmath>
#include "DrawDebugHelpers.h"
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
}

// Get the angle between two FVectors of the form (X, Y, 0)
// Returns angles in the full range (-PI, PI)
float ACornerCullingGameMode::GetAngle(FVector V1, FVector V2) {
	float Dot = FVector::DotProduct(V1, V2);
	float Det = FVector::CrossProduct(V1, V2).Z;
	return atan2f(Det, Dot);
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
	// Number of boxes blocking LOS between each Player/Enemy pair.
	int BlockingCount;
	FVector PlayerLocation;
	FVector EnemyLocation;
	FVector PlayerToEnemy;
	TArray<FVector> CornerLocations;
	// Index of corners
	int CornerLeftI;
	int CornerRightI;
	// Vectors from point A to B.
	FVector PlayerToCornerLeft;
	FVector PlayerToCornerRight;
	FVector PlayerToCenter;
	FVector EnemyToCornerLeft;
	FVector CornerLeftToCenter;
	// Used to store Z componets of various cross products.
	// Used to determine relative positions and angles.
	float Cross1Z;
	float Cross2Z;
	float EnemyDistance;
	// Enables accurate culling at all distances.
	float EnemyAngularWidth;
	// Angle between PlayerToCenter and PlayerToCornerLeft/Right
	float AngleLeft;
	float AngleRight;
	// Angle between PlayerToCenter and PlayerToEnemy
	float AngleEnemy;
	// Tracks if the enemy is peeking out of the left or right corners.
	bool PeekingLeft;
	bool PeekingRight;
	
	//int count; //debug
	//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::FromInt(count));
	for (ACornerCullingCharacter* Player : Players)
	{
		PlayerLocation = Player->GetActorLocation();
		for (AEnemy* Enemy : Enemies)
		{
			BlockingCount = 0;
			// Call PVS culling between player and enemy.
			// if (!IsPotentiallyVisible(Enemy)) continue;

			EnemyLocation = Enemy->GetActorLocation();
			PlayerToEnemy = EnemyLocation - PlayerLocation;
			EnemyDistance = PlayerToEnemy.Size();
			PlayerToEnemy = PlayerToEnemy.GetSafeNormal2D(1e-6);

			// Calculate enemy angular width.
			EnemyAngularWidth = atan(Enemy->Width / EnemyDistance);

			// NOTE: Might be able to precompute relevant boxes per PVS region.
			for (ACullingBox* Box : Boxes) {
				CornerLocations = Box->CornerLocations;
				// Get get indicies of the leftmost and rightmost corners.
				Box->GetRelevantCorners(Player, &CornerLeftI, &CornerRightI);
				// Get vectors used to determine if the corner is between player and enemy.
				PlayerToCornerLeft = CornerLocations[CornerLeftI] - PlayerLocation;
				EnemyToCornerLeft = CornerLocations[CornerLeftI] - EnemyLocation;
				CornerLeftToCenter = Box->CornerToCenter[CornerLeftI];
				Cross1Z = FVector::CrossProduct(PlayerToCornerLeft, CornerLeftToCenter).Z;
				Cross2Z = FVector::CrossProduct(EnemyToCornerLeft, CornerLeftToCenter).Z;
				// If the signs differ, then the corner sits between the player and the enemy.
				if ((Cross1Z > 0) ^ (Cross2Z > 0)) {
					PlayerToCornerRight = CornerLocations[CornerRightI] - PlayerLocation;
					// Normalize vectors needed to calculate angles.
					// TODO: Handle the case when the vectors are 0.
					PlayerToCornerLeft = PlayerToCornerLeft.GetSafeNormal2D(1e-6);
					PlayerToCornerRight = PlayerToCornerRight.GetSafeNormal2D(1e-6);
					PlayerToCenter = Box->Center - PlayerLocation;
					PlayerToCenter = PlayerToCenter.GetSafeNormal2D(1e-6);

					AngleLeft = GetAngle(PlayerToCenter, PlayerToCornerLeft);
					AngleRight = GetAngle(PlayerToCenter, PlayerToCornerRight);
					AngleEnemy = GetAngle(PlayerToCenter, PlayerToEnemy);

					PeekingLeft = (AngleLeft > AngleEnemy - EnemyAngularWidth);
					PeekingRight = (AngleRight < AngleEnemy + EnemyAngularWidth);
				
					// Enemy is peeking neither left or right. This box blocks LOS.
					if (!(PeekingLeft || PeekingRight)) {
						BlockingCount += 1;
					}
				}

			}
			if (BlockingCount == 0) {
				DrawDebugLine(GetWorld(), PlayerLocation, PlayerLocation + 800.f * PlayerToEnemy, FColor::Emerald, false, 0.1f, 0, 0.2f);
				Reveal(Player, Enemy);
			} else {
				// For demo purposes, remove in production.
				Enemy->SetInvisible();
			}
		}
	}
}

void ACornerCullingGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	CornerCull();
}
