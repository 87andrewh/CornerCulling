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

	class CornerCullingGameMode;

	// If a enemy is revealed, stay revealed for a few frame.
	// Combats otherwise worst-case scenario of all players being visible.
	int RevealTimer;
	int RevealTimerMax = 20;

	// Internal methods for setting visibility.
	// To externally reveal an enemy, call Reveal().
	// To externally hide an enemy, call Hide().
	void SetVisible();
	void SetInvisible();

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

	virtual void Tick(float DeltaTime) override;

	// Reveal the enemy.
	void Reveal();
	// Hide the enemy.
	void Hide();
	// Return if the enemy is almost visible.
	// Prevents flickering by allowig us to run a visibility check at the last invisible frame.
	bool IsAlmostVisible();
	// Return if the enemy is visible.
	bool IsVisible();
	// Update the bounding corners.
	void UpdateBounds();
};
