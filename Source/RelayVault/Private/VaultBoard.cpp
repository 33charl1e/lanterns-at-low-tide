#include "VaultBoard.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
    FString SlotPath(int32 Slot)
    {
        return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("VaultLevels"),
            FString::Printf(TEXT("slot-%d.json"), Slot + 1));
    }

    FVaultLevel FromRows(const TCHAR* Title, const TCHAR* Hint, const TCHAR* Journal,
        std::initializer_list<const TCHAR*> Rows)
    {
        const FIntPoint Size(FCString::Strlen(*Rows.begin()), static_cast<int32>(Rows.size()));
        FVaultLevel Level = FVaultLevel::Blank(Size);
        Level.Title = Title;
        Level.Hint = Hint;
        Level.Journal = Journal;
        int32 Y = 0;
        for (const TCHAR* Row : Rows)
        {
            check(FCString::Strlen(Row) == Size.X);
            for (int32 X = 0; X < Size.X; ++X)
            {
                const FIntPoint Position(X, Y);
                EVaultTile& Tile = Level.Tiles[Level.Index(Position)];
                switch (Row[X])
                {
                case '#': Tile = EVaultTile::Wall; break;
                case 'o': Tile = EVaultTile::Socket; break;
                case 'p': Tile = EVaultTile::Relay; break;
                case 'g': Tile = EVaultTile::Gate; break;
                case '~': Tile = EVaultTile::Water; break;
                case 'm': Tile = EVaultTile::Mirror; break;
                case 'P': Level.Start = Position; break;
                case 'B': Level.Batteries.Add(Position); break;
                default: break;
                }
            }
            ++Y;
        }
        check(Level.Validate().IsEmpty());
        return Level;
    }
}

bool FVaultLevel::Contains(FIntPoint Position) const
{
    return Position.X >= 0 && Position.Y >= 0 && Position.X < Size.X && Position.Y < Size.Y;
}

int32 FVaultLevel::Index(FIntPoint Position) const
{
    return Position.Y * Size.X + Position.X;
}

EVaultTile FVaultLevel::TileAt(FIntPoint Position) const
{
    return Contains(Position) ? Tiles[Index(Position)] : EVaultTile::Wall;
}

FString FVaultLevel::Validate() const
{
    if (Size.X < MinSide || Size.Y < MinSide || Size.X > MaxSide || Size.Y > MaxSide ||
        Tiles.Num() != Size.X * Size.Y)
    {
        return TEXT("棋盘尺寸无效。");
    }
    if (!Contains(Start) || TileAt(Start) == EVaultTile::Wall || TileAt(Start) == EVaultTile::Water
        || Batteries.Contains(Start))
    {
        return TEXT("请将玛拉放在空着的干地上。");
    }
    const int32 Sockets = Tiles.FilterByPredicate([](EVaultTile Tile) { return Tile == EVaultTile::Socket; }).Num();
    if (Sockets == 0 || Batteries.Num() != Sockets)
    {
        return TEXT("航标和灯石必须一一对应。");
    }
    for (FIntPoint Position : Batteries)
    {
        if (!Contains(Position) || TileAt(Position) == EVaultTile::Wall || TileAt(Position) == EVaultTile::Water)
        {
            return TEXT("请将灯石放在干地上。");
        }
    }
    if (Tiles.Contains(EVaultTile::Relay) != Tiles.Contains(EVaultTile::Gate))
    {
        return TEXT("石印和闸门需同时放置。");
    }
    const int32 Mirrors = Tiles.FilterByPredicate([](EVaultTile Tile) { return Tile == EVaultTile::Mirror; }).Num();
    if (Mirrors != 0 && Mirrors != 2)
    {
        return TEXT("潮镜必须成对放置。");
    }
    return {};
}

FVaultLevel FVaultLevel::Blank(FIntPoint Size)
{
    FVaultLevel Level;
    Level.Size = Size;
    Level.Tiles.Init(EVaultTile::Floor, Size.X * Size.Y);
    for (int32 Y = 0; Y < Size.Y; ++Y)
    {
        for (int32 X = 0; X < Size.X; ++X)
        {
            if (X == 0 || Y == 0 || X == Size.X - 1 || Y == Size.Y - 1)
            {
                Level.Tiles[Level.Index({ X, Y })] = EVaultTile::Wall;
            }
        }
    }
    return Level;
}

const TArray<FVaultLevel>& FVaultLevel::Campaign()
{
static const TArray<FVaultLevel> Levels = {
        FromRows(TEXT("空码头"), TEXT("把灯石推到父亲留下的蓝色航标上。"),
            TEXT("渡口迟迟没有铃声。父亲的灯还在码头上。"), {
                TEXT("########"),
                TEXT("#......#"),
                TEXT("#..B.o.#"),
                TEXT("#..P...#"),
                TEXT("#......#"),
                TEXT("########")
            }),
        FromRows(TEXT("跃水灯"), TEXT("把灯石推入水道，它会沿直线跳到第一格岸上。"),
            TEXT("泥里的字条写着：脚过不去的地方，灯能过去。"), {
                TEXT("##########"),
                TEXT("#........#"),
                TEXT("#..B~~o..#"),
                TEXT("#..P~~...#"),
                TEXT("#...~~...#"),
                TEXT("##########")
            }),
        FromRows(TEXT("一对潮镜"), TEXT("把灯石推上潮镜。站在镜上或旁边，按 Q 交换两面镜上的人和物。"),
            TEXT("父亲把两面银镜隔着缺口摆好，镜面遥遥相望。"), {
                TEXT("##########"),
                TEXT("#...#....#"),
                TEXT("#..m#..o.#"),
                TEXT("#..B#..m.#"),
                TEXT("#.P.#....#"),
                TEXT("##########")
            }),
        FromRows(TEXT("借一盏灯"), TEXT("用灯石压住金色石印，闸门便会降下。"),
            TEXT("他总留一盏灯，好让最后一批乘客过闸。"), {
                TEXT("##########"),
                TEXT("#....#...#"),
                TEXT("#.p..g.o.#"),
                TEXT("#..B.#...#"),
                TEXT("#.P.B#...#"),
                TEXT("#...o#...#"),
                TEXT("##########")
            }),
        FromRows(TEXT("静水两岸"), TEXT("先让灯石跃过水道，再从陆路绕到对岸。"),
            TEXT("潮水卷走了桥，可两岸都还留着父亲的记号。"), {
                TEXT("###########"),
                TEXT("#...#.....#"),
                TEXT("#.B.~~....#"),
                TEXT("#..P~~o...#"),
                TEXT("#...~~....#"),
                TEXT("#......#..#"),
                TEXT("###########")
            }),
        FromRows(TEXT("镜水彼岸"), TEXT("让灯石越过水道，再用潮镜把它送往更远的航标。"),
            TEXT("父亲在手记里画的路线，终点在镜水另一边。"), {
                TEXT("#############"),
                TEXT("#...#...#...#"),
                TEXT("#.B.~~m.#.o.#"),
                TEXT("#..P~~..#.m.#"),
                TEXT("#...~~..#...#"),
                TEXT("#.......#...#"),
                TEXT("#############")
            }),
        FromRows(TEXT("归航之灯"), TEXT("点亮两岸的航标。留一盏灯开闸，等走完回程再取回。"),
            TEXT("一盏灯留给归人，一盏灯唤渡船回家。"), {
                TEXT("###############"),
                TEXT("#...##...#....#"),
                TEXT("#.B.~~m..#..o.#"),
                TEXT("#..P.#...#..m.#"),
                TEXT("#..Bp#...#....#"),
                TEXT("#..#og...#....#"),
                TEXT("#....#...#....#"),
                TEXT("###############")
            })
    };
    return Levels;
}

bool FVaultLevel::Save(int32 Slot) const
{
    if (!Validate().IsEmpty())
    {
        return false;
    }

    const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
    Object->SetNumberField(TEXT("version"), 2);
    Object->SetNumberField(TEXT("width"), Size.X);
    Object->SetNumberField(TEXT("height"), Size.Y);
    Object->SetNumberField(TEXT("robot"), Index(Start));

    TArray<TSharedPtr<FJsonValue>> Terrain;
    TArray<TSharedPtr<FJsonValue>> Boxes;
    for (int32 Cell = 0; Cell < Tiles.Num(); ++Cell)
    {
        Terrain.Add(MakeShared<FJsonValueNumber>(static_cast<int32>(Tiles[Cell])));
        if (Batteries.Contains({ Cell % Size.X, Cell / Size.X }))
        {
            Boxes.Add(MakeShared<FJsonValueNumber>(Cell));
        }
    }
    Object->SetArrayField(TEXT("tiles"), Terrain);
    Object->SetArrayField(TEXT("batteries"), Boxes);

    FString Data;
    const FString Path = SlotPath(Slot);
    const FString TempPath = Path + TEXT(".tmp");
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Data);
    return FJsonSerializer::Serialize(Object, Writer)
        && IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true)
        && FFileHelper::SaveStringToFile(Data, *TempPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)
        && IFileManager::Get().Move(*Path, *TempPath, true);
}

bool FVaultLevel::Load(int32 Slot, FVaultLevel& OutLevel)
{
    const FString Path = SlotPath(Slot);
    const int64 Bytes = IFileManager::Get().FileSize(*Path);
    FString Data;
    TSharedPtr<FJsonObject> Object;
    if (Bytes <= 0 || Bytes > 65536 || !FFileHelper::LoadFileToString(Data, *Path)
        || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Data), Object) || !Object.IsValid())
    {
        return false;
    }

    int32 Version, Width, Height, Robot;
    const TArray<TSharedPtr<FJsonValue>>* Terrain = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Boxes = nullptr;
    if (!Object->TryGetNumberField(TEXT("version"), Version) || (Version != 1 && Version != 2)
        || !Object->TryGetNumberField(TEXT("width"), Width) || Width < MinSide || Width > MaxSide
        || !Object->TryGetNumberField(TEXT("height"), Height) || Height < MinSide || Height > MaxSide
        || !Object->TryGetNumberField(TEXT("robot"), Robot) || Robot < 0 || Robot >= Width * Height
        || !Object->TryGetArrayField(TEXT("tiles"), Terrain) || Terrain->Num() != Width * Height
        || !Object->TryGetArrayField(TEXT("batteries"), Boxes) || Boxes->Num() > Width * Height)
    {
        return false;
    }

    FVaultLevel Level;
    Level.Size = { Width, Height };
    Level.Start = { Robot % Width, Robot / Width };
    const int32 MaxTile = static_cast<int32>(Version == 1 ? EVaultTile::Gate : EVaultTile::Mirror);
    for (const TSharedPtr<FJsonValue>& Value : *Terrain)
    {
        double Tile;
        if (!Value->TryGetNumber(Tile) || Tile < 0 || Tile > MaxTile
            || Tile != FMath::FloorToDouble(Tile))
        {
            return false;
        }
        Level.Tiles.Add(static_cast<EVaultTile>(static_cast<int32>(Tile)));
    }
    for (const TSharedPtr<FJsonValue>& Value : *Boxes)
    {
        double Cell;
        if (!Value->TryGetNumber(Cell) || Cell < 0 || Cell >= Width * Height
            || Cell != FMath::FloorToDouble(Cell))
        {
            return false;
        }
        const int32 Index = static_cast<int32>(Cell);
        Level.Batteries.Add({ Index % Width, Index / Width });
    }
    if (Level.Batteries.Num() != Boxes->Num() || !Level.Validate().IsEmpty())
    {
        return false;
    }
    Level.Title = FString::Printf(TEXT("自制关卡 %d"), Slot + 1);
    Level.Hint = TEXT("把所有灯石推入航标。");
    OutLevel = MoveTemp(Level);
    return true;
}

void FVaultBoard::Start(const FVaultLevel& InLevel)
{
    Level = InLevel;
    Reset();
}

void FVaultBoard::Reset()
{
    State = { Level.Start, Level.Batteries, 0, 0 };
    History.Reset();
}

bool FVaultBoard::Move(FIntPoint Direction, FIntPoint* Landing)
{
    if (Landing)
    {
        *Landing = { INDEX_NONE, INDEX_NONE };
    }
    if (FMath::Abs(Direction.X) + FMath::Abs(Direction.Y) != 1 || IsComplete())
    {
        return false;
    }
    const bool bPowered = IsPowered();
    const FIntPoint Next = State.Player + Direction;
    if (!CanEnter(Next, bPowered))
    {
        return false;
    }
    const bool bPush = State.Batteries.Contains(Next);
    FIntPoint Destination = Next + Direction;
    if (bPush)
    {
        while (Level.TileAt(Destination) == EVaultTile::Water)
        {
            Destination += Direction;
        }
    }
    if (bPush && (!CanEnter(Destination, bPowered) || State.Batteries.Contains(Destination)))
    {
        return false;
    }

    History.Add(State);
    if (bPush)
    {
        State.Batteries.Remove(Next);
        State.Batteries.Add(Destination);
        ++State.Pushes;
        if (Landing)
        {
            *Landing = Destination;
        }
    }
    State.Player = Next;
    ++State.Moves;
    return true;
}

bool FVaultBoard::ExchangeTiles(FIntPoint& First, FIntPoint& Second) const
{
    if (IsComplete())
    {
        return false;
    }

    First = Second = { INDEX_NONE, INDEX_NONE };
    for (int32 Index = 0; Index < Level.Tiles.Num(); ++Index)
    {
        if (Level.Tiles[Index] == EVaultTile::Mirror)
        {
            const FIntPoint Cell(Index % Level.Size.X, Index / Level.Size.X);
            if (First.X == INDEX_NONE) { First = Cell; }
            else { Second = Cell; break; }
        }
    }
    if (Second.X == INDEX_NONE)
    {
        return false;
    }

    const auto Nearby = [this](FIntPoint At)
    {
        const FIntPoint Distance = At - State.Player;
        return FMath::Abs(Distance.X) + FMath::Abs(Distance.Y) <= 1;
    };
    return (Nearby(First) || Nearby(Second)) && (State.Player == First || State.Player == Second
        || State.Batteries.Contains(First) != State.Batteries.Contains(Second));
}

bool FVaultBoard::CanExchange() const
{
    FIntPoint First, Second;
    return ExchangeTiles(First, Second);
}

bool FVaultBoard::Exchange()
{
    FIntPoint First, Second;
    if (!ExchangeTiles(First, Second))
    {
        return false;
    }

    History.Add(State);
    if (State.Player == First) { State.Player = Second; }
    else if (State.Player == Second) { State.Player = First; }
    const bool bFirst = State.Batteries.Remove(First) > 0;
    const bool bSecond = State.Batteries.Remove(Second) > 0;
    if (bFirst) { State.Batteries.Add(Second); }
    if (bSecond) { State.Batteries.Add(First); }
    ++State.Moves;
    return true;
}

bool FVaultBoard::Undo()
{
    if (History.IsEmpty())
    {
        return false;
    }
    State = History.Pop();
    return true;
}

bool FVaultBoard::IsPowered() const
{
    for (FIntPoint Position : State.Batteries)
    {
        if (Level.TileAt(Position) == EVaultTile::Relay)
        {
            return true;
        }
    }
    return false;
}

bool FVaultBoard::IsGateOpen(FIntPoint Position, bool bPowered) const
{
    return bPowered || Position == State.Player || State.Batteries.Contains(Position);
}

bool FVaultBoard::CanEnter(FIntPoint Position, bool bPowered) const
{
    const EVaultTile Tile = Level.TileAt(Position);
    return Tile != EVaultTile::Wall && Tile != EVaultTile::Water && (Tile != EVaultTile::Gate || IsGateOpen(Position, bPowered));
}

int32 FVaultBoard::ChargedCount() const
{
    int32 Count = 0;
    for (FIntPoint Position : State.Batteries)
    {
        Count += Level.TileAt(Position) == EVaultTile::Socket;
    }
    return Count;
}

bool FVaultBoard::IsComplete() const
{
    return !State.Batteries.IsEmpty() && ChargedCount() == State.Batteries.Num();
}
