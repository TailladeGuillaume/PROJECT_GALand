// Copyright Epic Games, Inc. All Rights Reserved.

#include "GALandGameMode.h"
#include "GALandHUD.h"
#include "GALandCharacter.h"
#include "UObject/ConstructorHelpers.h"

AGALandGameMode::AGALandGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AGALandHUD::StaticClass();
}
