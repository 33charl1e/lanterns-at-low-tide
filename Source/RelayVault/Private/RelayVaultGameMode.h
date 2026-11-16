#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "RelayVaultGameMode.generated.h"

UCLASS()
class ARelayVaultGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ARelayVaultGameMode();
};

UCLASS()
class AVaultController : public APlayerController
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;
    virtual bool InputKey(const FInputKeyParams& Params) override;
};
