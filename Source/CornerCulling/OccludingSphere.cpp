// Fill out your copyright notice in the Description page of Project Settings.

#include "OccludingSphere.h"

AOccludingSphere::AOccludingSphere()
	: Super()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

    // Initialize SphereMesh.
    SphereMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SphereMesh"));
    RootComponent = SphereMesh;
    static ConstructorHelpers::FObjectFinder<UStaticMesh>SphereMeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));
    SphereMesh->SetStaticMesh(SphereMeshAsset.Object);
    SphereMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AOccludingSphere::BeginPlay()
{
	Super::BeginPlay();
}

void AOccludingSphere::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Update();
}

void AOccludingSphere::Update()
{
    FVector Scale = GetActorScale3D();
    Scale.X = Scale.Z;
    Scale.Y = Scale.Z;
    // 100 is the base mesh's radius.
    Radius = 50 * Scale.Z;
	FTransform T = FTransform(GetActorRotation(), GetActorLocation(), Scale);
    SetActorTransform(T);
}

bool AOccludingSphere::ShouldTickIfViewportsOnly() const { return true;  }
