// Fill out your copyright notice in the Description page of Project Settings.

#include "CullingBox.h"
#include "DrawDebugHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Utils.h"
#include "Components/BoxComponent.h"

ACullingBox::ACullingBox()
{
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	RootComponent = Box;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
}

void ACullingBox::BeginPlay()
{
	Super::BeginPlay();

	// Initialize corners.
	InitCorners(4);
	UpdateBounds();
}

// Update center and corner locations.
void ACullingBox::UpdateBounds() {
	FVector Center = GetActorLocation();
	FVector Extents = FVector(50, 50, 50);
	FTransform T = GetActorTransform();
	Corners[0] = FVector2D(Center + T.TransformVector(FVector(Extents.X, Extents.Y, 0)));
	Corners[1] = FVector2D(Center + T.TransformVector(FVector(Extents.X, -Extents.Y, 0)));
	Corners[2] = FVector2D(Center + T.TransformVector(FVector(-Extents.X, Extents.Y, 0)));
	Corners[3] = FVector2D(Center + T.TransformVector(FVector(-Extents.X, -Extents.Y, 0)));
	ZTop = Center.Z + T.GetScale3D().Z * Extents.Z;
}
