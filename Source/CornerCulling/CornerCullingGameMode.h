// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CornerCullingCharacter.h"
#include "Enemy.h"
#include "CullingBox.h"
#include "VisiblePrism.h"
#include "CornerCullingGameMode.generated.h"

UCLASS(minimalapi)
class ACornerCullingGameMode : public AGameModeBase
{
	GENERATED_BODY()

	friend VisiblePrism;
	friend ACornerCullingCharacter;

	TArray<ACornerCullingCharacter*> Players;
	TArray<AEnemy*> Enemies;
	TArray<ACullingBox*> Boxes;
	FVector PlayerCenter3D;
	FVector2D PlayerCenter;
	FVector EnemyCenter3D;
	FVector2D EnemyCenter;
	FVector2D PlayerToEnemy;
	// Left and rightmost corners of the enemy bouding prism, from the player's perspective.
	FVector2D EnemyLeft = FVector2D();
	FVector2D EnemyRight = FVector2D();
	// Left and rightmost positons that the player could be along an axis perpendicular to PlayerToEnemy
	// in the time between culling events, defined by PlayerPerpendicularDisplacement.
	FVector2D PlayerLeft;
	FVector2D PlayerRight;
	FVector2D PlayerPerpendicularDisplacement = FVector2D();
	FVector2D PlayerLeftToEnemyLeft;
	FVector2D PlayerRightToEnemyRight;
	FVector2D BoxCenter;
	// Left and rightmost corners of the occluding object, from the player's perspective.
	FVector2D BoxLeft = FVector2D();
	FVector2D BoxRight = FVector2D();
	FVector2D BoxRightToBoxLeft;
	FVector2D PlayerToBox;
	FVector2D PlayerLeftToBoxLeft;
	FVector2D PlayerRightToBoxRight;
	FVector2D EnemyLeftToBoxLeft;
	FVector2D EnemyRightToBoxRight;
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
	virtual void BeginPlay() override;
	// Reveal the Enemy to the Player.
	static void Reveal(ACornerCullingCharacter* Player, AEnemy* Enemy);
	// Cull with desired algorithm. Default is angle mode, but enable line segment mode with:
	#define LINE_SEGMENT_MODE
	void Cull();
	void BenchmarkCull();

public:
	ACornerCullingGameMode();
	virtual void Tick(float DeltaTime) override;
};
