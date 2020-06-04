// Copyright Epic Games, Inc. All Rights Reserved.

#include "CornerCullingGameMode.h"
#include "CornerCullingHUD.h"
#include "CornerCullingCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "CullingBox.h"
#include <Runtime\Engine\Classes\Kismet\GameplayStatics.h>

ACornerCullingGameMode::ACornerCullingGameMode()
	: Super()
{
	// Set default pawn class to our Blueprinted character.
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// Use our custom HUD class.
	HUDClass = ACornerCullingHUD::StaticClass();

	// Sort and list actors so we that can iterate over them.
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACornerCullingCharacter::StaticClass(), Players);
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemy::StaticClass(), Enemies);
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACullingBox::StaticClass(), Boxes);
}

void ACornerCullingGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	CornerCull();
}

void ACornerCullingGameMode::CornerCull() {
	for (AActor* Player : Players)
	{
		FVector PlayerLocation = Player->GetActorLocation();
		for (AActor* Enemy : Enemies)
		{	
			// if (!IsPotentiallyVisible(Enemy)) continue;
			FVector EnemyLocation = Enemy->GetActorLocation();
			FVector PlayerToEnemy = EnemyLocation - PlayerLocation;

			for (AActor* Box : Boxes) {
				Box = (ACullingBox*) Box;
			}

		}
	}

}
