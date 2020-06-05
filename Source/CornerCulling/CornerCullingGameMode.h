// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Enemy.h"
#include "CullingBox.h"
#include "CornerCullingGameMode.generated.h"

UCLASS(minimalapi)
class ACornerCullingGameMode : public AGameModeBase
{
	GENERATED_BODY()

	TArray<ACornerCullingCharacter*> Players;
	TArray<AEnemy*> Enemies;
	TArray<ACullingBox*> Boxes;

protected:
	// Use corner culling to calculate LOS between all pairs of opponents.
	void CornerCull();
	virtual void BeginPlay() override;
	// Reveal the Enemy to the Player.
	static void Reveal(ACornerCullingCharacter* Player, AEnemy* Enemy);

public:
	ACornerCullingGameMode();
	virtual void Tick(float DeltaTime) override;
};



