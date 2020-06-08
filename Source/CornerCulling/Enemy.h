// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CornerCullingCharacter.h"
#include "VisiblePrismInterface.h"
#include "Enemy.generated.h"

UCLASS(config=Game)
class AEnemy : public AActor, public VisiblePrismInterface
{
	GENERATED_BODY()

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	AEnemy();

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

	// Width of prism along X or Y axis.
	float BaseWidth = 33.5f;
	// Additional width between corners along XY axis.
	float CornerExtraWidth = BaseWidth * (2 / sqrt(2) - 1);
	// Unit vector from center of the enemy to a corner.
	FVector CenterToCorner;

	UPROPERTY(EditAnywhere)
	class UMaterialInterface* VisibleMaterial;

	UPROPERTY(EditAnywhere)
	class UMaterialInterface* InvisibleMaterial;

	void SetVisible();
	void SetInvisible();
	// Get half of the angular width of the enemy from the player's perspective.
	// Note: Distance is explicit so we don't worry about normalized vectors.
	float GetHalfAngularWidth(const FVector2D& PlayerToEnemy, const float Distance);

	// Set corners of the enemy.
	virtual void SetCorners() override;
	virtual void Tick(float DeltaTime) override;
};
