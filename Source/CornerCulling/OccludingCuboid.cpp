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
    if (DrawEdgesInGame)
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
	Vertices.Reset();
	Vertices.Emplace(T.TransformPosition(V0));
	Vertices.Emplace(T.TransformPosition(V1));
	Vertices.Emplace(T.TransformPosition(V2));
	Vertices.Emplace(T.TransformPosition(V3));
	Vertices.Emplace(T.TransformPosition(V4));
	Vertices.Emplace(T.TransformPosition(V5));
	Vertices.Emplace(T.TransformPosition(V6));
	Vertices.Emplace(T.TransformPosition(V7));
	OccludingCuboid = Cuboid(Vertices);
}

bool AOccludingCuboid::ShouldTickIfViewportsOnly() const { return true;  }
