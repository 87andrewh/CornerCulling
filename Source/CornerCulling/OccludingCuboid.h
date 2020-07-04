// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CullingController.h"
#include "OccludingCuboid.generated.h"

// Box that occludes vision.
 UCLASS(BlueprintType, Blueprintable)
class AOccludingCuboid : public AActor
{
	 GENERATED_BODY()

	// Counts ticks to not draw every tick.
	int TickCount = 0;
	// Frames between draw calls.
	int DrawPeriod = 60;

public:	
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

	AOccludingCuboid();
	// Vertices of an occluding cuboid.
	UPROPERTY()
	TArray<FVector> Vectors = TArray<FVector>();
	// The occluding cuboid.
	Cuboid OccludingCuboid;

	// Update OccludingCuboid according to the vertices.
	void Update();

protected:
	// Draw the bounds of this occluder in the editor.
	void DrawEdges(bool Persist);
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override;
};
