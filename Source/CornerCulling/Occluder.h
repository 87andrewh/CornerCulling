// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CullingController.h"
#include "Occluder.generated.h"

// Box that occludes vision.
// MAP EDITORS, OVERLAP YOUR BOXES FOR FASTER FRAMES !!!
// The more boxes block a line of sight, the more likely it is to find one quickly.
// But don't add redundant boxes.
UCLASS()
class AOccluder : public AActor
{
	GENERATED_BODY()

public:	
	AOccluder();

	FCuboid OccludingCuboid;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override;
};
