// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VisiblePrism.h"
#include "CullingBox.generated.h"

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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// Update center and corner locations.
	void UpdateBounds();
};
