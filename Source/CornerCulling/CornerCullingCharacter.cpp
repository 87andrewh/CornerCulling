// Copyright Epic Games, Inc. All Rights Reserved.

#include "CornerCullingCharacter.h"
#include "CornerCullingProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Utils.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// ACornerCullingCharacter

ACornerCullingCharacter::ACornerCullingCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 0.1886f;
	BaseLookUpRate = 0.1886f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(0.f, 0.f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.
}

void ACornerCullingCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	// Show the single player gun mesh.
	Mesh1P->SetHiddenInGame(false, true);
}

//////////////////////////////////////////////////////////////////////////
// Input

void ACornerCullingCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ACornerCullingCharacter::OnFire);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &ACornerCullingCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ACornerCullingCharacter::MoveRight);

	// Bind mouse movements to rotation functions.
	PlayerInputComponent->BindAxis("Turn", this, &ACornerCullingCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &ACornerCullingCharacter::LookUpAtRate);
}

void ACornerCullingCharacter::OnFire()
{
	// Fire a line trace.
	UWorld* const World = GetWorld();
	if (World != NULL)
	{
		FVector Start = FirstPersonCameraComponent->GetComponentLocation() + FVector(0, 0, -10);
		FVector Forward = FirstPersonCameraComponent->GetForwardVector();
		FVector End = Start + 2000.f * Forward;
		DrawDebugLine(GetWorld(), Start, End, FColor::Turquoise, false, 1, 0, 1.f);
	}

	// try and play a firing animation if specified
	if (FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void ACornerCullingCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void ACornerCullingCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void ACornerCullingCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate);
}

void ACornerCullingCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate);
}

// Called every frame
void ACornerCullingCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FVector ACornerCullingCharacter::GetCameraLocation()
{
	return GetFirstPersonCameraComponent()->GetComponentLocation();
}

// Get maximum displacement along axis perpendicular to PlayerToEnemy between culling events.
// The Magnitude is ideally a function of culling period, server latency, player maximum acceleration,
// plyaer maximum speed, player maximum velocity, and location-modifying game events.
void ACornerCullingCharacter::GetPerpendicularDisplacement(const FVector2D& PlayerToEnemy, FVector2D& Displacement) {
	float Distance = PlayerToEnemy.Size();
	if (abs(Distance) < Utils::MIN_SAFE_LENGTH) {
		Displacement = FVector2D::ZeroVector;
	}
	// I said ideally.
	float Magnitude = 20;
	Displacement = FVector2D(-PlayerToEnemy.Y, PlayerToEnemy.X) * (Magnitude / Distance);
}
