// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/BoxComponent.h"
#include "CullingBox.h"

// Sets default values
ACullingBox::ACullingBox()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACullingBox::BeginPlay()
{
	Super::BeginPlay();

	Mesh = CreateDefaultSubobject<UBoxComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	Center = GetActorLocation();
	//FVector CornerLocations [N];
	//FVector ToCenter [N];
}

// Called every frame
void ACullingBox::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
