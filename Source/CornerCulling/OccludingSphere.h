// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CullingController.h"
#include "OccludingSphere.generated.h"

// Box that occludes vision.
 UCLASS(BlueprintType, Blueprintable)
class AOccludingSphere : public AActor
{
	 GENERATED_BODY()

	int TickCount = 0;

    UPROPERTY(EditAnywhere)
    UStaticMeshComponent* SphereMesh;

public:	
	AOccludingSphere();
	// Vertices of an occluding cuboid.
	// The occluding sphere.
	Sphere OccludingSphere;

	// Update OccludingSphere according to the vertices.
	void Update();

protected:
	// Draw the bounds of this occluder in the editor.
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override;
};
