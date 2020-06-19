// Fill out your copyright notice in the Description page of Project Settings.

#include "Occluder.h"

AOccluder::AOccluder()
	: Super()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

// Draw edges of the occluder. Currently only implemented for cuboids.
// Code is not performance optimal, but it doesn't need to be.
void AOccluder::DrawEdges(bool Persist = false) {
	UWorld* World = GetWorld();
	for (int i = 0; i < CUBOID_F; i++) {
		//FString S = FString::SanitizeFloat(OccludingCuboid.Faces[i].Normal.X);
		//GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, S);
		for (int j = 0; j < CUBOID_FACE_V; j++) {
			ACullingController::ConnectVectors(
				World,
				OccludingCuboid.GetVertex(i, j),
				OccludingCuboid.GetVertex(i, (j + 1) % CUBOID_FACE_V),
				Persist
			);
		}
	}
}

void AOccluder::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(false);
	FlushPersistentDebugLines(GetWorld());

	FTransform T = GetTransform();
	Vectors.Reset();
	Vectors.Emplace(T.TransformPosition(V0));
	Vectors.Emplace(T.TransformPosition(V1));
	Vectors.Emplace(T.TransformPosition(V2));
	Vectors.Emplace(T.TransformPosition(V3));
	Vectors.Emplace(T.TransformPosition(V4));
	Vectors.Emplace(T.TransformPosition(V5));
	Vectors.Emplace(T.TransformPosition(V6));
	Vectors.Emplace(T.TransformPosition(V7));
	OccludingCuboid = Cuboid(Vectors);
	DrawEdges(true);
}

void AOccluder::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if ((int(DeltaTime) % 10) != 0) {
		return;
	}
	FTransform T = GetTransform();
	Vectors.Reset();
	Vectors.Emplace(T.TransformPosition(V0));
	Vectors.Emplace(T.TransformPosition(V1));
	Vectors.Emplace(T.TransformPosition(V2));
	Vectors.Emplace(T.TransformPosition(V3));
	Vectors.Emplace(T.TransformPosition(V4));
	Vectors.Emplace(T.TransformPosition(V5));
	Vectors.Emplace(T.TransformPosition(V6));
	Vectors.Emplace(T.TransformPosition(V7));
	OccludingCuboid = Cuboid(Vectors);
	DrawEdges(false);
}

bool AOccluder::ShouldTickIfViewportsOnly() const { return true;  }
