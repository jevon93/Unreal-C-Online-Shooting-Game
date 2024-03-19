// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SpaceCombatOnline/Weapon/Weapon.h"
#include "SpaceCombatOnline/Types/TurningInPlace.h"
#include "SpaceMarine.generated.h"

UCLASS()
class SPACECOMBATONLINE_API ASpaceMarine : public ACharacter
{
	GENERATED_BODY()

public:
	ASpaceMarine();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	void PlayFireMontage(bool bAiming);
	void PlayDownedMontage();

	//Player Downed or Eliminated

	UFUNCTION(NetMulticast, Reliable)
	void Elim();
	UFUNCTION(NetMulticast, Reliable)
	void Downed();
	void Revived();



protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float value);
	void LookUp(float value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void AimOffset(float DeltaTime);
	void FireButtonPressed();
	void FireButtonReleased();


	UFUNCTION(BlueprintCallable)
	void ReceiveDamage(AActor * DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
	UFUNCTION()
	void UpdateHUDHealth();

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
		class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
		class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* Combat;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;

	void TurninPlace(float DeltaTime);

	//Fire Weapon Anim
	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	//Downed Anim
	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* DownedMontage;
	
	//Player Health
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 250.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, BluePrintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Player Stats")
	float Health = 250.f;


	UFUNCTION()
	void OnRep_Health();

	class ACPlayerController* PlayerController;

	bool bisDowned = false;
	bool bisElimmed = false;

public:	
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const{ return AO_Pitch; }
	AWeapon* GetEquippedWeapon();

	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FVector GetHitTarget() const;

	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	FORCEINLINE bool isDowned() const { return bisDowned; }
};
