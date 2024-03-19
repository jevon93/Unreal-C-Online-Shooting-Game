// Fill out your copyright notice in the Description page of Project Settings.


#include "AlienGameMode.h"
#include "SpaceCombatOnline/Character/SpaceMarine.h"
#include "SpaceCombatOnline/PlayerController/CPlayerController.h"

void AAlienGameMode::PlayerDowned(ASpaceMarine* DownedCharacter)
{
	if (DownedCharacter)
	{
		DownedCharacter->Downed();
	}
}

void AAlienGameMode::PlayerRevive(ASpaceMarine* DownedCharacter)
{
	if (DownedCharacter)
	{

		DownedCharacter->Revived();

	}
}

void AAlienGameMode::PlayerEliminated(ASpaceMarine* ElimmedCharacter, ACPlayerController* VictimController, ACPlayerController* KillerController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}
