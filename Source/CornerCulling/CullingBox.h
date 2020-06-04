// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "CullingBox.generated.h"

UCLASS()
class CORNERCULLING_API ACullingBox : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACullingBox();

	UPROPERTY(Category = Box, VisibleAnywhere)
	UBoxComponent* Mesh;
	// Number of corners.
	UPROPERTY(Category = Box, VisibleAnywhere)
	int N = 4;
	// Arbitrary forward direction, used to order corners
	UPROPERTY(Category = Box, VisibleAnywhere)
	FVector Forward;
	// List of corners positions, ordered clockwise from forward vector
	UPROPERTY(Category = Box, VisibleAnywhere)
	TArray<FVector> CornerLocations;
	// Center of box
	UPROPERTY(Category = Box, VisibleAnywhere)
	FVector Center; 
	// Vectors from corners to the center
	UPROPERTY(Category = Box, VisibleAnywhere)
	TArray<FVector> CornerToCenter;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
