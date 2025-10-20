#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Sparta_GameInstance.generated.h"

UCLASS()
class SPARTA_PROJECT_API USparta_GameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	USparta_GameInstance();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GameData")
	int32 TotalScore;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GameData")
	int32 CurrentLevelIndex;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GameData")
	int32 CurrentWaveIndex;

	UFUNCTION(BlueprintCallable, Category = "GameData")
	void AddToScore(int32 Amount);
};