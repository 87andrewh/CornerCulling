#include "OccludingCuboid.h"

AOccludingCuboid::AOccludingCuboid()
	: Super()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

// Draw edges of the occluder. Currently only implemented for cuboids.
// Code is not performance optimal, but it doesn't need to be.
void AOccludingCuboid::DrawEdges(bool Persist = false)
{
	UWorld* World = GetWorld();
	for (int i = 0; i < CUBOID_F; i++)
    {
		//FString S = FString::SanitizeFloat(OccludingCuboid.Faces[i].Normal.X);
		//GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, S);
		for (int j = 0; j < CUBOID_FACE_V; j++)
        {
			ACullingController::ConnectVectors(
				World,
				OccludingCuboid.GetVertex(i, j),
				OccludingCuboid.GetVertex(i, (j + 1) % CUBOID_FACE_V),
				Persist,
				1 + (DrawPeriod / 120.0f),
				3,
				FColor::Black
			);
		}
	}
}

void AOccludingCuboid::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(false);
	Update();
	DrawEdges(true);
}

void AOccludingCuboid::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TickCount++;
	if ((TickCount % DrawPeriod) == 0)
    {
		Update();
		DrawEdges(false);
	}
}

void AOccludingCuboid::Update()
{
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
}

bool AOccludingCuboid::ShouldTickIfViewportsOnly() const { return true;  }
