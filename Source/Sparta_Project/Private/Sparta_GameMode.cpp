#include "Sparta_GameMode.h"
#include "Sparta_Character.h"
#include "Sparta_PlayerController.h"
#include "Sparta_GameState.h"

ASparta_GameMode::ASparta_GameMode()
{
	DefaultPawnClass = ASparta_Character::StaticClass();
	PlayerControllerClass = ASparta_PlayerController::StaticClass();
	GameStateClass = ASparta_GameState::StaticClass();
} 
