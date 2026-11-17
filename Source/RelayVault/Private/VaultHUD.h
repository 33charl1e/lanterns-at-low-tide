#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "VaultBoard.h"
#include "VaultHUD.generated.h"

class AVaultCourtyard;

UCLASS()
class AVaultHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void DrawHUD() override;
    bool HandleInput(FKey Key, EInputEvent Event);

private:
    enum class EScreen : uint8 { Dispatch, Play, Workshop };
    enum class ETool : uint8 { Floor, Wall, Socket, Relay, Gate, Battery, Robot };
    enum class EAction : uint8
    {
        Campaign, Custom, Workshop, Back, Undo, Reset, Advance, Tool, Slot,
        Save, Open, Clear, Test, Width, Height
    };

    struct FButton
    {
        FBox2D Bounds;
        EAction Action;
        int32 Value;
    };

    EScreen Screen = EScreen::Dispatch;
    FVaultBoard Board;
    UPROPERTY() TObjectPtr<AVaultCourtyard> Courtyard;
    FVaultLevel Draft;
    TArray<FVaultLevel> DraftHistory;
    int32 Stage = 0;
    int32 Slot = 0;
    bool bTesting = false;
    bool bPainting = false;
    bool bStrokeChanged = false;
    bool bFrameReady = false;
    bool Slots[3] = {};
    bool Cleared[5] = {};
    ETool Tool = ETool::Wall;
    FString Notice;
    bool bError = false;
    FVector2D Origin = FVector2D::ZeroVector;
    FVector2D Mouse = FVector2D(-1, -1);
    float Scale = 1.f;
    TArray<FButton> Buttons;
    FVector2D GridOrigin = FVector2D::ZeroVector;
    float CellSize = 0.f;
    FIntPoint LastPaint = { -1, -1 };
    FIntPoint Facing = { 0, 1 };
    FIntPoint PrevRobot = { -1, -1 };
    FIntPoint MovedBattery = { -1, -1 };
    double MoveTime = 0;

    static FIntPoint DirectionForInput(FKey Key);
    void DrawDispatch();
    void DrawPlay();
    void DrawWorkshop();
    void DrawDraft();
    void PresentCourtyard();
    void DrawTile(EVaultTile Tile, float X, float Y, float Side, bool bPowered, bool bGateOpen);
    void DrawBattery(float X, float Y, float Side, EVaultTile Tile);
    void DrawRobot(float X, float Y, float Side);

    void Rect(float X, float Y, float W, float H, FLinearColor Color);
    void Frame(float X, float Y, float W, float H, FLinearColor Color, float Width = 1.f);
    void Label(const FString& Text, float X, float Y, float Height, FLinearColor Color, bool bHeading = false);
    void Button(const FString& Text, float X, float Y, float W, float H, EAction Action,
        int32 Value = 0, bool bAccent = false, bool bEnabled = true);
    void Perform(EAction Action, int32 Value = 0);
    void ChangeScreen(EScreen Next);
    void RefreshSlots();
    void BeginLevel(const FVaultLevel& Level, int32 CampaignIndex, bool bPlaytest);
    FVector2D MousePosition() const;
    void Paint(FVector2D Position, bool bErase);
    void Resize(FIntPoint NewSize);
    void RememberDraft();
    void SetNotice(const FString& Text, bool bIsError = false);
};
