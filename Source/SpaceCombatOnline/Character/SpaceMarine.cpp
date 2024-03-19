


#include "SpaceMarine.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "SpaceCombatOnline/Weapon/Weapon.h"
#include "SpaceCombatOnline/SpaceComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "SpaceMarineAnimInstance.h"
#include "SpaceCombatOnline/PlayerController/CPlayerController.h"
#include "SpaceCombatOnline/GameMode/AlienGameMode.h"

ASpaceMarine::ASpaceMarine()
{

	PrimaryActorTick.bCanEverTick = true;
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom,USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	//Two lines of code to stop Camera from colliding with Character
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_GameTraceChannel1);
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
}

void ASpaceMarine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASpaceMarine, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASpaceMarine, Health, COND_None);
}



void ASpaceMarine::Elim_Implementation()
{
	if (bisDowned)
	{
		bisElimmed = true;
		//Disable Collision
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		//Drop Weapon
		if (Combat && Combat->EquippedWeapon)
		{
			Combat->EquippedWeapon->Dropped();
		}
	}
}

void ASpaceMarine::Downed_Implementation()
{
	bisDowned = true;
	PlayDownedMontage();

	//Disable Character Movement
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if (PlayerController)
	{
		DisableInput(PlayerController);
	}
}

void ASpaceMarine::Revived()
{
	bisDowned = false;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	if (PlayerController)
	{
		EnableInput(PlayerController);
	}
}

void ASpaceMarine::BeginPlay()
{
	Super::BeginPlay();
	
	UpdateHUDHealth();
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ASpaceMarine::ReceiveDamage);
	}
}



void ASpaceMarine::UpdateHUDHealth()
{
	PlayerController = PlayerController == nullptr ? Cast<ACPlayerController>(Controller) : PlayerController;
	if (PlayerController)
	{
		PlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ASpaceMarine::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHUDHealth();

	//Eliminated
	if (Health == 0.f && bisDowned)
	{
		AAlienGameMode* AlienGameMode = GetWorld()->GetAuthGameMode<AAlienGameMode>();
		if (AlienGameMode)
		{
			PlayerController = PlayerController == nullptr ? Cast<ACPlayerController>(Controller) : PlayerController;
			ACPlayerController* KillerController = Cast<ACPlayerController>(InstigatorController);
			AlienGameMode->PlayerEliminated(this, PlayerController, KillerController);
		}
	}
	//Downed
	if (Health == 0.f && !bisDowned)
	{
		Health = (MaxHealth / 2.f);

		AAlienGameMode* AlienGameMode = GetWorld()->GetAuthGameMode<AAlienGameMode>();
		if (AlienGameMode)
		{
			//Calls Downed() on this module which sets bIsDowned and plays anim montage
			AlienGameMode->PlayerDowned(this);
		}
	}
	//Revived
	if (Health == MaxHealth && bisDowned)
	{
		Health = MaxHealth;

		AAlienGameMode* AlienGameMode = GetWorld()->GetAuthGameMode<AAlienGameMode>();
		if (AlienGameMode)
		{
			AlienGameMode->PlayerRevive(this);
		}
	}
}

void ASpaceMarine::OnRep_Health()
{
	UpdateHUDHealth();
}

void ASpaceMarine::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AimOffset(DeltaTime);

}

void ASpaceMarine::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASpaceMarine::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASpaceMarine::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ASpaceMarine::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ASpaceMarine::LookUp);

	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ASpaceMarine::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASpaceMarine::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ASpaceMarine::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ASpaceMarine::AimButtonReleased);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASpaceMarine::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASpaceMarine::FireButtonReleased);

}

void ASpaceMarine::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
}

void ASpaceMarine::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ASpaceMarine::PlayDownedMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DownedMontage)
	{
		AnimInstance->Montage_Play(DownedMontage);
	}
}

void ASpaceMarine::MoveForward(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}

}

void ASpaceMarine::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ASpaceMarine::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ASpaceMarine::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}


void ASpaceMarine::CrouchButtonPressed()
{
	if (Combat->EquippedWeapon)
	{
		if (bIsCrouched)
		{
			UnCrouch();
		}
		else
		{
			Crouch();
		}
	}
}

void ASpaceMarine::AimButtonPressed()
{
	if (Combat->EquippedWeapon)
	{
		Combat->SetAiming(true);
	}
}

void ASpaceMarine::AimButtonReleased()
{
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void ASpaceMarine::AimOffset(float DeltaTime)
{
	//Don't need aim offset for uneqipped
	if (Combat && Combat->EquippedWeapon == nullptr) return;

	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	float Speed = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir) //standing still, not jumping
	{
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw; //get rotation for upper body when standing still
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurninPlace(DeltaTime);

	}
	if (Speed > 0.f || bIsInAir) //running or jumping
	{
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f; //reset rotation calculation after running or jumping, so that upper body moves normally while standing still
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		//map pitch from [270, 360] to [-90,0]
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}

}

void ASpaceMarine::FireButtonPressed()
{
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void ASpaceMarine::FireButtonReleased()
{
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}



void ASpaceMarine::TurninPlace(float DeltaTime)
{
	if (AO_Yaw > 45.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -45.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ASpaceMarine::EquipButtonPressed()
{
	if (Combat)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed();
		}
	}
}

void ASpaceMarine::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}


void ASpaceMarine::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}

	OverlappingWeapon = Weapon;
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void ASpaceMarine::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}

	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

bool ASpaceMarine::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool ASpaceMarine::IsAiming()
{
	return (Combat && Combat->bAiming);
}

AWeapon* ASpaceMarine::GetEquippedWeapon()
{
	if (Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

FVector ASpaceMarine::GetHitTarget() const
{

	if (Combat == nullptr) return FVector();
	return Combat->HitTarget;
}




