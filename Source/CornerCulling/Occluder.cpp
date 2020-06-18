// Fill out your copyright notice in the Description page of Project Settings.

#include "Occluder.h"

AOccluder::AOccluder()
{
	if (WITH_EDITOR)
	{
		PrimaryActorTick.bCanEverTick = true;
		PrimaryActorTick.bStartWithTickEnabled = true;
	}
}

// Draw edges of the occluder. Currently only implemented for cuboids.
// Code is not performance optimal, but it doesn't need to be.
void AOccluder::DrawEdges() {
	UWorld* World = GetWorld();
	for (int i = 0; i < CUBOID_F; i++) {
		for (int j = 0; j < CUBOID_FACE_V; j++) {
			ACullingController::ConnectVectors(
				World,
				OccludingCuboid.GetVertex(i, j),
				OccludingCuboid.GetVertex(i, (j + 1) % CUBOID_FACE_V)
			);
		}
	}
}

void AOccluder::BeginPlay()
{
	Super::BeginPlay();
	PrimaryActorTick.bCanEverTick = false;
}

void AOccluder::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool AOccluder::ShouldTickIfViewportsOnly() const { return true;  }
