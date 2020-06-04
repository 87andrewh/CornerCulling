// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CornerCullingGameMode.generated.h"

UCLASS(minimalapi)
class ACornerCullingGameMode : public AGameModeBase
{
	GENERATED_BODY()

	TArray<AActor*> Players;
	TArray<AActor*> Enemies;
	TArray<AActor*> Boxes;

protected:
	void CornerCull();

public:
	ACornerCullingGameMode();
	virtual void Tick(float DeltaTime) override;
};



