// Fill out your copyright notice in the Description page of Project Settings.


#include "CPlayerController.h"
#include "SpaceCombatOnline/HUD/CHUD.h"
#include "SpaceCombatOnline/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"



void ACPlayerController::BeginPlay()
{
	Super::BeginPlay();

	PlayerHUD = Cast<ACHUD>(GetHUD());
}

void ACPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<ACHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD &&
		PlayerHUD->CharacterOverlay &&
		PlayerHUD->CharacterOverlay->HealthBar &&
		PlayerHUD->CharacterOverlay->HealthText;
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		PlayerHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health),
			FMath::CeilToInt(MaxHealth));
		PlayerHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
}