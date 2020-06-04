// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy.h"
#include "UObject/UObjectGlobals.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include <stdlib.h> 

// Sets default values
AEnemy::AEnemy()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

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
}

void AEnemy::SetVisible() {
	Mesh->SetMaterial(0, VisibleMaterial);
}

void AEnemy::SetInvisible() {
	Mesh->SetMaterial(0, InvisibleMaterial);
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}
