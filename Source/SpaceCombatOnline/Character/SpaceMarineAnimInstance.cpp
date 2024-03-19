// Fill out your copyright notice in the Description page of Project Settings.


#include "SpaceMarineAnimInstance.h"
#include "SpaceMarine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "SpaceCombatOnline/Weapon/Weapon.h"

void USpaceMarineAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	SpaceMarine = Cast<ASpaceMarine>(TryGetPawnOwner());
}

void USpaceMarineAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (SpaceMarine == nullptr)
	{
		SpaceMarine = Cast<ASpaceMarine>(TryGetPawnOwner());
	}
	if (SpaceMarine == nullptr) return;

	FVector Velocity = SpaceMarine->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = SpaceMarine->GetCharacterMovement()->IsFalling();
	bIsAccelerating = SpaceMarine->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	bWeaponEquipped = SpaceMarine->IsWeaponEquipped();
	EquippedWeapon = SpaceMarine->GetEquippedWeapon();
	bIsCrouched = SpaceMarine->bIsCrouched;
	bAiming = SpaceMarine->IsAiming();
	TurningInPlace = SpaceMarine->GetTurningInPlace();
	bisDowned = SpaceMarine->isDowned();

	// Offset Yaw for Strafing
	FRotator AimRotation = SpaceMarine->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(SpaceMarine->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f);
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = SpaceMarine->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = SpaceMarine->GetAO_Yaw();
	AO_Pitch = SpaceMarine->GetAO_Pitch();
}

	//can probably delete all of this, doesnt help/work
//	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && SpaceMarine->GetMesh())
//	{
//		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
//		FVector OutPosition;
//		FRotator OutRotation;
//		SpaceMarine->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
//		LeftHandTransform.SetLocation(OutPosition);
//		LeftHandTransform.SetRotation(FQuat(OutRotation));
//
//		FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("hand_r"), ERelativeTransformSpace::RTS_World);
//		RightHandRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - SpaceMarine->GetHitTarget()));
//
//
//		FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleSocket"), ERelativeTransformSpace::RTS_World);
//		FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
//		DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f, FColor::Red);
//		DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), SpaceMarine->GetHitTarget(), FColor::Orange);
//
//	}
//}



