#pragma once

#include "CoreMinimal.h"

enum class EVaultTile : uint8 { Floor, Wall, Socket, Relay, Gate, Water, Mirror };

struct FVaultLevel
{
    static constexpr int32 MinSide = 5;
    static constexpr int32 MaxSide = 16;

    FString Title;
    FString Hint;
    FString Journal;
    FIntPoint Size = { 10, 8 };
    TArray<EVaultTile> Tiles;
    FIntPoint Start = { INDEX_NONE, INDEX_NONE };
    TSet<FIntPoint> Batteries;

    bool Contains(FIntPoint Position) const;
    int32 Index(FIntPoint Position) const;
    EVaultTile TileAt(FIntPoint Position) const;
    FString Validate() const;
    static FVaultLevel Blank(FIntPoint Size);
    static const TArray<FVaultLevel>& Campaign();

    bool Save(int32 Slot) const;
    static bool Load(int32 Slot, FVaultLevel& OutLevel);
};

struct FVaultState
{
    FIntPoint Player;
    TSet<FIntPoint> Batteries;
    int32 Moves = 0;
    int32 Pushes = 0;
};

class FVaultBoard
{
public:
    FVaultLevel Level;
    FVaultState State;

    void Start(const FVaultLevel& InLevel);
    void Reset();
    bool Move(FIntPoint Direction, FIntPoint* Landing = nullptr);
    bool CanExchange() const;
    bool Exchange();
    bool Undo();
    bool IsPowered() const;
    bool IsGateOpen(FIntPoint Position, bool bPowered) const;
    int32 ChargedCount() const;
    bool IsComplete() const;

private:
    bool CanEnter(FIntPoint Position, bool bPowered) const;
    bool ExchangeTiles(FIntPoint& First, FIntPoint& Second) const;
    TArray<FVaultState> History;
};
