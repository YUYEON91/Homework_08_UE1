 #pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Sparta_Character.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UWidgetComponent;
class UUserWidget;

struct FInputActionValue;

UCLASS()
class SPARTA_PROJECT_API ASparta_Character : public ACharacter
{
	GENERATED_BODY()

public:
	ASparta_Character();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	USpringArmComponent* SpringArmComp;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* CameraComp;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* OverheadWidget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	TSubclassOf<UUserWidget> BuffInfoWidgetClass;

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const;
	UFUNCTION(BlueprintCallable, Category = "Health")
	void AddHealth(float Amount);


protected:
	// Health 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float Health;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//생명주기, 데미지 관련
	virtual void BeginPlay() override;
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, 
		AActor* DamageCauser) override;

	void OnDeath();
	void UpdateOverheadHP();

	UFUNCTION()
	void Move(const FInputActionValue& value);
	UFUNCTION()
	void StartJump(const FInputActionValue& value);
	UFUNCTION()
	void StopJump(const FInputActionValue& value);
	UFUNCTION()
	void Look(const FInputActionValue& value);
	UFUNCTION()
	void StartSprint(const FInputActionValue& value);
	UFUNCTION()
	void StopSprint(const FInputActionValue& value);
	UFUNCTION()
	void TryInteract(const FInputActionValue& value);


	// 디버프(이동 속도 및 컨트롤 반전)
public:
	// HealingItem 사용 시 : 5초간 이동속도 50% 감소
	UFUNCTION(BlueprintCallable, Category = "Debuff")
	void ApplySpeedDebuff(float Duration, float SpeedMultiplier);

	// MineItem 폭발 시 : 5초간 이동 입력 반전
	UFUNCTION(BlueprintCallable, Category = "Debuff")
	void ApplyReverseControlsDebuff(float Duration);

	void AddBuffInfoUI(const FString& BuffName, float Duration);

protected:
	UFUNCTION()
	void OnSpeedDebuffEnd();

	UFUNCTION()
	void OnReverseControlsDebuffEnd();

	// 스프린트 및 디버프 속도 계산
	void UpdateCharacterSpeed();

private:
	// 기본 및 스프린트 속도
	float NormalSpeed;
	float SprintSpeedMultiplier;
	float SprintSpeed;

	// 스프린트 여부
	bool bSprinting;

	// 이동 속도 디버프 관련 (배율은 0.0 ~ 1.0)
	int32 SpeedDebuffStack;
	float CurrentSpeedMultiplier;

	// 컨트롤 반전 디버프 관련
	int32 ReverseControlStack;
	bool bIsControlReversed;
};
