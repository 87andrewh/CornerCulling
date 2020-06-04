// Fill out your copyright notice in the Description page of Project Settings.


#include "CornerMark.h"

// Sets default values
ACornerMark::ACornerMark()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;


	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	RootComponent = Mesh;
}

// Called when the game starts or when spawned
void ACornerMark::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACornerMark::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

