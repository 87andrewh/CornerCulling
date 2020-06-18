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
	
	//  Vectors that define the vertices of the cuboid.
	UPROPERTY(EditAnywhere)
	FVector V0;
	UPROPERTY(EditAnywhere)
	FVector V1;
	UPROPERTY(EditAnywhere)
	FVector V2;
	UPROPERTY(EditAnywhere)
	FVector V3;
	UPROPERTY(EditAnywhere)
	FVector V4;
	UPROPERTY(EditAnywhere)
	FVector V5;
	UPROPERTY(EditAnywhere)
	FVector V6;
	UPROPERTY(EditAnywhere)
	FVector V7;


public:	
	AOccluder();
	FCuboid OccludingCuboid;

protected:
	// Draw the bounds of this occluder in the editor.
	void DrawEdges();
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override;
};
