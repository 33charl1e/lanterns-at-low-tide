#include "VaultHUD.h"

#include "Algo/AnyOf.h"
#include "Engine/Canvas.h"
#include "VaultCourtyard.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
    constexpr float ViewWidth = 1280;
    constexpr float ViewHeight = 800;

    FLinearColor Ink(uint32 RGB)
    {
        return FLinearColor(FColor((RGB >> 16) & 255, (RGB >> 8) & 255, RGB & 255));
    }

    const FLinearColor Background = Ink(0x151b1a);
    const FLinearColor BoardBack = Ink(0x111b1e);
    const FLinearColor Stroke = Ink(0x4b574b);
    const FLinearColor Mint = Ink(0xe7bd78);
    const FLinearColor Amber = Ink(0xffbd6b);
    const FLinearColor Ocean = Ink(0x80dad6);

    const FBox2D GameArea({ 75, 185 }, { 779, 687 });
    const FIntPoint NoCell(-1, -1);

}

FIntPoint AVaultHUD::DirectionForInput(FKey Key)
{
    if (Key == EKeys::Up || Key == EKeys::W) { return { 0, -1 }; }
    if (Key == EKeys::Down || Key == EKeys::S) { return { 0, 1 }; }
    if (Key == EKeys::Left || Key == EKeys::A) { return { -1, 0 }; }
    if (Key == EKeys::Right || Key == EKeys::D) { return { 1, 0 }; }
    return FIntPoint::ZeroValue;
}

void AVaultHUD::BeginPlay()
{
    Super::BeginPlay();
    PrepareTypefaces();
    Draft = FVaultLevel::Blank({ 10, 8 });
    RefreshSlots();
    Courtyard = GetWorld()->SpawnActor<AVaultCourtyard>();
    Courtyard->Build(FVaultLevel::Campaign().Last(), 0);
}

void AVaultHUD::RefreshSlots()
{
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Slots); ++Index)
    {
        FVaultLevel Level;
        Slots[Index] = FVaultLevel::Load(Index, Level);
    }
}

void AVaultHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || Canvas->SizeX <= 0 || Canvas->SizeY <= 0)
    {
        return;
    }

    Scale = FMath::Min(Canvas->SizeX / ViewWidth, Canvas->SizeY / ViewHeight);
    Origin = (FVector2D(Canvas->SizeX, Canvas->SizeY) - FVector2D(ViewWidth, ViewHeight) * Scale) / 2;
    Mouse = MousePosition();
    Buttons.Reset();
    BeginJournalTextFrame();
    if (bPainting && Screen == EScreen::Workshop)
    {
        if (GetOwningPlayerController()->IsInputKeyDown(EKeys::LeftMouseButton))
        {
            Paint(Mouse, false);
        }
        else
        {
            bPainting = false;
        }
    }

    if (Screen == EScreen::Workshop)
    {
        DrawRect(Background, 0, 0, Canvas->SizeX, Canvas->SizeY);
    }
    else
    {
        PresentCourtyard();
    }
    DrawJournalChrome();

    switch (Screen)
    {
    case EScreen::Dispatch: DrawDispatch(); break;
    case EScreen::Play: DrawPlay(); break;
    case EScreen::Workshop: DrawWorkshop(); break;
    }
    FinishJournalTextFrame();
    bFrameReady = true;
}

void AVaultHUD::PresentCourtyard()
{
    const FVector2D Viewport(Canvas->SizeX, Canvas->SizeY);
    if (Screen == EScreen::Dispatch)
    {
        const FVaultLevel& Preview = FVaultLevel::Campaign().Last();
        const FVaultState State{ Preview.Start, Preview.Batteries };
        Courtyard->Present(Preview, State, FVector2D::ZeroVector, FVector2D::ZeroVector, NoCell, {0, 1}, 1.f, false,
            Viewport, Origin + FVector2D(386, 486) * Scale, Scale, true);
        return;
    }

    const bool bSkipping = BatteryTravel.SizeSquared() > 1;
    const float Duration = bSkipping ? .26f : .14f;
    const double Now = GetWorld()->GetRealTimeSeconds();
    const float Progress = PrevRobot == NoCell ? 1.f
        : FMath::Clamp(static_cast<float>((Now - MoveTime) / Duration), 0.f, 1.f);
    const float Remaining = 1.f - FMath::InterpEaseOut(0.f, 1.f, Progress, 2.f);
    const FVector2D Motion = FVector2D(Facing) * Remaining;
    Courtyard->Present(Board.Level, Board.State, Motion, FVector2D(BatteryTravel) * Remaining,
        MovedBattery, Facing, Progress, Now - ExchangeTime < .38,
        Viewport, Origin + FVector2D(418, 420) * Scale, Scale, false);
}

void AVaultHUD::DrawDraft()
{
    const FVector2D Area = GameArea.GetSize();
    CellSize = FMath::FloorToFloat(FMath::Min(Area.X / Draft.Size.X, Area.Y / Draft.Size.Y));
    const FVector2D Size = FVector2D(Draft.Size) * CellSize;
    GridOrigin = GameArea.GetCenter() - Size / 2;
    const bool bPowered = Algo::AnyOf(Draft.Batteries,
        [this](FIntPoint Battery) { return Draft.TileAt(Battery) == EVaultTile::Relay; });

    Rect(GridOrigin.X - 8, GridOrigin.Y - 8, Size.X + 16, Size.Y + 16, BoardBack);
    Frame(GridOrigin.X - 8, GridOrigin.Y - 8, Size.X + 16, Size.Y + 16, Stroke);
    for (int32 Y = 0; Y < Draft.Size.Y; ++Y)
    {
        for (int32 X = 0; X < Draft.Size.X; ++X)
        {
            const FIntPoint Position(X, Y);
            const FVector2D At = GridOrigin + FVector2D(Position) * CellSize;
            const EVaultTile Tile = Draft.TileAt(Position);
            const bool bBattery = Draft.Batteries.Contains(Position);
            DrawTile(Tile, At.X, At.Y, CellSize, bPowered, bPowered || bBattery || Position == Draft.Start);
            if (bBattery) { DrawBattery(At.X, At.Y, CellSize, Tile); }
            if (Position == Draft.Start) { DrawRobot(At.X, At.Y, CellSize); }
        }
    }

    const FVector2D At = (Mouse - GridOrigin) / CellSize;
    const FIntPoint Hovered(FMath::FloorToInt(At.X), FMath::FloorToInt(At.Y));
    if (Draft.Contains(Hovered))
    {
        Frame(GridOrigin.X + Hovered.X * CellSize, GridOrigin.Y + Hovered.Y * CellSize,
            CellSize, CellSize, Mint, 2);
    }
}

void AVaultHUD::DrawTile(EVaultTile Tile, float X, float Y, float S, bool bPowered, bool bGateOpen)
{
    Rect(X, Y, S - 1, S - 1, Ink(0x223033));
    switch (Tile)
    {
    case EVaultTile::Wall:
        Rect(X, Y, S - 1, S - 1, Ink(0x3e5355));
        Rect(X, Y + S * 0.79f, S - 1, S * 0.19f, Ink(0x2d3e40));
        Rect(X + 2, Y + 2, S - 5, S * 0.08f, Ink(0x5a7070));
        break;
    case EVaultTile::Socket:
        Frame(X + S * 0.2f, Y + S * 0.2f, S * 0.6f, S * 0.6f, Ocean, S * 0.05f);
        Rect(X + S * 0.44f, Y + S * 0.44f, S * 0.12f, S * 0.12f, Ocean);
        break;
    case EVaultTile::Relay:
        Frame(X + S * 0.1f, Y + S * 0.1f, S * 0.8f, S * 0.8f, bPowered ? Mint : Amber, S * 0.045f);
        for (int32 Stripe = 0; Stripe < 3; ++Stripe)
        {
            Rect(X + S * (0.22f + Stripe * 0.2f), Y + S * 0.29f, S * 0.11f, S * 0.42f,
                bPowered ? Mint : Ink(0x886541));
        }
        break;
    case EVaultTile::Gate:
        Rect(X + S * 0.06f, Y + S * 0.06f, S * 0.1f, S * 0.88f, bGateOpen ? Mint : Amber);
        Rect(X + S * 0.84f, Y + S * 0.06f, S * 0.1f, S * 0.88f, bGateOpen ? Mint : Amber);
        if (!bGateOpen)
        {
            for (int32 Bar = 0; Bar < 3; ++Bar)
            {
                Rect(X + S * 0.18f, Y + S * (0.22f + Bar * 0.23f), S * 0.64f, S * 0.09f, Amber);
            }
        }
        break;
    case EVaultTile::Water:
        Rect(X, Y, S - 1, S - 1, Ink(0x194859));
        Rect(X + S * .15f, Y + S * .33f, S * .62f, S * .08f, Ocean);
        Rect(X + S * .31f, Y + S * .59f, S * .55f, S * .08f, Ink(0x569f9f));
        break;
    case EVaultTile::Mirror:
        Rect(X + S * .17f, Y + S * .17f, S * .66f, S * .66f, Ink(0x4a436a));
        Frame(X + S * .23f, Y + S * .23f, S * .54f, S * .54f, Ink(0xc8a3ff), S * .05f);
        Rect(X + S * .44f, Y + S * .31f, S * .10f, S * .37f, Ink(0xc8a3ff));
        break;
    case EVaultTile::Floor:
        Rect(X + S * 0.48f, Y + S * 0.48f, S * 0.04f, S * 0.04f, Stroke);
        break;
    }
}

void AVaultHUD::DrawBattery(float X, float Y, float S, EVaultTile Tile)
{
    const bool bCharged = Tile == EVaultTile::Socket;
    Rect(X + S * 0.17f, Y + S * 0.23f, S * 0.69f, S * 0.66f, BoardBack);
    Rect(X + S * 0.15f, Y + S * 0.13f, S * 0.7f, S * 0.69f, bCharged ? Mint : Ink(0xe7a451));
    Frame(X + S * 0.24f, Y + S * 0.23f, S * 0.52f, S * 0.48f, bCharged ? Ink(0x5a813d) : Ink(0x88603b),
        S * 0.045f);
    Rect(X + S * 0.35f, Y + S * 0.41f, S * 0.30f, S * 0.12f, bCharged ? Background : Ink(0x644329));
    Rect(X + S * 0.45f, Y + S * 0.31f, S * 0.10f, S * 0.32f, bCharged ? Background : Ink(0x644329));
}

void AVaultHUD::DrawRobot(float X, float Y, float S)
{
    Rect(X + S * .22f, Y + S * .72f, S * .56f, S * .12f, BoardBack);
    Rect(X + S * .25f, Y + S * .44f, S * .50f, S * .34f, Ink(0xa94834));
    Rect(X + S * .33f, Y + S * .22f, S * .34f, S * .29f, Ink(0x85372a));
    Rect(X + S * .41f, Y + S * .29f, S * .22f, S * .17f, Ink(0xe9b587));
    Rect(X + S * .29f, Y + S * .52f, S * .42f, S * .08f, Ink(0xe4a453));
    Rect(X + S * .68f, Y + S * .49f, S * .12f, S * .20f, Ocean);
}

void AVaultHUD::Rect(float X, float Y, float W, float H, FLinearColor Color)
{
    DrawRect(Color, Origin.X + X * Scale, Origin.Y + Y * Scale, W * Scale, H * Scale);
}

void AVaultHUD::Frame(float X, float Y, float W, float H, FLinearColor Color, float Width)
{
    Rect(X, Y, W, Width, Color);
    Rect(X, Y + H - Width, W, Width, Color);
    Rect(X, Y, Width, H, Color);
    Rect(X + W - Width, Y, Width, H, Color);
}

FVector2D AVaultHUD::MousePosition() const
{
    float X, Y;
    if (const APlayerController* Controller = GetOwningPlayerController(); Controller && Controller->GetMousePosition(X, Y))
    {
        return (FVector2D(X, Y) - Origin) / Scale;
    }
    return { -1, -1 };
}
