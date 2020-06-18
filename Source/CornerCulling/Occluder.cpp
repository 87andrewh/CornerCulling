// Fill out your copyright notice in the Description page of Project Settings.

#include "Occluder.h"
#include "DrawDebugHelpers.h"
//#include "Components/StaticMeshComponent.h"
#include "Utils.h"

AOccluder::AOccluder()
{
	if (WITH_EDITOR)
	{
		PrimaryActorTick.bCanEverTick = true;
		PrimaryActorTick.bStartWithTickEnabled = true;
	}
}

void AOccluder::BeginPlay()
{
	Super::BeginPlay();
}

void AOccluder::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool AOccluder::ShouldTickIfViewportsOnly() const { return true;  }

