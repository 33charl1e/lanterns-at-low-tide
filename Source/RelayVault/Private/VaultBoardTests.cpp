#include "VaultBoard.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVaultCampaignTest, "RelayVault.Board.CampaignAndUndo",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FVaultCampaignTest::RunTest(const FString& Parameters)
{
    const TArray<FVaultLevel>& Levels = FVaultLevel::Campaign();
const TCHAR* Solutions[] = {
        TEXT("LURR"), TEXT("LUR"), TEXT("RUQUQDQU"), TEXT("RDRUULDLURURRRLLLULDDDLDRR"),
        TEXT("UULDLDRR"), TEXT("LLURRDDDRRRUUQUQDQU"), TEXT("LLURRLDDRLDDRRURRUUQUQDQUQDDDLLDLLUURURD")
    };
    if (!TestEqual(TEXT("Solution count"), Levels.Num(), static_cast<int32>(UE_ARRAY_COUNT(Solutions))))
    {
        return false;
    }

    FVaultBoard Board;
    const TMap<TCHAR, FIntPoint> Directions = {
        { 'L', { -1, 0 } }, { 'R', { 1, 0 } }, { 'U', { 0, -1 } }, { 'D', { 0, 1 } }
    };
    for (int32 Stage = 0; Stage < Levels.Num(); ++Stage)
    {
        const FVaultLevel& Level = Levels[Stage];
        Board.Start(Level);
        bool bUsedRelay = false;
        bool bSkipped = false;
        bool bExchanged = false;
        for (const TCHAR* Step = Solutions[Stage]; *Step; ++Step)
        {
            const bool bExchange = *Step == 'Q';
            FIntPoint Landing(INDEX_NONE, INDEX_NONE);
            const auto Apply = [&]()
            {
                return bExchange ? Board.Exchange() : Board.Move(Directions.FindChecked(*Step), &Landing);
            };
            const FVaultState Before = Board.State;
            const bool bWasPowered = Board.IsPowered();
            if (!TestTrue(Level.Title + TEXT(" legal step"), Apply()))
            {
                return false;
            }
            const bool bNowPowered = Board.IsPowered();
            const bool bSkip = Landing.X != INDEX_NONE && (Landing - Board.State.Player).SizeSquared() > 1;
            bUsedRelay |= bNowPowered;
            bSkipped |= bSkip;
            bExchanged |= bExchange;
            if (bWasPowered != bNowPowered || bSkip || bExchange)
            {
                const FVaultState After = Board.State;
                TestTrue(TEXT("Can undo mechanism in one move"), Board.Undo());
                TestEqual(TEXT("Undo restores circuit power"), Board.IsPowered(), bWasPowered);
                TestEqual(TEXT("Undo restores traveler"), Board.State.Player, Before.Player);
                TestEqual(TEXT("Undo restores move count"), Board.State.Moves, Before.Moves);
                TestEqual(TEXT("Undo restores number of lanterns"), Board.State.Batteries.Num(), Before.Batteries.Num());
                for (FIntPoint Position : Before.Batteries)
                {
                    TestTrue(TEXT("Undo restores lantern position"), Board.State.Batteries.Contains(Position));
                }
                TestTrue(TEXT("Can replay mechanism"), Apply());
                TestEqual(TEXT("Replay restores traveler"), Board.State.Player, After.Player);
                TestEqual(TEXT("Replay restores new circuit power"), Board.IsPowered(), bNowPowered);
            }
        }
        TestTrue(Level.Title + TEXT(" is solvable"), Board.IsComplete());
        TestEqual(Level.Title + TEXT(" move count"), Board.State.Moves, FCString::Strlen(Solutions[Stage]));
        TestEqual(Level.Title + TEXT(" uses its relay"), bUsedRelay, Level.Tiles.Contains(EVaultTile::Relay));

        TestEqual(Level.Title + TEXT(" uses its channel"), bSkipped, Level.Tiles.Contains(EVaultTile::Water));
        TestEqual(Level.Title + TEXT(" uses its mirrors"), bExchanged, Level.Tiles.Contains(EVaultTile::Mirror));

        TestTrue(TEXT("Can undo winning move"), Board.Undo());
        TestFalse(TEXT("Undo reopens level"), Board.IsComplete());
        Board.Reset();
        TestEqual(TEXT("Reset returns the robot to its starting tile"), Board.State.Player, Level.Start);
        TestFalse(TEXT("Reset clears undo history"), Board.Undo());
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVaultSlotTest, "RelayVault.Workshop.SlotRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FVaultSlotTest::RunTest(const FString& Parameters)
{
    const int32 Slot = 9879;
    const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("VaultLevels/slot-9880.json"));
    if (IFileManager::Get().FileExists(*Path) || IFileManager::Get().FileExists(*(Path + TEXT(".tmp"))))
    {
        AddError(TEXT("The isolated test slot already exists."));
        return false;
    }
    FVaultLevel Draft = FVaultLevel::Campaign().Last();
    FVaultLevel Loaded;
    TestTrue(TEXT("Can save a valid layout"), Draft.Save(Slot));
    TestTrue(TEXT("Can load a saved layout"), FVaultLevel::Load(Slot, Loaded));
    TestEqual(TEXT("Grid size survives saving"), Loaded.Size, Draft.Size);
    TestTrue(TEXT("Terrain survives saving"), Loaded.Tiles == Draft.Tiles);
    TestEqual(TEXT("Robot start survives saving"), Loaded.Start, Draft.Start);
    TestEqual(TEXT("Battery count survives saving"), Loaded.Batteries.Num(), Draft.Batteries.Num());
    for (FIntPoint Battery : Draft.Batteries)
    {
        TestTrue(TEXT("Battery position survives saving"), Loaded.Batteries.Contains(Battery));
    }
    FVaultBoard Board;
    Board.Start(Loaded);
    TestTrue(TEXT("Can move in a playtest"), Board.Move({ 1, 0 }));
    TestEqual(TEXT("Playtest leaves editor layout intact"), Loaded.Start, Draft.Start);
    Draft.Batteries.Reset();
    TestFalse(TEXT("Invalid edit cannot overwrite slot"), Draft.Save(Slot));
    TestTrue(TEXT("Last valid save still loads"), FVaultLevel::Load(Slot, Loaded));

    IFileManager::Get().Delete(*Path, false);
    IFileManager::Get().Delete(*(Path + TEXT(".tmp")), false);
    return true;
}
#endif
