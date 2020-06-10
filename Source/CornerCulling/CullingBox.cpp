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

FVector ACullingBox::GetCenter() {
	return GetActorLocation();
}

// Update center and corner locations.
void ACullingBox::UpdateBounds() {
	FVector Center3D = GetActorLocation();
	Center = FVector2D(GetActorLocation());
	FVector Extents = FVector(50, 50, 50);
	FTransform T = GetActorTransform();
	Corners[0] = Center + FVector2D( T.TransformVector(FVector(Extents.X, Extents.Y, 0)));
	Corners[1] = Center + FVector2D( T.TransformVector(FVector(Extents.X, -Extents.Y, 0)));
	Corners[2] = Center + FVector2D( T.TransformVector(FVector(-Extents.X, Extents.Y, 0)));
	Corners[3] = Center + FVector2D( T.TransformVector(FVector(-Extents.X, -Extents.Y, 0)));
	ZTop = Center3D.Z + T.GetScale3D().Z * Extents.Z;
}
