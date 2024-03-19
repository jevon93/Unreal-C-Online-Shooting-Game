// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "AlienGameMode.generated.h"

/**
 * 
 */
UCLASS()
class SPACECOMBATONLINE_API AAlienGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	virtual void PlayerDowned(class ASpaceMarine* DownedCharacter);
	virtual void PlayerRevive(class ASpaceMarine* DownedCharacter);
	virtual void PlayerEliminated(class ASpaceMarine*ElimmedCharacter, class ACPlayerController* VictimController, class ACPlayerController* KillerController);
};
