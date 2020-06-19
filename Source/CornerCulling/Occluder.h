// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CullingController.h"
#include "Occluder.generated.h"

// Box that occludes vision.
 UCLASS(BlueprintType, Blueprintable)
class AOccluder : public AActor
{
	GENERATED_BODY()

public:	
	//  Vectors that define the vertices of the cuboid.
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

	AOccluder();
	// Vertices of an occluding cuboid.
	UPROPERTY()
	TArray<FVector> Vectors;
	// The occluding cuboid.
	Cuboid OccludingCuboid;

protected:
	// Draw the bounds of this occluder in the editor.
	void DrawEdges(bool Persist);
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override;
};
