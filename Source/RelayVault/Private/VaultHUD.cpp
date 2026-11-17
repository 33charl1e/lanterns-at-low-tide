#include "VaultHUD.h"

#include "Algo/AnyOf.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
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
    const FLinearColor Panel = Ink(0x1e2520);
    const FLinearColor BoardBack = Ink(0x111b1e);
    const FLinearColor Stroke = Ink(0x4b574b);
    const FLinearColor Text = Ink(0xfff2d9);
    const FLinearColor Muted = Ink(0xc5c9ae);
    const FLinearColor Mint = Ink(0xe7bd78);
    const FLinearColor Amber = Ink(0xffbd6b);
    const FLinearColor Ocean = Ink(0x80dad6);

    const FBox2D GameArea({ 75, 165 }, { 779, 687 });
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
        const FLinearColor Shade(Background.R, Background.G, Background.B, .96f);
        DrawRect(Shade, 0, 0, Canvas->SizeX, Origin.Y + 103 * Scale);
        const float FooterY = Origin.Y + 725 * Scale;
        DrawRect(Shade, 0, FooterY, Canvas->SizeX, Canvas->SizeY - FooterY);
    }
    Rect(50, 49, 30, 30, Mint);
    Rect(57, 56, 5, 16, Background);
    Rect(65, 56, 8, 5, Background);
    Rect(65, 67, 8, 5, Background);
    Label(TEXT("LANTERNS AT LOW TIDE"), 92, 47, 30, Text, true);
    Rect(50, 101, 1180, 1, Stroke);
    Rect(50, 725, 1180, 1, Stroke);

    switch (Screen)
    {
    case EScreen::Dispatch: DrawDispatch(); break;
    case EScreen::Play: DrawPlay(); break;
    case EScreen::Workshop: DrawWorkshop(); break;
    }
    bFrameReady = true;
}

void AVaultHUD::DrawDispatch()
{
    const TArray<FVaultLevel>& Levels = FVaultLevel::Campaign();
    Rect(52, 127, 665, 128, FLinearColor(Background.R, Background.G, Background.B, .90f));
    Rect(52, 127, 3, 128, Mint);
    const bool bEnding = Cleared.Contains(Levels.Num() - 1);
    Label(bEnding ? TEXT("Home Again") : TEXT("The Empty Ferry"), 73, 139, 51, Text, true);
    Paragraph(bEnding ? TEXT("The far shore answered with a bell. Then a light appeared on the water.")
        : TEXT("Mara's father left his lamp. The ferry never returned. Relight the shore beacons to call it home."),
        75, 200, 617, 20, Muted);
    Rect(737, 117, 492, 592, Panel);
    Frame(737, 117, 492, 592, Stroke);

    Label(TEXT("MARA'S JOURNEY"), 759, 136, 19, Mint);
    for (int32 Index = 0; Index < Levels.Num(); ++Index)
    {
        const FString Row = FString::Printf(TEXT("%02d    %s%s"), Index + 1, *Levels[Index].Title,
            Cleared.Contains(Index) ? TEXT("   /   Clear") : TEXT(""));
        Button(Row, 759, 174 + Index * 40, 469, 36, EAction::Campaign, Index, Index == 0);
    }

    Label(TEXT("WORKSHOP"), 759, 473, 19, Mint);
    Button(TEXT("Open level editor"), 759, 514, 469, 51, EAction::Workshop);
    Label(TEXT("PLAY SAVED LEVEL"), 759, 588, 17, Muted);
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Slots); ++Index)
    {
        Button(FString::Printf(TEXT("Slot %d"), Index + 1), 759 + Index * 159, 620, 149, 46,
            EAction::Custom, Index, false, Slots[Index]);
    }
    Label(TEXT("Enter  Start campaign       E  Open workshop"), 52, 746, 18, Muted);
}

void AVaultHUD::DrawPlay()
{
    const FVaultLevel& Level = Board.Level;
    const FVaultState& State = Board.State;
    const bool bComplete = Board.IsComplete();
    const bool bHasRelay = Level.Tiles.Contains(EVaultTile::Relay);
    const bool bPowered = Board.IsPowered();
    const bool bHasMirrors = Level.Tiles.Contains(EVaultTile::Mirror);
    const bool bHasWater = Level.Tiles.Contains(EVaultTile::Water);
    const bool bEnding = Stage == FVaultLevel::Campaign().Num() - 1;

    Rect(824, 132, 405, 577, Panel);
    Frame(824, 132, 405, 577, Stroke);
    const FString Mode = bTesting ? TEXT("WORKSHOP PLAYTEST")
        : (Stage == INDEX_NONE ? TEXT("CUSTOM LEVEL") : FString::Printf(TEXT("CHAPTER  %02d / %02d"),
            Stage + 1, FVaultLevel::Campaign().Num()));
    Label(Mode, 849, 156, 17, Mint);
    Label(Level.Title, 849, 191, 35, Text, true);
    Rect(849, 254, 355, 1, Stroke);

    Label(TEXT("BEACONS"), 849, 277, 15, Muted);
    Label(TEXT("MOVES"), 982, 277, 15, Muted);
    Label(TEXT("PUSHES"), 1097, 277, 15, Muted);
    Label(FString::Printf(TEXT("%d / %d"), Board.ChargedCount(), State.Batteries.Num()),
        849, 302, 29, Mint, true);
    Label(FString::FromInt(State.Moves), 982, 302, 29, Text, true);
    Label(FString::FromInt(State.Pushes), 1097, 302, 29, Text, true);
    Rect(849, 359, 355, 1, Stroke);

    if (bComplete)
    {
        Label(bEnding ? TEXT("The ferry answers") : TEXT("Beacon lit"), 849, 390, 35, Mint, true);
        Paragraph(bEnding ? TEXT("A bell rings across the water. A lantern swings at the prow.")
            : TEXT("A new light marks the way home."), 849, 447, 355, 19, Text);
        const bool bHasNext = Stage != INDEX_NONE && FVaultLevel::Campaign().IsValidIndex(Stage + 1);
        Button(bTesting ? TEXT("Return to editor   [Enter]") : (bHasNext ? TEXT("Next chapter   [Enter]")
            : TEXT("Return to dispatch   [Enter]")), 849, 507, 355, 47, EAction::Advance, 0, true);
    }
    else
    {
        Rect(849, 376, 355, 40, BoardBack);
        Rect(862, 390, 10, 10, bHasRelay ? (bPowered ? Mint : Amber) : Ocean);
        Label(bHasRelay ? (bPowered ? TEXT("GATE HELD") : TEXT("GATE SHUT"))
            : (bHasMirrors ? TEXT("TIDE MIRRORS") : (bHasWater ? TEXT("TIDAL CHANNEL") : TEXT("FERRY LANDING"))),
            881, 387, 17, bHasRelay && bPowered ? Mint : Text);
        if (bHasMirrors)
        {
            Button(TEXT("Exchange [Q]"), 1028, 377, 175, 38, EAction::Exchange, 0,
                Board.CanExchange(), Board.CanExchange());
        }
        Paragraph(Level.Hint, 849, 428, 355, 18, Text);
        if (!Level.Journal.IsEmpty())
        {
            Label(TEXT("FERRY LOG"), 849, 500, 13, Mint);
            Paragraph(Level.Journal, 849, 521, 355, 17, Muted);
        }
    }
    Button(TEXT("Undo   [Z]"), 849, 574, 171, 42, EAction::Undo, 0, false, State.Moves > 0);
    Button(TEXT("Restart   [R]"), 1031, 574, 173, 42, EAction::Reset);
    Button(bTesting ? TEXT("Back to editor   [Esc]") : TEXT("Back to dispatch   [Esc]"),
        849, 638, 355, 43, EAction::Back);
    Label(bHasMirrors ? TEXT("WASD / Arrows   Move      Q   Exchange at a tide mirror")
        : TEXT("WASD / Arrows   Move"), 52, 746, 18, Muted);
}

void AVaultHUD::DrawWorkshop()
{
    Rect(51, 132, 752, 577, Panel);
    Frame(51, 132, 752, 577, Stroke);
    DrawDraft();

    Rect(824, 132, 405, 577, Panel);
    Frame(824, 132, 405, 577, Stroke);
    Label(TEXT("Level workshop"), 849, 150, 37, Text, true);

    const TCHAR* Names[] = { TEXT("Floor"), TEXT("Wall"), TEXT("Socket"), TEXT("Relay"),
        TEXT("Gate"), TEXT("Water"), TEXT("Mirror"), TEXT("Lantern"), TEXT("Mara") };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
    {
        Button(Names[Index], 849 + (Index % 2) * 180, 197 + (Index / 2) * 35, 171, 31,
            EAction::Tool, Index, static_cast<int32>(Tool) == Index);
    }
    Button(TEXT("Clear grid"), 1029, 337, 171, 31, EAction::Clear);
    Rect(849, 376, 351, 1, Stroke);

    const auto Dimension = [this](int32 X, const TCHAR* Name, int32 Size, EAction Action)
    {
        Label(FString::Printf(TEXT("%s %d"), Name, Size), X, 393, 16, Muted);
        Button(TEXT("-"), X + 80, 385, 38, 33, Action, -1, false, Size > FVaultLevel::MinSide);
        Button(TEXT("+"), X + 125, 385, 38, 33, Action, 1, false, Size < FVaultLevel::MaxSide);
    };
    Dimension(849, TEXT("Width"), Draft.Size.X, EAction::Width);
    Dimension(1033, TEXT("Height"), Draft.Size.Y, EAction::Height);

    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Slots); ++Index)
    {
        const FString Name = FString::Printf(TEXT("Slot %d%s"), Index + 1, Slots[Index] ? TEXT(" *") : TEXT(""));
        Button(Name, 849 + Index * 120, 434, 111, 34, EAction::Slot, Index, Slot == Index);
    }

    Button(TEXT("Save slot"), 849, 483, 171, 39, EAction::Save);
    Button(TEXT("Load slot"), 1029, 483, 171, 39, EAction::Open, 0, false, Slots[Slot]);
    Button(TEXT("Playtest   [Enter]"), 849, 538, 351, 47, EAction::Test, 0, true);
    Label(Notice, 849, 600, 16, bError ? Amber : Mint);
    Button(TEXT("Back to dispatch   [Esc]"), 849, 645, 351, 36, EAction::Back);
    Label(TEXT("Click or drag  Paint      Right click  Erase      Z  Undo edit"),
        52, 746, 18, Muted);
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
    Rect(X + S * 0.25f, Y + S * 0.31f, S * 0.58f, S * 0.54f, BoardBack);
    Rect(X + S * 0.21f, Y + S * 0.23f, S * 0.58f, S * 0.54f, Text);
    Rect(X + S * 0.11f, Y + S * 0.37f, S * 0.10f, S * 0.25f, Muted);
    Rect(X + S * 0.79f, Y + S * 0.37f, S * 0.10f, S * 0.25f, Muted);
    Rect(X + S * 0.29f, Y + S * 0.33f, S * 0.42f, S * 0.29f, Background);
    Rect(X + S * (0.43f + Facing.X * 0.1f), Y + S * (0.42f + Facing.Y * 0.045f),
        S * 0.14f, S * 0.11f, Ocean);
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

void AVaultHUD::Label(const FString& Value, float X, float Y, float Height, FLinearColor Color, bool bHeading)
{
    if (Value.IsEmpty())
    {
        return;
    }
    UFont* Font = bHeading ? GEngine->GetLargeFont() : GEngine->GetMediumFont();
    FCanvasTextItem Item(Origin + FVector2D(X, Y) * Scale, FText::FromString(Value), Font, Color);
    const float TextScale = Height * Scale / FMath::Max(1.f, Font->GetMaxCharHeight());
    Item.Scale = FVector2D(TextScale, TextScale);
    Canvas->DrawItem(Item);
}

void AVaultHUD::Paragraph(const FString& Value, float X, float Y, float Width, float Height, FLinearColor Color)
{
    UFont* Font = GEngine->GetMediumFont();
    const float FontScale = Height / FMath::Max(1.f, Font->GetMaxCharHeight());
    TArray<FString> Words;
    Value.ParseIntoArrayWS(Words);
    FString Line;
    for (const FString& Word : Words)
    {
        const FString Candidate = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
        float MeasuredWidth, MeasuredHeight;
        GetTextSize(Candidate, MeasuredWidth, MeasuredHeight, Font, FontScale);
        if (MeasuredWidth > Width && !Line.IsEmpty())
        {
            Label(Line, X, Y, Height, Color);
            Y += Height * 1.16f;
            Line = Word;
        }
        else
        {
            Line = Candidate;
        }
    }
    Label(Line, X, Y, Height, Color);
}

void AVaultHUD::Button(const FString& Value, float X, float Y, float W, float H, EAction Action,
    int32 Argument, bool bAccent, bool bEnabled)
{
    const FBox2D Bounds({ X, Y }, { X + W, Y + H });
    const bool bHover = bEnabled && Bounds.IsInside(Mouse);
    FLinearColor Fill = bAccent ? Mint : BoardBack;
    if (bHover) { Fill = bAccent ? Ink(0xdcffa9) : Ink(0x304744); }
    Rect(X, Y, W, H, Fill);
    if (!bAccent)
    {
        Frame(X, Y, W, H, bHover ? Ocean : Stroke);
    }
    Label(Value, X + 12, Y + (H - 19) / 2, 19, bAccent ? Background : (bEnabled ? Text : Ink(0x708482)));
    if (bEnabled)
    {
        Buttons.Add({ Bounds, Action, Argument });
    }
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
