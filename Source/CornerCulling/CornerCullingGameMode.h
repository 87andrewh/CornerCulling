// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Enemy.h"
#include "CullingBox.h"
#include "VisiblePrism.h"
#include "CornerCullingGameMode.generated.h"

UCLASS(minimalapi)
class ACornerCullingGameMode : public AGameModeBase
{
	GENERATED_BODY()

	friend VisiblePrism;

	TArray<ACornerCullingCharacter*> Players;
	TArray<AEnemy*> Enemies;
	TArray<ACullingBox*> Boxes;

	FVector PlayerLocation3D;
	FVector2D PlayerLocation;
	FVector EnemyCenter3D;
	FVector2D EnemyCenter;
	FVector2D EnemyLeft = FVector2D();
	FVector2D EnemyRight = FVector2D();
	FVector2D PlayerToEnemy;
	FVector2D PlayerToEnemyLeft;
	FVector2D PlayerToEnemyRight;
	FVector2D BoxLeft = FVector2D();
	FVector2D BoxRight = FVector2D();
	FVector2D BoxCenter;
	FVector2D BoxRightToBoxLeft;
	FVector2D PlayerToBox;
	FVector2D PlayerToBoxLeft;
	FVector2D PlayerToBoxRight;
	FVector2D EnemyLeftToBoxLeft;
	FVector2D EnemyRightToBoxRight;
	// Used to store results of various cross products.
	float Cross1Z;
	float Cross2Z;
	// Angle between PlayerToEnemyLeft/Right and PlayerToBoxLeft/Right
	float AngleLeft;
	float AngleRight;
	// Track if any object blocks line of sight between a player and an enemy.
	bool Blocked = false;
	// How many frames pass between each cull.
	int CullingPeriod = 4;
	// Used to calcualte short rolling average of frame times.
	float RollingTotalTime = 0;
	float RollingAverageTime;
	// Number of frames in the rolling window.
	int RollingLength = 2 * CullingPeriod;
	// Total tick counter
	int TotalTicks = 0;
	// Random offset to the order in which occluding objects are iterated through.
	// Eliminates worst case scenario where all LOS checks are blocked by the last numbered object.
	// This converts long stretches of high frame into long stretches of short frame times.
	int RandomOffset = 0;
	// If the average frametime (microseconds) is above this value,
	// permute the order in which we check occluding objects.
	// Best to emperically tune this.
	float RandomizationThreshold = 55;

	// For benchmarking. Not necessary in production
	int box_i = 0;
	float TotalTime = 0;

protected:
	// Use corner culling to calculate LOS between all pairs of opponents.
	void CornerCull();
	// Cull using angles between corners and enemies.
	void AngleCull();
	// Cull by calculating intersection of line segments between LOS and occluding planes.
	void LineCull();
	virtual void BeginPlay() override;
	// Reveal the Enemy to the Player.
	static void Reveal(ACornerCullingCharacter* Player, AEnemy* Enemy);

public:
	ACornerCullingGameMode();
	virtual void Tick(float DeltaTime) override;
};
