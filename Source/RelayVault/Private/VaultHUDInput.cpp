#include "VaultHUD.h"

#include "Engine/World.h"
#include "VaultCourtyard.h"

bool AVaultHUD::HandleInput(FKey Key, EInputEvent Event)
{
    if (Key == EKeys::LeftMouseButton && Event == IE_Released)
    {
        bPainting = false;
        return false;
    }
    if (Event != IE_Pressed && Event != IE_Repeat)
    {
        return false;
    }

    const FIntPoint Direction = DirectionForInput(Key);
    if (Screen == EScreen::Play && Direction != FIntPoint::ZeroValue)
    {
        const FIntPoint OldRobot = Board.State.Player;
        FIntPoint Landing;
        if (Board.Move(Direction, &Landing))
        {
            Facing = Direction;
            PrevRobot = OldRobot;
            MoveTime = GetWorld()->GetRealTimeSeconds();
            MovedBattery = Landing;
            BatteryTravel = Landing.X == INDEX_NONE ? FIntPoint::ZeroValue : Landing - Board.State.Player;
            RegisterClear();
        }
        return true;
    }
    if (Event == IE_Repeat)
    {
        return false;
    }
    if (bFrameReady && (Key == EKeys::LeftMouseButton || Key == EKeys::RightMouseButton))
    {
        const FVector2D Position = MousePosition();
        if (Key == EKeys::LeftMouseButton)
        {
            for (const FButton& Item : Buttons)
            {
                if (Item.Bounds.IsInside(Position))
                {
                    Perform(Item.Action, Item.Value);
                    return true;
                }
            }
        }
        if (Screen == EScreen::Workshop)
        {
            bPainting = Key == EKeys::LeftMouseButton;
            bStrokeChanged = false;
            LastPaint = { -1, -1 };
            Paint(Position, !bPainting);
            return true;
        }
    }
    if (Key == EKeys::Escape || Key == EKeys::BackSpace)
    {
        if (Screen == EScreen::Dispatch)
        {
            return false;
        }
        Perform(EAction::Back);
        return true;
    }
    if (Key == EKeys::Enter)
    {
        switch (Screen)
        {
        case EScreen::Dispatch: Perform(EAction::Campaign); break;
        case EScreen::Workshop: Perform(EAction::Test); break;
        case EScreen::Play: Perform(EAction::Advance); break;
        }
        return true;
    }
    if (Key == EKeys::E && Screen == EScreen::Dispatch)
    {
        Perform(EAction::Workshop);
        return true;
    }
    if (Key == EKeys::Z && Screen != EScreen::Dispatch)
    {
        Perform(EAction::Undo);
        return true;
    }
    if (Key == EKeys::Q && Screen == EScreen::Play && Board.Level.Tiles.Contains(EVaultTile::Mirror))
    {
        Perform(EAction::Exchange);
        return true;
    }
    if (Key == EKeys::R && Screen == EScreen::Play)
    {
        Perform(EAction::Reset);
        return true;
    }
    return false;
}

void AVaultHUD::RegisterClear()
{
    if (Stage != INDEX_NONE && Board.IsComplete())
    {
        Cleared.Add(Stage);
    }
}

void AVaultHUD::ChangeScreen(EScreen Next)
{
    Screen = Next;
    bPainting = false;
    bFrameReady = false;
    if (Next == EScreen::Workshop)
    {
        Courtyard->Hide();
    }
    else if (Next == EScreen::Play)
    {
        Courtyard->Build(Board.Level, FMath::Max(0, Stage));
    }
    else
    {
        Courtyard->Build(FVaultLevel::Campaign().Last(), 0);
    }
}

void AVaultHUD::BeginLevel(const FVaultLevel& Level, int32 CampaignIndex, bool bPlaytest)
{
    Board.Start(Level);
    Stage = CampaignIndex;
    bTesting = bPlaytest;
    ExchangeTime = -100;
    Facing = { 0, 1 };
    PrevRobot = { -1, -1 };
    ChangeScreen(EScreen::Play);
}

void AVaultHUD::Perform(EAction Action, int32 Value)
{
    switch (Action)
    {
    case EAction::Campaign:
        BeginLevel(FVaultLevel::Campaign()[Value], Value, false);
        break;
    case EAction::Custom:
    {
        FVaultLevel Level;
        if (FVaultLevel::Load(Value, Level))
        {
            BeginLevel(Level, INDEX_NONE, false);
        }
        else
        {
            RefreshSlots();
        }
        break;
    }
    case EAction::Workshop:
        ChangeScreen(EScreen::Workshop);
        break;
    case EAction::Back:
        ChangeScreen(Screen == EScreen::Play && bTesting ? EScreen::Workshop : EScreen::Dispatch);
        break;
    case EAction::Exchange:
        if (Board.Exchange())
        {
            PrevRobot = { INDEX_NONE, INDEX_NONE };
            ExchangeTime = GetWorld()->GetRealTimeSeconds();
            RegisterClear();
        }
        break;
    case EAction::Undo:
        if (Screen == EScreen::Play)
        {
            Board.Undo();
            ExchangeTime = -100;
            PrevRobot = { -1, -1 };
        }
        else if (Screen == EScreen::Workshop && !DraftHistory.IsEmpty())
        {
            Draft = DraftHistory.Pop();
            SetNotice(TEXT("已撤销上一步编辑。"));
        }
        break;
    case EAction::Reset:
        Board.Reset();
        ExchangeTime = -100;
        PrevRobot = { -1, -1 };
        Facing = { 0, 1 };
        break;
    case EAction::Advance:
        if (Board.IsComplete())
        {
            if (Stage != INDEX_NONE && FVaultLevel::Campaign().IsValidIndex(Stage + 1))
            {
                Perform(EAction::Campaign, Stage + 1);
            }
            else
            {
                Perform(EAction::Back);
            }
        }
        break;
    case EAction::Tool:
        Tool = static_cast<ETool>(Value);
        break;
    case EAction::Slot:
        Slot = Value;
        SetNotice(FString::Printf(TEXT("已选中存档 %d。"), Slot + 1));
        break;
    case EAction::Save:
    case EAction::Test:
    {
        const FString Error = Draft.Validate();
        if (!Error.IsEmpty())
        {
            SetNotice(Error, true);
        }
        else if (Action == EAction::Test)
        {
            FVaultLevel Level = Draft;
            Level.Title = TEXT("草稿试玩");
            Level.Hint = TEXT("把所有灯石推入航标。");
            BeginLevel(Level, INDEX_NONE, true);
        }
        else if (Draft.Save(Slot))
        {
            RefreshSlots();
            SetNotice(FString::Printf(TEXT("已保存到存档 %d。"), Slot + 1));
        }
        else
        {
            SetNotice(TEXT("存档写入失败。"), true);
        }
        break;
    }
    case EAction::Open:
    {
        FVaultLevel Level;
        if (FVaultLevel::Load(Slot, Level))
        {
            RememberDraft();
            Draft = MoveTemp(Level);
            SetNotice(FString::Printf(TEXT("已载入存档 %d。"), Slot + 1));
        }
        else
        {
            RefreshSlots();
            SetNotice(TEXT("此存档没有可用关卡。"), true);
        }
        break;
    }
    case EAction::Clear:
        RememberDraft();
        Draft = FVaultLevel::Blank(Draft.Size);
        SetNotice(TEXT("棋盘已清空，按 Z 可撤销。"));
        break;
    case EAction::Width:
        Resize(Draft.Size + FIntPoint(Value, 0));
        break;
    case EAction::Height:
        Resize(Draft.Size + FIntPoint(0, Value));
        break;
    }
}

void AVaultHUD::SetNotice(const FString& Value, bool bIsError)
{
    Notice = Value;
    bError = bIsError;
}

void AVaultHUD::RememberDraft()
{
    constexpr int32 MaxUndo = 128;
    if (DraftHistory.Num() == MaxUndo)
    {
        DraftHistory.RemoveAt(0);
    }
    DraftHistory.Add(Draft);
}

void AVaultHUD::Paint(FVector2D Position, bool bErase)
{
    if (CellSize <= 0)
    {
        return;
    }
    const FVector2D Offset = (Position - GridOrigin) / CellSize;
    const FIntPoint Cell(FMath::FloorToInt(Offset.X), FMath::FloorToInt(Offset.Y));
    if (!Draft.Contains(Cell) || Cell == LastPaint)
    {
        return;
    }
    LastPaint = Cell;

    FVaultLevel Edit = Draft;
    EVaultTile& Tile = Edit.Tiles[Edit.Index(Cell)];
    if (bErase)
    {
        if (Edit.Batteries.Remove(Cell) == 0)
        {
            if (Edit.Start == Cell) { Edit.Start = { -1, -1 }; }
            else { Tile = EVaultTile::Floor; }
        }
    }
    else if (Tool == ETool::Battery)
    {
        if (Tile == EVaultTile::Wall || Tile == EVaultTile::Water || Edit.Start == Cell)
        {
            SetNotice(TEXT("请将灯石放在干地上。"), true);
            return;
        }
        Edit.Batteries.Add(Cell);
    }
    else if (Tool == ETool::Robot)
    {
        if (Tile == EVaultTile::Wall || Tile == EVaultTile::Water || Edit.Batteries.Contains(Cell))
        {
            SetNotice(TEXT("请将玛拉放在空着的干地上。"), true);
            return;
        }
        Edit.Start = Cell;
    }
    else
    {
        Tile = static_cast<EVaultTile>(Tool);
        if (Tile == EVaultTile::Wall || Tile == EVaultTile::Water)
        {
            Edit.Batteries.Remove(Cell);
            if (Edit.Start == Cell) { Edit.Start = { -1, -1 }; }
        }
    }

    if (Edit.Tiles == Draft.Tiles && Edit.Start == Draft.Start && Edit.Batteries.Num() == Draft.Batteries.Num())
    {
        return;
    }
    if (bErase || !bStrokeChanged)
    {
        RememberDraft();
        bStrokeChanged = true;
    }
    Draft = MoveTemp(Edit);
    SetNotice(TEXT(""));
}

void AVaultHUD::Resize(FIntPoint NewSize)
{
    FVaultLevel Edit = FVaultLevel::Blank(NewSize);
    for (int32 Y = 0; Y < FMath::Min(NewSize.Y, Draft.Size.Y); ++Y)
    {
        for (int32 X = 0; X < FMath::Min(NewSize.X, Draft.Size.X); ++X)
        {
            Edit.Tiles[Edit.Index({ X, Y })] = Draft.TileAt({ X, Y });
        }
    }
    for (FIntPoint Battery : Draft.Batteries)
    {
        if (Edit.Contains(Battery)) { Edit.Batteries.Add(Battery); }
    }
    if (Edit.Contains(Draft.Start))
    {
        Edit.Start = Draft.Start;
    }
    RememberDraft();
    Draft = MoveTemp(Edit);
    bFrameReady = false;
    SetNotice(TEXT("已调整棋盘尺寸，按 Z 可撤销。"));
}
