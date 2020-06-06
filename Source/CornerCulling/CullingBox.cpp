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
	Center.Z = 300;
	//FVector CornerLocations [N];
	//FVector ToCenter [N];
	FVector Extents = Box->GetScaledBoxExtent();
	FRotator Rotator = GetActorRotation();

	// Initialize corner locations.
	// TODO: N is a variable, but corners are hardcoded for cubes.
	CornerLocations.SetNum(N);
	CornerLocations[0] = Center + Rotator.RotateVector(FVector(Extents.X, Extents.Y, 0));
	CornerLocations[1] = Center + Rotator.RotateVector(FVector(Extents.X, -Extents.Y, 0));
	CornerLocations[2] = Center + Rotator.RotateVector(FVector(-Extents.X, Extents.Y, 0));
	CornerLocations[3] = Center + Rotator.RotateVector(FVector(-Extents.X, -Extents.Y, 0));

	// Initialize corner to center vectors.
	CornerToCenter.SetNum(N);
	CornerToCenter[0] = Center - CornerLocations[0];
	CornerToCenter[1] = Center - CornerLocations[1];
	CornerToCenter[2] = Center - CornerLocations[2];
	CornerToCenter[3] = Center - CornerLocations[3];
}
 
/** Get indices of the two corners that could hide an enemy from the player, storing them in Corner1/2.
*/
void ACullingBox::GetRelevantCorners(const FVector& PlayerLocation, int& CornerLeftI, int& CornerRightI)
{
	FVector PlayerToCenter = Center - PlayerLocation;
	// Angle between PlayerToCenter and PlayerToCorner
	float Angle;
	float Min = FLT_MAX;
	float Max = FLT_MIN;
	int MinIndex = 0;
	int MaxIndex = 0;
	for (int i = 0; i < N; i++)
	{
		FVector PlayerToCorner = CornerLocations[i] - PlayerLocation;
		// NOTE: Confident, but not certain that GetAngleFast maintains correctness.
		Angle = Utils::GetAngleFast(PlayerToCenter, PlayerToCorner);
		if (Angle < Min)
		{
			Min = Angle;
			MinIndex = i;
		}
		if (Angle > Max)
		{
			Max = Angle;
			MaxIndex = i;
		}
	}
	CornerLeftI = MinIndex;
	CornerRightI = MaxIndex;
}
