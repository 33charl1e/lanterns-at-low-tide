#include "RelayVaultGameMode.h"

#include "GameFramework/PlayerInput.h"
#include "VaultHUD.h"

ARelayVaultGameMode::ARelayVaultGameMode()
{
    DefaultPawnClass = nullptr;
    PlayerControllerClass = AVaultController::StaticClass();
    HUDClass = AVaultHUD::StaticClass();
}

void AVaultController::BeginPlay()
{
    Super::BeginPlay();
    bShowMouseCursor = true;
    FInputModeGameAndUI Mode;
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
}

bool AVaultController::InputKey(const FInputKeyParams& Params)
{
    const bool bHandled = Super::InputKey(Params);
    if (AVaultHUD* Vault = GetHUD<AVaultHUD>())
    {
        if (Vault->HandleInput(Params.Key, Params.Event))
        {
            return true;
        }
    }
    return bHandled;
}
