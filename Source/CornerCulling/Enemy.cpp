// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy.h"
#include "CullingBox.h"
#include "UObject/UObjectGlobals.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Utils.h"
#include <stdlib.h> 

// Sets default values
AEnemy::AEnemy()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	
	VisibleMaterial = CreateDefaultSubobject<UMaterial>(TEXT("VisibleMaterial"));
	InvisibleMaterial = CreateDefaultSubobject<UMaterial>(TEXT("InvisibleMaterial"));
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	InitCorners(4);
	UpdateBounds();
}

void AEnemy::SetVisible() {
	Mesh->SetMaterial(0, VisibleMaterial);
}

void AEnemy::SetInvisible() {
	Mesh->SetMaterial(0, InvisibleMaterial);
}

// Update bounds of the player.
// This implementation may appear to be code duplication of CullingBox.
// Howerver, note that one can change the position and number of corners.
// For example, pushing forward a corner if the player picks up a long gun.
void AEnemy::UpdateBounds() {
	FVector Center = GetActorLocation();
	FVector Extents = FVector(50, 50, 50);
	FTransform T = GetActorTransform();
	Corners[0] = FVector2D(Center + T.TransformVector(FVector(Extents.X, Extents.Y, 0)));
	Corners[1] = FVector2D(Center + T.TransformVector(FVector(Extents.X, -Extents.Y, 0)));
	Corners[2] = FVector2D(Center + T.TransformVector(FVector(-Extents.X, Extents.Y, 0)));
	Corners[3] = FVector2D(Center + T.TransformVector(FVector(-Extents.X, -Extents.Y, 0)));
	ZTop = Center.Z + Extents.Z;
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateBounds();
}
