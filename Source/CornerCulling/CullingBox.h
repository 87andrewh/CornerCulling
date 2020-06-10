// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VisiblePrism.h"
#include "CullingBox.generated.h"

// Box that occludes vision.
// MAP EDITORS, OVERLAP YOUR BOXES FOR FASTER FRAMES !!!
// The more boxes block a line of sight, the more likely it is to find one quickly.
// But don't add redundant boxes.
UCLASS()
class CORNERCULLING_API ACullingBox : public AActor, public VisiblePrism
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACullingBox();

	// Root BoxComponent.
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* Box;
	// Mesh.
	UPROPERTY(VisibleDefaultsOnly)
	class UStaticMeshComponent* Mesh;

	// Update the bounds of this box (center, corners, height)
	void UpdateBounds();
	virtual FVector GetCenter() override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
