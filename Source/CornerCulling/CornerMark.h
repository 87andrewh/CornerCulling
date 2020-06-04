// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CornerMark.generated.h"

UCLASS()
class CORNERCULLING_API ACornerMark : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACornerMark();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(Category = CornerMark, VisibleAnywhere)
	UStaticMeshComponent* Mesh;
};
