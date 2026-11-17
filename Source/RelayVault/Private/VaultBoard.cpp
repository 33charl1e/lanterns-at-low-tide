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

    FVaultLevel FromRows(const TCHAR* Title, const TCHAR* Hint, std::initializer_list<const TCHAR*> Rows)
    {
        const FIntPoint Size(FCString::Strlen(*Rows.begin()), static_cast<int32>(Rows.size()));
        FVaultLevel Level = FVaultLevel::Blank(Size);
        Level.Title = Title;
        Level.Hint = Hint;
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
        return TEXT("Grid size is invalid.");
    }
    if (!Contains(Start) || TileAt(Start) == EVaultTile::Wall || Batteries.Contains(Start))
    {
        return TEXT("Place the traveler on a free tile.");
    }
    const int32 Sockets = Tiles.FilterByPredicate([](EVaultTile Tile) { return Tile == EVaultTile::Socket; }).Num();
    if (Sockets == 0 || Batteries.Num() != Sockets)
    {
        return TEXT("Use one socket per core.");
    }
    for (FIntPoint Position : Batteries)
    {
        if (!Contains(Position) || TileAt(Position) == EVaultTile::Wall)
        {
            return TEXT("Put each core on a walkable tile.");
        }
    }
    if (Tiles.Contains(EVaultTile::Relay) != Tiles.Contains(EVaultTile::Gate))
    {
        return TEXT("Relays and gates must be used together.");
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
        FromRows(TEXT("Outer Terrace"), TEXT("Push the amber core onto the blue socket."), {
            TEXT("########"),
            TEXT("#......#"),
            TEXT("#..B.o.#"),
            TEXT("#..P...#"),
            TEXT("#......#"),
            TEXT("########")
        }),
        FromRows(TEXT("Twin Sockets"), TEXT("Go around the cores to push them into place."), {
            TEXT("#########"),
            TEXT("#.......#"),
            TEXT("#..#..o.#"),
            TEXT("#.PB....#"),
            TEXT("#..#B.o.#"),
            TEXT("#.......#"),
            TEXT("#########")
        }),
        FromRows(TEXT("The First Relay"), TEXT("A core on the gold relay opens the gate."), {
            TEXT("##########"),
            TEXT("#....#...#"),
            TEXT("#.p..g.o.#"),
            TEXT("#..B.#...#"),
            TEXT("#.P.B#...#"),
            TEXT("#...o#...#"),
            TEXT("##########")
        }),
        FromRows(TEXT("Borrowed Light"), TEXT("Send a core through, then reclaim the one powering the relay."), {
            TEXT("###########"),
            TEXT("#......####"),
            TEXT("#.p...g..o#"),
            TEXT("#..#...#..#"),
            TEXT("#.B..B.#..#"),
            TEXT("#...#..####"),
            TEXT("#..P.o.####"),
            TEXT("###########")
        }),
        FromRows(TEXT("The Last Crossing"), TEXT("Power the gates for two deliveries, then recover the last core."), {
            TEXT("############"),
            TEXT("#.......####"),
            TEXT("#.p..B.g..o#"),
            TEXT("#..#...#...#"),
            TEXT("#.B..B.g..o#"),
            TEXT("#...#..#####"),
            TEXT("#.P.o..#####"),
            TEXT("############")
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
    Object->SetNumberField(TEXT("version"), 1);
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
    if (!Object->TryGetNumberField(TEXT("version"), Version) || Version != 1
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
    for (const TSharedPtr<FJsonValue>& Value : *Terrain)
    {
        double Tile;
        if (!Value->TryGetNumber(Tile) || Tile < 0 || Tile > static_cast<int32>(EVaultTile::Gate)
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
    Level.Title = FString::Printf(TEXT("Workshop slot %d"), Slot + 1);
    Level.Hint = TEXT("Bring every core to a socket.");
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

bool FVaultBoard::Move(FIntPoint Direction)
{
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
    const FIntPoint Destination = Next + Direction;
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
    }
    State.Player = Next;
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
    return Tile != EVaultTile::Wall && (Tile != EVaultTile::Gate || IsGateOpen(Position, bPowered));
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
