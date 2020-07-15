#pragma once

#include "CoreMinimal.h"
#include "CullingController.h"
#include "OccludingSphere.generated.h"

// Sphere that occludes vision.
 UCLASS(BlueprintType, Blueprintable)
class AOccludingSphere : public AActor
{
	 GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    UStaticMeshComponent* SphereMesh;

public:	
	AOccludingSphere();
    UPROPERTY(VisibleAnywhere)
    float Radius;

	// Update OccludingSphere's transform to enforce sphericality.
	void Update();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override;
};
