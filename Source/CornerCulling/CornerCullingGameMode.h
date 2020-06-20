// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CullingController.h"
#include "CornerCullingGameMode.generated.h"

UCLASS(minimalapi)
class ACornerCullingGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

public:
	ACornerCullingGameMode();
	virtual void Tick(float DeltaTime) override;
};
