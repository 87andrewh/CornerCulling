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
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	RootComponent = Mesh;
	
	VisibleMaterial = CreateDefaultSubobject<UMaterial>(TEXT("VisibleMaterial"));
	InvisibleMaterial = CreateDefaultSubobject<UMaterial>(TEXT("InvisibleMaterial"));
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	CenterToCorner = GetActorRotation().RotateVector(FVector(1, 1, 0));
	CenterToCorner = CenterToCorner.GetSafeNormal2D(Utils::MIN_SAFE_LENGTH);
}

void AEnemy::SetVisible() {
	Mesh->SetMaterial(0, VisibleMaterial);
}

void AEnemy::SetInvisible() {
	Mesh->SetMaterial(0, InvisibleMaterial);
}

// Get half of the enemy's angular width's from the player's perspective.
// Currently implemented for boxes, but you could extend it to account for guns sticking out.
float AEnemy::GetHalfAngularWidth(const FVector2D& PlayerToEnemy, const float Distance) {
	FVector2D Normalized = PlayerToEnemy.GetSafeNormal(Utils::MIN_SAFE_LENGTH);
	float cos = Normalized.X * CenterToCorner.X + Normalized.Y * CenterToCorner.Y;
	// Largest cosine of angle between PlayerToEnemy and and CenterToCorner
	// for all corners. Uses double angle identity.
	// Corners contribute the most width when protruding out perpendicular to PlayerToEnemy.
	float CornerMultiplier = abs(2 * cos * cos - 1);
	float ApparentWidth = BaseWidth + CornerMultiplier * CornerExtraWidth;
	// In a scientic setting, we would use arctangent, but identity fast, and overestimation is fine.
	// It is even fairly accurate because ApparentWidth / (2 * Distance) is usually small.
	return ApparentWidth / 2 / Distance;
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}
