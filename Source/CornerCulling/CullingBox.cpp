// Fill out your copyright notice in the Description page of Project Settings.

#include "CullingBox.h"
#include "DrawDebugHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Utils.h"
#include "Components/BoxComponent.h"

// Sets default values
ACullingBox::ACullingBox()
{
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	RootComponent = Box;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ACullingBox::BeginPlay()
{
	Super::BeginPlay();

	Center = GetActorLocation();
	Center2D = FVector2D(Center);
	//FVector CornerLocations [N];
	//FVector ToCenter [N];
	FVector Extents = Box->GetScaledBoxExtent();
	TopZ = Center.Z + Extents.Z;
	// Initialize corners.
	N = 4;
	Corners.resize(N);
	SetCorners();
}

// Set corners of the box.
void ACullingBox::SetCorners() {
	FVector Extents = Box->GetScaledBoxExtent();
	FRotator Rotator = GetActorRotation();
	Corners[0] = FVector2D(Center + Rotator.RotateVector(FVector(Extents.X, Extents.Y, 0)));
	Corners[1] = FVector2D(Center + Rotator.RotateVector(FVector(Extents.X, -Extents.Y, 0)));
	Corners[2] = FVector2D(Center + Rotator.RotateVector(FVector(-Extents.X, Extents.Y, 0)));
	Corners[3] = FVector2D(Center + Rotator.RotateVector(FVector(-Extents.X, -Extents.Y, 0)));
}
