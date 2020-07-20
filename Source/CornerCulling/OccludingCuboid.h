#pragma once

#include "CoreMinimal.h"
#include "CullingController.h"
#include "GeometricPrimitives.h"
#include "OccludingCuboid.generated.h"

// Cuboid that occludes vision.
 UCLASS(BlueprintType, Blueprintable)
class AOccludingCuboid : public AActor
{
	 GENERATED_BODY()

	// Counts ticks to not draw every tick.
	int TickCount = 0;
	// Frames between draw calls.
	int DrawPeriod = 60;

public:	
	AOccludingCuboid();
	// Vectors that define the vertices of the cuboid.
	// These are not in a list to enabled editing in UE4.
	UPROPERTY(EditAnywhere)
	FVector V0 = FVector(200, 200, 200);
	UPROPERTY(EditAnywhere)
	FVector V1 = FVector(-200, 200, 200);
	UPROPERTY(EditAnywhere)
	FVector V2 = FVector(-200, -200, 200);
	UPROPERTY(EditAnywhere)
	FVector V3 = FVector(200, -200, 200);
	UPROPERTY(EditAnywhere)
	FVector V4 = FVector(200, 200, -200);
	UPROPERTY(EditAnywhere)
	FVector V5 = FVector(-200, 200, -200);
	UPROPERTY(EditAnywhere)
	FVector V6 = FVector(-200, -200, -200);
	UPROPERTY(EditAnywhere)
	FVector V7 = FVector(200, -200, -200);
	// Vertices of an occluding cuboid.
	UPROPERTY()
	TArray<FVector> Vertices;
	UPROPERTY(EditAnywhere)
    bool DrawEdgesInGame = true;
	// The occluding cuboid.
    // NOTE:
    //   Redundant and separate from CullingController
    //   because of UE4's garbage collection.
	Cuboid OccludingCuboid;

	// Updates the OccludingCuboid according to the vertices.
	void Update();
	// Draw the edges of the OccludingCuboid in the level editor.
	void DrawEdges(bool Persist);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override;
};
