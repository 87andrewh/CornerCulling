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
	int RevealTimer = 0;
	int RevealTimerBaseMax = 20;
	// Make RevealTimer slightly random to prevent culling time spikes,
	// such as in case when a Viper wall goes down in a 5v5,
	// revealing everyone on the same frame, and causing all timer refresh checks to be synchronized.
	// Shouldn't be too small, or culling will be too clustered.
	int RevealTimerJitter = 20;
	// Adaptively multiply the revel timer if the enemy remains visible for a long time.
	float RevealTimerMultiplier = 1;
	// If the enemy is revealed, multiply the timer multiplier. Reveal time grows linearly with time
	// spent visible, as both the growth rate of the multiplier and the time between multiplications
	// are the same.
	float RevealTimerMultiplierMultiplier = 1.05f;
	float RevealTimerMultierMax = 3;

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
	virtual FVector GetCenter() override;
};
