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
	SetActorTickEnabled(false);
    OccludingSphere = Sphere(FVector(0, 0, 0), 100);
	Update();
}

void AOccludingSphere::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TickCount++;
	Update();
}

void AOccludingSphere::Update() {
	FTransform T = GetTransform();
    FVector Scale = GetActorScale3D();
    Scale.X = Scale.Z;
    Scale.Y = Scale.Z;
    OccludingSphere.Center = T.TransformPosition(OccludingSphere.Center);
    OccludingSphere.Radius = OccludingSphere.Radius * Scale.Z;
	T = FTransform(T.Rotator(), T.GetTranslation(), Scale);
    SetActorTransform(T);
}

bool AOccludingSphere::ShouldTickIfViewportsOnly() const { return true;  }
