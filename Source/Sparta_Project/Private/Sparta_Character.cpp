#include "Sparta_Character.h"
#include "Sparta_PlayerController.h"
#include "Sparta_GameState.h"
#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "Blueprint/UserWidget.h"
#include "BuffInfo.h"
#include "InteractableItem.h"

ASparta_Character::ASparta_Character()
{
	PrimaryActorTick.bCanEverTick = false;

	// 카메라 컴포넌트 초기화
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->TargetArmLength = 300.0f;
	SpringArmComp->bUsePawnControlRotation = true;

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;

	// UI 초기화
	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(GetMesh());
	OverheadWidget->SetWidgetSpace(EWidgetSpace::Screen);

	// 이동 속도 및 스프린트 기본값
	NormalSpeed = 600.0f;
	SprintSpeedMultiplier = 1.7f;
	SprintSpeed = NormalSpeed * SprintSpeedMultiplier;
	GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;

	MaxHealth = 100.0f;
	Health = MaxHealth;

	// 디버프 관련 초기값
	bSprinting = false;
	SpeedDebuffStack = 0;
	CurrentSpeedMultiplier = 1.0f;
	ReverseControlStack = 0;
	bIsControlReversed = false;
}

void ASparta_Character::BeginPlay()
{
	Super::BeginPlay();
	UpdateOverheadHP();
}

void ASparta_Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (ASparta_PlayerController* PlayerController = Cast<ASparta_PlayerController>(GetController()))
		{
			if (PlayerController->MoveAction)
			{
				EnhancedInput->BindAction(PlayerController->MoveAction, ETriggerEvent::Triggered,this, &ASparta_Character::Move);
			}

			if (PlayerController->JumpAction)
			{
				EnhancedInput->BindAction(PlayerController->JumpAction, ETriggerEvent::Triggered, this, &ASparta_Character::StartJump);
				EnhancedInput->BindAction(PlayerController->JumpAction, ETriggerEvent::Completed, this, &ASparta_Character::StopJump);
			}

			if (PlayerController->LookAction)
			{
				EnhancedInput->BindAction(PlayerController->LookAction, ETriggerEvent::Triggered, this, &ASparta_Character::Look);
			}

			if (PlayerController->SprintAction)
			{
				EnhancedInput->BindAction(PlayerController->SprintAction, ETriggerEvent::Triggered, this, &ASparta_Character::StartSprint);
				EnhancedInput->BindAction(PlayerController->SprintAction, ETriggerEvent::Completed, this, &ASparta_Character::StopSprint);
			}

			if (PlayerController->InteractAction)
			{
				EnhancedInput->BindAction(
					PlayerController->InteractAction,
					ETriggerEvent::Completed,
					this,
					&ASparta_Character::TryInteract
				);
			}


		 }
	}
}

void ASparta_Character::Move(const FInputActionValue& value)
{
	if (!Controller) return;

	FVector2D MoveInput = value.Get<FVector2D>();

	// 컨트롤 반전 디버프 적용
	if (bIsControlReversed)
	{
		MoveInput *= -1.f;
	}

	if (!FMath::IsNearlyZero(MoveInput.X))
	{
		AddMovementInput(GetActorForwardVector(), MoveInput.X);
	}

	if (!FMath::IsNearlyZero(MoveInput.Y))
	{
		AddMovementInput(GetActorRightVector(), MoveInput.Y);
	}
}
void ASparta_Character::StartJump(const FInputActionValue& value)
{
	if (value.Get<bool>())
	{
		Jump();
	}
}

void ASparta_Character::StopJump(const FInputActionValue& value)
{
	if (!value.Get<bool>())
	{
		StopJumping();
	}
}

void ASparta_Character:: Look(const FInputActionValue& value)
{
	FVector2D LookInput = value.Get<FVector2D>();

	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void ASparta_Character::StartSprint(const FInputActionValue& value)
{
	bSprinting = true;
	UpdateCharacterSpeed();
}

void ASparta_Character::StopSprint(const FInputActionValue& value)
{
	bSprinting = false;
	UpdateCharacterSpeed();
}

void ASparta_Character::TryInteract(const FInputActionValue& value)
{
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FVector Start = GetActorLocation();
	FVector End = Start + GetActorForwardVector() * 100.f;

	float SphereRadius = 50.f;

	FCollisionShape Sphere = FCollisionShape::MakeSphere(SphereRadius);
	TArray<FHitResult> HitResults;

	bool HitDetected = GetWorld()->SweepMultiByChannel(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ECC_Visibility,
		Sphere,
		Params
	);


	// Sphere 스윕 디버그 드로잉
	DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 2.0f, 0, 2.0f);
	DrawDebugSphere(GetWorld(), Start, SphereRadius, 16, FColor::Blue, false, 2.0f);
	DrawDebugSphere(GetWorld(), Start, SphereRadius, 16, FColor::Red, false, 2.0f);

	if (HitDetected)
	{
		for (const FHitResult& Hit : HitResults)
		{
			if (AInteractableItem* Item = Cast<AInteractableItem>(Hit.GetActor()))
			{
				Item->OnInteract();
			}
		}
	}

}

float ASparta_Character::GetHealth() const
{
	return Health;
}

void ASparta_Character::AddHealth(float Amount)
{
	Health = FMath::Clamp(Health + Amount, 0.0f, MaxHealth);
	UpdateOverheadHP();
}

float ASparta_Character::TakeDamage(
	float DamageAmount,
	struct FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	Health = FMath::Clamp(Health - DamageAmount, 0.0f, MaxHealth);
	UpdateOverheadHP();

	if (Health <= 0.0f)
	{
		OnDeath();
	}
	
	return ActualDamage;
}

void ASparta_Character::OnDeath()
{
	ASparta_GameState* SpartaGameState = GetWorld() ? GetWorld()->GetGameState<ASparta_GameState>() : nullptr;
	if (SpartaGameState)
	{
		SpartaGameState->OnGameOver();
	}
}

void ASparta_Character::UpdateOverheadHP()
{
	if (UUserWidget* WidgetInstance = OverheadWidget->GetUserWidgetObject())
	{
		if (UProgressBar* HPBar = Cast<UProgressBar>(WidgetInstance->GetWidgetFromName(TEXT("HealthBar"))))
		{
			const float HPPercent = (MaxHealth > 0.f) ? Health / MaxHealth : 0.f;
			HPBar->SetPercent(HPPercent);

			// HP가 낮을 때 색상 변경
			if (HPPercent < 0.3f)
			{
				HPBar->SetFillColorAndOpacity(FLinearColor::Red);
			}
		}

		if (UTextBlock* HPText = Cast<UTextBlock>(WidgetInstance->GetWidgetFromName(TEXT("HPText"))))
		{
			HPText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Health, MaxHealth)));
		}
	
	}
}

// 이동 속도 디버프
void ASparta_Character::ApplySpeedDebuff(float Duration, float SpeedMultiplier)
{
	// 디버프 스택 증가
	SpeedDebuffStack++;

	// 적용 배율 갱신(더 강력한 디버프 우선)
	if (SpeedMultiplier < CurrentSpeedMultiplier)
	{
		CurrentSpeedMultiplier = SpeedMultiplier;
	}
	UpdateCharacterSpeed();

	FTimerHandle TempTimerHandle;
	GetWorldTimerManager().SetTimer(
		TempTimerHandle,
		this,
		&ASparta_Character::OnSpeedDebuffEnd,
		Duration,
		false
	);

	AddBuffInfoUI(TEXT("Speed Down"), Duration);
}

void ASparta_Character::OnSpeedDebuffEnd()
{
	SpeedDebuffStack = FMath::Max(0, SpeedDebuffStack - 1);
	if (SpeedDebuffStack == 0)
	{
		CurrentSpeedMultiplier = 1.0f;
		UpdateCharacterSpeed();
	}
}

// 입력 반전 디버프
void ASparta_Character::ApplyReverseControlsDebuff(float Duration)
{
	ReverseControlStack++;

	if (!bIsControlReversed)
	{
		bIsControlReversed = true;
	}

	FTimerHandle TempTimerHandle;
	GetWorldTimerManager().SetTimer(
		TempTimerHandle,
		this,
		&ASparta_Character::OnReverseControlsDebuffEnd,
		Duration,
		false
	);

	AddBuffInfoUI(TEXT("Reverse Control"), Duration);
}

void ASparta_Character::AddBuffInfoUI(const FString& BuffName, float Duration)
{
	if (ASparta_PlayerController* PC = Cast<ASparta_PlayerController>(GetController()))
	{
		UBuffInfo* ExistingBuff = nullptr;

		if (UUserWidget* HUDWidget = PC->GetHUDWidget())
		{
			UVerticalBox* BuffInfoStack = Cast<UVerticalBox>(HUDWidget->GetWidgetFromName(TEXT("BuffInfoStack")));
		
			if (!BuffInfoStack)
				return;
			
			// 버프 스택 UI의 자식 순회
			for (UWidget* Child : BuffInfoStack->GetAllChildren())
			{
				UBuffInfo* BuffWidget = Cast<UBuffInfo>(Child);

				if (BuffWidget && BuffWidget->GetBuffName() == BuffName)
				{
					ExistingBuff = BuffWidget;
					break;
				}
			}

			// 이미 해당 버프의 UI가 쌓여 있다면
			if (ExistingBuff)
			{
				ExistingBuff->ExtendBuffDuration(Duration);
			}
		
			else
			{
				UBuffInfo* BuffInfoInstance = CreateWidget<UBuffInfo>(PC, BuffInfoWidgetClass);

				// 새 버프의 UI 쌓기
				if (BuffInfoInstance)
				{
					BuffInfoStack->AddChild(BuffInfoInstance);
					BuffInfoInstance->InitializeBuffInfo(BuffName, Duration);
				}
			}
		}
	}
}

void ASparta_Character::OnReverseControlsDebuffEnd()
{
	ReverseControlStack = FMath::Max(0, ReverseControlStack - 1);
	if (ReverseControlStack == 0)
	{
		bIsControlReversed = false;
	}
}

// 스프린트 및 최종 속도 업데이트
void ASparta_Character::UpdateCharacterSpeed()
{
	const float BaseSpeed = bSprinting ? SprintSpeed : NormalSpeed;
	GetCharacterMovement()->MaxWalkSpeed = BaseSpeed * CurrentSpeedMultiplier;
}