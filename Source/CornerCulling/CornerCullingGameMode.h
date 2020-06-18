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
	void MarkFVector(const FVector2D& V);
	void ConnectVectors(const FVector2D& V1, const FVector2D& V2);

public:
	ACornerCullingGameMode();
	virtual void Tick(float DeltaTime) override;
};
