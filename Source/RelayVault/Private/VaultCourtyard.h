#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VaultBoard.h"
#include "VaultCourtyard.generated.h"

class ACameraActor;
class UMaterialInterface;
class UProceduralMeshComponent;

UCLASS()
class AVaultCourtyard : public AActor
{
    GENERATED_BODY()

public:
    AVaultCourtyard();

    void Build(const FVaultLevel& Level, int32 Sector);
    void Present(const FVaultLevel& Level, const FVaultState& State, FVector2D Motion, FVector2D BatteryMotion,
        FIntPoint MovingBattery, FIntPoint Facing, float Step, bool bMirrorFlash, FVector2D Viewport,
        FVector2D BoardCenter, float UiScale, bool bTitle);
    void Hide();

protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    UPROPERTY() TObjectPtr<ACameraActor> Camera;
    UPROPERTY() TObjectPtr<UMaterialInterface> StoneMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> RuneMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> WaterMaterial;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Terrain;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Surrounds;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Basin;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Walker;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> LiveRelays;
    UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> Batteries;
    UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> GateBars;
    TArray<FIntPoint> GateCells;

    UProceduralMeshComponent* NewMesh(bool bShadows = true);
    void BuildBoard(const FVaultLevel& Level, int32 Sector);
    void BuildSurrounds(FIntPoint Size, int32 Sector);
    void BuildWalker();
    void BuildBattery(UProceduralMeshComponent& Component);
    void BuildGate(UProceduralMeshComponent& Component, bool bAcrossX);
    void Frame(FIntPoint Size, FVector2D Viewport, FVector2D BoardCenter, float UiScale, bool bTitle);
};
