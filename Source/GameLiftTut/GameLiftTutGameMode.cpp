// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "GameLiftTutGameMode.h"
#include "GameLiftTutCharacter.h"
#include "UObject/ConstructorHelpers.h"

AGameLiftTutGameMode::AGameLiftTutGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
