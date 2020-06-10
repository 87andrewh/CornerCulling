// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CornerCullingCharacter.h"
#include "Enemy.h"
#include "CullingBox.h"
#include "VisiblePrism.h"
#include <deque>
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

	// How many frames pass between each cull.
	int CullingPeriod = 4;
	// Used to calcualte short rolling average of frame times.
	float RollingTotalTime = 0;
	float RollingAverageTime;
	// Max culling time in rolling window.
	int RollingMaxTime;
	// Number of frames in the rolling window.
	int RollingLength = 2 * CullingPeriod;
	// Total tick counter
	int TotalTicks = 0;

	// Cache of the index of recently used boxes.
	// For now uses FIFO. We'll test LRU, CLOCK, and RANDOM.
	std::deque<int> BoxIndexCache;
	// Flag if each index is in the cache.
	std::vector<bool> IndexInCache;
	// 50 is the number of lines of sight in a 5v5 game.
	int MaxCacheSize = 50;
	// Put index i in the cache, possibly evicting another member.
	void PutInCache(int i);

	// For benchmarking. Not necessary in production
	float TotalTime = 0;

protected:
	virtual void BeginPlay() override;
	void MarkFVector(const FVector2D& V);
	void ConnectVectors(const FVector2D& V1, const FVector2D& V2);
	// Reveal the Enemy to the Player.
	static void Reveal(ACornerCullingCharacter* Player, AEnemy* Enemy);
	// Check if line segment between points P3 and P4 blocks line of sight between P1 and P2.
	// P3 is on the left of P4.
	static bool IsBlocking(const FVector2D& P1, const FVector2D& P2, const FVector2D& P3, const FVector2D& P4);
	// Check if the VisiblePrism is blocking line of sight
	// between both P1LeftToP2Left and P1RightToP2Right.
	static bool IsBlocking(
		const FVector2D& P1Left,
		const FVector2D& P1Right,
		const FVector2D& P2Left,
		const FVector2D& P2Right,
		const float P1Z,
		const float P2Z,
		VisiblePrism* Prism
	);
	// Cull with desired algorithm. Default is angle mode, but enable line segment mode with:
	// #define LINE_SEGMENT_MODE
	void Cull();
	void BenchmarkCull();

public:
	ACornerCullingGameMode();
	virtual void Tick(float DeltaTime) override;
};
