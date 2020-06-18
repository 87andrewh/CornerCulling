// Fill out your copyright notice in the Description page of Project Settings.

#include "CullingController.h"
#include "EngineUtils.h"
#include "CornerCullingCharacter.h"
#include "Occluder.h"
#include "Utils.h"
#include "Math/UnrealMathUtility.h"
//#include "DrawDebugHelpers.h"
#include <chrono> 

ACullingController::ACullingController()
	: Super()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
}

void ACullingController::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

    for (ACornerCullingCharacter* Player : TActorRange<ACornerCullingCharacter>(GetWorld()))
    {
		Characters.Emplace(Player);
		IsAlive.Emplace(true);
    }
	// Acquire the prisms of occluding objects.
    for (AOccluder* Occluder : TActorRange<AOccluder>(GetWorld()))
    {
		// Try profiling emplace.
		OccludingCuboids.Add(Occluder->OccludingCuboid);
    }
}


void ACullingController::UpdateCharacterBounds()
{
	for (int i = 0; i < Characters.Num(); i++) {
		if (IsAlive[i]) {
			Bounds.Add(CharacterBounds(Characters[i]->GetActorTransform()));
		}
	}
}

void ACullingController::PopulateBundles()
{
	for (int i = 0; i < Characters.Num(); i++) {
		if (IsAlive[i]) {
			for (int j = 0; j < Characters.Num(); j++) {
				if ((i != j) && IsAlive[j]) {
					BundleQueue.Add(Bundle(i, j));
				}
			}
		}
	}
}

void ACullingController::CullWithCache()
{
}

void ACullingController::CullRemaining()
{
}

void ACullingController::UpdateVisibility()
{
}


void ACullingController::ClearQueues()
{
}


void ACullingController::Tick(float DeltaTime)
{
	TotalTicks++;
	BenchmarkCull();
}

// Function for accurately and quickly computing LOS between all pairs of opponents.
// Can be sped up by building on top of PVS checks, or parallelizing checks.
void ACullingController::Cull() {
	UpdateCharacterBounds();
	PopulateBundles();
	CullWithCache();
	CullRemaining();
	UpdateVisibility();
	ClearQueues();
}

void ACullingController::BenchmarkCull() {
	auto Start = std::chrono::high_resolution_clock::now();
	Cull();
	auto Stop = std::chrono::high_resolution_clock::now();
	int Delta = std::chrono::duration_cast<std::chrono::microseconds>(Stop - Start).count();
	TotalTime += Delta;
	RollingTotalTime += Delta;
	RollingMaxTime = std::max(RollingMaxTime, Delta);
	if ((TotalTicks % RollingLength) == 0) {
		RollingAverageTime = RollingTotalTime / RollingLength;
		RollingTotalTime = 0;
		RollingMaxTime = 0;
	}
	if (GEngine && (TotalTicks - 1) % RollingLength == 0) {
		// Remember, 1 cull happens per culling period. Each cull takes period times as long as the average.
		// Make sure that multiple servers are staggered so these spikes do not add up.
		FString Msg = "Average time to cull (microseconds): " + FString::FromInt(int(TotalTime / TotalTicks));
		GEngine->AddOnScreenDebugMessage(1, 0.25f, FColor::Yellow, Msg, true, FVector2D(1.5f, 1.5f));
		Msg = "Rolling average time to cull (microseconds): " + FString::FromInt(int(RollingAverageTime));
		GEngine->AddOnScreenDebugMessage(2, 0.25f, FColor::Yellow, Msg, true, FVector2D(1.5f, 1.5f));
		Msg = "Rolling max time to cull (microseconds): " + FString::FromInt(RollingMaxTime);
		GEngine->AddOnScreenDebugMessage(3, 0.25f, FColor::Yellow, Msg, true, FVector2D(1.5f, 1.5f));
	}
}

