// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CornerCullingCharacter.h"
#include "VisiblePrism.h"
#include "Enemy.generated.h"

UCLASS(config=Game)
class AEnemy : public AActor, public VisiblePrism
{
	GENERATED_BODY()

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	AEnemy();

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere)
	class UMaterialInterface* VisibleMaterial;

	UPROPERTY(EditAnywhere)
	class UMaterialInterface* InvisibleMaterial;

	void SetVisible();
	void SetInvisible();

	// Update center and corner locations.
	void UpdateBounds();
	virtual void Tick(float DeltaTime) override;
};
