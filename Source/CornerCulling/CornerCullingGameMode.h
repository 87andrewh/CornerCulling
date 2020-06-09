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
	bool Blocked;

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

	// Enables revealing enemies slightly before they exit cover (radians).
	float CullingThreshold = 0.001f;

	// Note: Code for benchmarking, remove from production.
	int count;
	float totaltime;

public:
	ACornerCullingGameMode();
	virtual void Tick(float DeltaTime) override;
};
