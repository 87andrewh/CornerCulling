// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CornerCullingHUD.generated.h"

UCLASS()
class ACornerCullingHUD : public AHUD
{
	GENERATED_BODY()

public:
	ACornerCullingHUD();

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

};

