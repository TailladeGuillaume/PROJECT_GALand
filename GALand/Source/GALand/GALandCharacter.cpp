// Copyright Epic Games, Inc. All Rights Reserved.

#include "GALandCharacter.h"
#include "GALandProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AGALandCharacter

AGALandCharacter::AGALandCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	/** First Person **/

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	StaticMeshFirstPerson = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SM_FirstPerson"));
	//StaticMeshFirstPerson->SetOnlyOwnerSee(true);
	//StaticMeshFirstPerson->SetOwnerNoSee(false);
	StaticMeshFirstPerson->SetupAttachment(FirstPersonCameraComponent);
	StaticMeshFirstPerson->bCastDynamicShadow = false;
	StaticMeshFirstPerson->CastShadow = false;
	//StaticMeshFirstPerson->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	//StaticMeshFirstPerson->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_GunMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_GunMesh"));
	//FP_GunMesh->SetOnlyOwnerSee(true);
	//FP_GunMesh->SetOwnerNoSee(false);
	FP_GunMesh->SetupAttachment(StaticMeshFirstPerson);
	FP_GunMesh->bCastDynamicShadow = false;
	FP_GunMesh->CastShadow = false;

	FP_SpawnShootLocation = CreateDefaultSubobject<USceneComponent>(TEXT("FP_SpawnShootLocation"));
	FP_SpawnShootLocation->SetupAttachment(FP_GunMesh);
	FP_SpawnShootLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	/** Third Person **/

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	StaticMeshThirdPerson = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SM_ThirdPerson"));
	//StaticMeshThirdPerson->SetOnlyOwnerSee(false);
	//StaticMeshThirdPerson->SetOwnerNoSee(true);
	StaticMeshThirdPerson->SetupAttachment(RootComponent);
	StaticMeshThirdPerson->bCastDynamicShadow = false;
	StaticMeshThirdPerson->CastShadow = false;
	//StaticMeshThirdPerson->SetRelativeRotation(FRotator(0, 0, -90));
	//StaticMeshThirdPerson->SetRelativeLocation(FVector(0,0,-90));

	// Create a gun mesh component
	TP_GunMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TP_GunMesh"));
	//TP_GunMesh->SetOnlyOwnerSee(false);
	//TP_GunMesh->SetOwnerNoSee(true);
	TP_GunMesh->SetupAttachment(StaticMeshThirdPerson);
	TP_GunMesh->bCastDynamicShadow = false;
	TP_GunMesh->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));

	TP_SpawnShootLocation = CreateDefaultSubobject<USceneComponent>(TEXT("TP_SpawnShootLocation"));
	TP_SpawnShootLocation->SetupAttachment(TP_GunMesh);
	TP_SpawnShootLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));
	

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.

}

void AGALandCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_GunMesh->AttachToComponent(StaticMeshFirstPerson, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));
	TP_GunMesh->AttachToComponent(StaticMeshThirdPerson, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("Weapon_Socket"));

}

//////////////////////////////////////////////////////////////////////////
// Input

void AGALandCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event (FAIT EN BLUEPRINT)
	//PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AGALandCharacter::OnFire);

	// Enable touchscreen input
	EnableTouchscreenMovement(PlayerInputComponent);

	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AGALandCharacter::OnResetVR);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AGALandCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGALandCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AGALandCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AGALandCharacter::LookUpAtRate);
}

void AGALandCharacter::OnFire_Owner()
{
	
	// try and play the sound if specified
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = StaticMeshFirstPerson->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void AGALandCharacter::OnFire_Server()
{
	// try and fire a projectile
	if (ProjectileClass != nullptr)
	{
		UWorld* const World = GetWorld();
		
		const FRotator SpawnRotation = GetControlRotation();
		// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
		const FVector SpawnLocation = ((FP_SpawnShootLocation != nullptr) ? FP_SpawnShootLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);

		//Set Spawn Collision Handling Override
		FActorSpawnParameters ActorSpawnParams;
		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

		// APPELER SUR LE SERVEUR
		//spawn the projectile at the muzzle
		World->SpawnActor<AGALandProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);

	}

}

void AGALandCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AGALandCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		//OnFire_Owner();
		//OnFire_Server();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AGALandCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	TouchItem.bIsPressed = false;
}

//Commenting this section out to be consistent with FPS BP template.
//This allows the user to turn without using the right virtual joystick

//void AGALandCharacter::TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location)
//{
//	if ((TouchItem.bIsPressed == true) && (TouchItem.FingerIndex == FingerIndex))
//	{
//		if (TouchItem.bIsPressed)
//		{
//			if (GetWorld() != nullptr)
//			{
//				UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();
//				if (ViewportClient != nullptr)
//				{
//					FVector MoveDelta = Location - TouchItem.Location;
//					FVector2D ScreenSize;
//					ViewportClient->GetViewportSize(ScreenSize);
//					FVector2D ScaledDelta = FVector2D(MoveDelta.X, MoveDelta.Y) / ScreenSize;
//					if (FMath::Abs(ScaledDelta.X) >= 4.0 / ScreenSize.X)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.X * BaseTurnRate;
//						AddControllerYawInput(Value);
//					}
//					if (FMath::Abs(ScaledDelta.Y) >= 4.0 / ScreenSize.Y)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.Y * BaseTurnRate;
//						AddControllerPitchInput(Value);
//					}
//					TouchItem.Location = Location;
//				}
//				TouchItem.Location = Location;
//			}
//		}
//	}
//}

void AGALandCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AGALandCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AGALandCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AGALandCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool AGALandCharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AGALandCharacter::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &AGALandCharacter::EndTouch);

		//Commenting this out to be more consistent with FPS BP template.
		//PlayerInputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AGALandCharacter::TouchUpdate);
		return true;
	}
	
	return false;
}
