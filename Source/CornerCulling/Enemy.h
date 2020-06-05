// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CornerCullingCharacter.h"
#include "Enemy.generated.h"

UCLASS(config=Game)
class AEnemy : public AActor
{
	GENERATED_BODY()

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	AEnemy();

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

	float Width = 30.f;

	bool IsVisible = false;

	UPROPERTY(EditAnywhere)
	class UMaterialInterface* VisibleMaterial;

	UPROPERTY(EditAnywhere)
	class UMaterialInterface* InvisibleMaterial;

	UPROPERTY(EditAnywhere)
	AActor* PlayerCharacter;

	void SetVisible();
	void SetInvisible();

	virtual void Tick(float DeltaTime) override;
};
