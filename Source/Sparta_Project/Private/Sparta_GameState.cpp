#include "Sparta_GameState.h"
#include "Sparta_GameInstance.h"
#include "Sparta_PlayerController.h"
#include "SpawnVolume.h"
#include "CoinItem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h"
#include "SpikeTrap.h" 

ASparta_GameState::ASparta_GameState()
{
	Score = 0;
	SpawnedCoinCount = 0;
	CollectedCoinCount = 0;
	CurrentLevelIndex = 0;
	MaxLevels = 3;
	CurrentWaveIndex = 0;
	MaxWaves = 3;
	WaveDuration = 20.f;
	ItemsToSpawnPerWave = { 20, 30, 40 };
	SpikeTrapClass = nullptr;
}

void ASparta_GameState::BeginPlay()
{
	Super::BeginPlay();

	StartLevel();

	GetWorldTimerManager().SetTimer(
		HUDUpdateTimerHandle,
		this,
		&ASparta_GameState::UpdateHUD,
		0.1f,
		true
	);
}

int32 ASparta_GameState::GetScore() const
{
	return Score;
}

void ASparta_GameState::AddScore(int32 Amount)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		USparta_GameInstance* SpartaGameInstance = Cast<USparta_GameInstance>(GameInstance);
		if (SpartaGameInstance)
		{
			SpartaGameInstance->AddToScore(Amount);
		}
	}
}

void ASparta_GameState::StartLevel()
{
	if (ASparta_PlayerController* SpartaPlayerController = GetSpartaPlayerController())
	{
		SpartaPlayerController->ShowGameHUD();
	}
	

	if (USparta_GameInstance* SpartaGameInstance = GetSpartaGameInstance())
	{
		CurrentLevelIndex = SpartaGameInstance->CurrentLevelIndex;
	}

	CurrentWaveIndex = 0;
	StartWave();
}

void ASparta_GameState::StartWave()
{
	// 코인 카운트 초기화
	SpawnedCoinCount = 0;
	CollectedCoinCount = 0;

	// 이전 Wave 아이템 제거
	for (AActor* Item : CurrentWaveItems)
	{
		if (Item && Item->IsValidLowLevelFast())
		{
			Item->Destroy();
		}
	}

	CurrentWaveItems.Empty();

	// // 이번 Wave에 스폰할 아이템 개수 결정
	int32 ItemToSpawn = (ItemsToSpawnPerWave.IsValidIndex(CurrentWaveIndex)) ? ItemsToSpawnPerWave[CurrentWaveIndex] : 20;

	// SpawnVolume을 이용해 아이템 스폰
	if (ASpawnVolume* SpawnVolume = GetSpawnVolume())
	{
		for (int32 i = 0; i < ItemToSpawn; i++)
		{
			if (AActor* SpawnedActor = SpawnVolume->SpawnRandomItem())
			{
				if (SpawnedActor->IsA(ACoinItem::StaticClass()))
				{
					SpawnedCoinCount++;
				}

				CurrentWaveItems.Add(SpawnedActor);

			}
		}

	}

	// Wave별 환경 변화
	if (CurrentWaveIndex == 1)
	{
		EnableWave2();
	}
	else if (CurrentWaveIndex == 2)
	{
		EnableWave3();
	}

	if (ASparta_PlayerController* SpartaPlayerController = GetSpartaPlayerController())
	{
		if (UUserWidget* HUDWidget = SpartaPlayerController->GetHUDWidget())
		{
			UFunction* PlayAnimFunc = HUDWidget->FindFunction(FName("PlayShowWaveNotifyAnim"));
		
			if (PlayAnimFunc)
			{
				HUDWidget->ProcessEvent(PlayAnimFunc, nullptr);
			}

			if (UTextBlock* WaveNotifyText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName("WaveNotifyText")))
			{
				WaveNotifyText->SetText(FText::FromString(
					FString::Printf(TEXT("Wave %d 발생!"), CurrentWaveIndex + 1)));
			}
		}
	}	

	// Wave 타이머 시작
	GetWorldTimerManager().SetTimer(
		WaveTimerHandle,
		this,
		&ThisClass::OnWaveTimeUp,
		WaveDuration,
		false
	);

	UE_LOG(LogTemp, Warning, TEXT("Wave %d Started"), CurrentWaveIndex + 1);
}

void ASparta_GameState::OnWaveTimeUp()
{
	EndWave();
}

void ASparta_GameState::EndWave()
{
	// 남은 Wave 타이머 제거
	GetWorldTimerManager().ClearTimer(WaveTimerHandle);

	// Wave3에서 활성화한 코인 이동 비활성화
	if (CurrentWaveIndex == 2)
	{
		SetAllCoinsMove(false);
	}

	// 다음 Wave 진행 또는 Level 종료
	++CurrentWaveIndex;
	if (CurrentWaveIndex >= MaxWaves)
	{
		EndLevel();
	}
	else
	{
		StartWave();
	}
}

void ASparta_GameState::OnCoinCollected()
{
	CollectedCoinCount++;
	UE_LOG(LogTemp, Warning, TEXT("Coin Collected: %d / %d"), CollectedCoinCount, SpawnedCoinCount)
	
	if (SpawnedCoinCount > 0 && CollectedCoinCount >= SpawnedCoinCount)
	{
		EndWave();
	}
}

void ASparta_GameState::EndLevel()
{
	GetWorldTimerManager().ClearTimer(WaveTimerHandle);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		USparta_GameInstance* SpartaGameInstance = Cast<USparta_GameInstance>(GameInstance);
		if (SpartaGameInstance)
		{
			AddScore(Score);
			CurrentLevelIndex++;
			SpartaGameInstance->CurrentLevelIndex = CurrentLevelIndex;
		}
	}

	if (CurrentLevelIndex >= MaxLevels)
	{
		OnGameOver();
		return;
	}

	if (LevelMapNames.IsValidIndex(CurrentLevelIndex))
	{
		UGameplayStatics::OpenLevel(GetWorld(), LevelMapNames[CurrentLevelIndex]);
	}

	else
	{
		OnGameOver();
	}
}

void ASparta_GameState::OnGameOver()
{
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (ASparta_PlayerController* SpartaPlayerController = Cast<ASparta_PlayerController>(PlayerController))
		{
			SpartaPlayerController->SetPause(true);
			SpartaPlayerController->ShowMainMenu(true);
		}
	}
}

void ASparta_GameState::UpdateHUD()
{
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (ASparta_PlayerController* Sparta_PlayerController = Cast<ASparta_PlayerController>(PlayerController))
		{
			if (UUserWidget* HUDWidget = Sparta_PlayerController->GetHUDWidget())
			{
				// 남은 시간 업데이트
				if (UTextBlock* WaveTimeText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Time"))))
				{
					float RemainingWaveTime = GetWorldTimerManager().GetTimerRemaining(WaveTimerHandle);
					WaveTimeText->SetText(FText::FromString(FString::Printf(TEXT("Time : %.1f"), RemainingWaveTime)));
				}

				// Score 업데이트
				if (UTextBlock* ScoreText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Score"))))
				{
					if (UGameInstance* GameInstance = GetGameInstance())
					{
						USparta_GameInstance* Sparta_GameInstance = Cast<USparta_GameInstance>(GameInstance);
						if (Sparta_GameInstance)
						{
							ScoreText->SetText(FText::FromString(FString::Printf(TEXT("Score : %d"), Sparta_GameInstance->TotalScore)));
						}
					}
				}

				// Level 업데이트
				if (UTextBlock* LevelIndexText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Level"))))
				{
					LevelIndexText->SetText(FText::FromString(FString::Printf(TEXT("Level : %d"), CurrentLevelIndex + 1)));
				}

				// Wave 업데이트
				if (UTextBlock* WaveText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Wave"))))
				{
					WaveText->SetText(FText::FromString(FString::Printf(TEXT("Wave : %d / %d"), CurrentWaveIndex + 1, MaxWaves)));
				}
			}
		}
	}
}

void ASparta_GameState::EnableWave2()
{
	//Wave 2 : 스파이크 트랩
	const FString Msg = TEXT("Wave 2 : Spawning and Activating 15 Spike Traps!");
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, Msg);
	}

	if (ASpawnVolume* SpawnVolume = GetSpawnVolume())
	{
		if (SpikeTrapClass)
		{
			for (int32 i = 0; i < 15; i++)
			{
				if (AActor* SpawnedActor = SpawnVolume->SpawnItem(SpikeTrapClass))
				{
					if (ASpikeTrap* Trap = Cast<ASpikeTrap>(SpawnedActor))
					{
						Trap->ActivateTrap();
					}
				}
			}
		}
	}
}

void ASparta_GameState::EnableWave3()
{
	//Wave 3 : 움직이는 코인
	const FString Msg = TEXT("Wave 3 : Spawning coins that orbit around!");
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, Msg);
	}

	SetAllCoinsMove(true);
}

void ASparta_GameState::SetAllCoinsMove(bool bActive)
{
	for (AActor* CoinActor : CurrentWaveItems)
	{
		if (ACoinItem* Coin = Cast<ACoinItem>(CoinActor))
		{
			Coin->SetWave3MoveActive(bActive);
		}
	}
}

ASpawnVolume* ASparta_GameState::GetSpawnVolume() const
{
	TArray<AActor*> FoundVolumes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnVolume::StaticClass(), FoundVolumes);
	return (FoundVolumes.Num() > 0) ? Cast<ASpawnVolume>(FoundVolumes[0]) : nullptr;
}

ASparta_PlayerController* ASparta_GameState::GetSpartaPlayerController() const
{
	return Cast<ASparta_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
}

USparta_GameInstance* ASparta_GameState::GetSpartaGameInstance() const
{
	return Cast<USparta_GameInstance>(GetGameInstance());
}