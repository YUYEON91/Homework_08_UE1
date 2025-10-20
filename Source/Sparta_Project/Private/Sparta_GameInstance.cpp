#include "Sparta_GameInstance.h"

USparta_GameInstance::USparta_GameInstance()
{
	TotalScore = 0;
	CurrentLevelIndex = 0;
	CurrentWaveIndex = 0;
}

void USparta_GameInstance::AddToScore(int32 Amount)
{
	TotalScore += Amount;
	UE_LOG(LogTemp, Warning, TEXT("Total Score Updated : %d"), TotalScore);
}
