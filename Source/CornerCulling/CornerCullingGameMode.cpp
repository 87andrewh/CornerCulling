// Copyright Epic Games, Inc. All Rights Reserved.

#include "CornerCullingGameMode.h"
#include "CornerCullingHUD.h"
#include "CullingController.h"
#include "DrawDebugHelpers.h"
//#include "CornerCullingCharacter.h"

ACornerCullingGameMode::ACornerCullingGameMode()
	: Super()
{
	// Enable culling every tick.
	PrimaryActorTick.bCanEverTick = true;

	// Set default pawn class to our Blueprinted character.
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// Use our custom HUD class.
	HUDClass = ACornerCullingHUD::StaticClass();
}

void ACornerCullingGameMode::BeginPlay() {
}

void ACornerCullingGameMode::MarkFVector(const FVector2D& V) {
	FVector start = FVector(V.X, V.Y, 200);
	FVector end = start + FVector(0, 0, 200);
	DrawDebugLine(GetWorld(), start, end, FColor::Emerald, false, 0.3f, 0, 0.2f);
}

void ACornerCullingGameMode::ConnectVectors(const FVector2D& V1, const FVector2D& V2) {
	DrawDebugLine(GetWorld(), FVector(V1.X, V1.Y, 260), FVector(V2.X, V2.Y, 260), FColor::Emerald, false, 0.3f, 0, 0.2f);
}

  
void ACornerCullingGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	CullingController->BenchmarkCull();
}
