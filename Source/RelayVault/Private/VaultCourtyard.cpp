#include "VaultCourtyard.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "VaultGeometry.h"

using namespace VaultArt;

namespace
{
    constexpr float Cell = 100.f;

    FVector Grid(FIntPoint Position)
    {
        return FVector(Position.X * Cell, Position.Y * Cell, 0);
    }

    FQuat Yaw(float Degrees)
    {
        return FRotator(0, Degrees, 0).Quaternion();
    }

    void EtchedPlate(FMesh& Stone, FMesh& Glow, FVector At, FLinearColor Tint, bool bSocket)
    {
        const FLinearColor Metal = Color(0x495f59);
        Stone.Box(At + FVector(0,0,1), {68,68,8}, 4, Metal, Yaw(45));
        Stone.Box(At + FVector(0,0,5), {58,58,4}, 2, Color(0x344747), Yaw(45));
        Glow.Ring(At + FVector(0,0,7.3), 35, 39, bSocket ? 12 : 4, Tint, .10f);
        if (bSocket)
        {
            Glow.Ring(At + FVector(0,0,7.4), 14, 17, 4, Tint);
        }
        else
        {
            for (int32 Bar = -1; Bar <= 1; ++Bar)
            {
                Glow.Box(At + FVector(Bar * 9,0,7.6), {3,25,1}, .25, Tint, Yaw(18));
            }
        }
    }

    void MasonryWall(FMesh& Stone, FVector At, FRandomStream& Random, bool bFront)
    {
        const float Height = bFront ? 19 : 39;
        const FLinearColor Tints[] = { Color(0x8e8a71), Color(0x99927a), Color(0x797c68) };
        Stone.Box(At + FVector(0,0,3), {97,97,11}, 4, Color(0x666e60));
        for (int32 Side : { -1, 1 })
        {
            Stone.Box(At + FVector(Side * 22,0,Height * .45), {43,83,Height}, 5,
                Tints[Random.RandRange(0, 2)]);
        }
        Stone.Box(At + FVector(0,0,Height + 1), {93,90,11}, 5, Color(0x928267));
        if (Random.FRand() < .46)
        {
            Stone.Box(At + FVector(-30,23,Height + 7), {22,12,2}, .5,
                Color(0x686b42), Yaw(Random.FRandRange(-35, 35)));
        }
    }
}

AVaultCourtyard::AVaultCourtyard()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Origin")));

    UDirectionalLightComponent* Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Afternoon sun"));
    Sun->SetupAttachment(GetRootComponent());
    Sun->SetMobility(EComponentMobility::Movable);
    Sun->SetRelativeRotation(FRotator(-47, -132, 0));
    Sun->SetIntensity(2.85);
    Sun->SetLightColor(FLinearColor(1.0f, 0.83f, 0.63f));
    Sun->DynamicShadowDistanceMovableLight = 16000.f;
    Sun->DynamicShadowCascades = 3;
    Sun->ShadowBias = .43f;

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Stone(TEXT("/Game/Art/Materials/M_Courtyard"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Rune(TEXT("/Game/Art/Materials/M_Rune"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Water(TEXT("/Game/Art/Materials/M_Basin"));
    StoneMaterial = Stone.Object;
    RuneMaterial = Rune.Object;
    WaterMaterial = Water.Object;
}

UProceduralMeshComponent* AVaultCourtyard::NewMesh(bool bShadows)
{
    UProceduralMeshComponent* Component = NewObject<UProceduralMeshComponent>(this);
    Component->SetupAttachment(GetRootComponent());
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetCastShadow(bShadows);
    Component->RegisterComponent();
    return Component;
}

void AVaultCourtyard::Build(const FVaultLevel& Level, int32 Sector)
{
    if (!Camera)
    {
        Camera = GetWorld()->SpawnActor<ACameraActor>();
        UCameraComponent* Lens = Camera->GetCameraComponent();
        Lens->SetProjectionMode(ECameraProjectionMode::Orthographic);
        Lens->SetAutoCalculateOrthoPlanes(false);
        Lens->SetOrthoNearClipPlane(5.f);
        Lens->SetOrthoFarClipPlane(22000.f);
        Lens->bConstrainAspectRatio = false;
        Lens->bOverrideAspectRatioAxisConstraint = true;
        Lens->SetAspectRatioAxisConstraint(AspectRatio_MaintainXFOV);

        Terrain = NewMesh();
        Surrounds = NewMesh();
        Basin = NewMesh(false);
        Walker = NewMesh();
        LiveRelays = NewMesh(false);
        BuildWalker();
        Walker->SetRelativeScale3D(FVector(1.15));
    }

    for (UProceduralMeshComponent* Gate : GateBars)
    {
        Gate->DestroyComponent();
    }
    GateBars.Reset();
    GateCells.Reset();
    while (Batteries.Num() > Level.Batteries.Num())
    {
        Batteries.Pop()->DestroyComponent();
    }
    while (Batteries.Num() < Level.Batteries.Num())
    {
        UProceduralMeshComponent* Battery = NewMesh();
        BuildBattery(*Battery);
        Batteries.Add(Battery);
    }

    BuildBoard(Level, Sector);
    BuildSurrounds(Level.Size, Sector);
    SetActorHiddenInGame(false);
}

void AVaultCourtyard::EndPlay(const EEndPlayReason::Type Reason)
{
    if (IsValid(Camera))
    {
        Camera->Destroy();
    }
    Super::EndPlay(Reason);
}

void AVaultCourtyard::Hide()
{
    SetActorHiddenInGame(true);
}

void AVaultCourtyard::BuildBoard(const FVaultLevel& Level, int32 Sector)
{
    FMesh Stone, Glow, Power, Channel, Flash;
    FRandomStream Random(503 + Sector * 17);
    const FLinearColor Slabs[] = {
        Color(0xc8b894), Color(0xbba985), Color(0xb6a98d), Color(0xc6bda3),
        Color(0xc2b18d), Color(0xcbbe9d), Color(0xb7ac91)
    };
    const FLinearColor Cyan = Color(0x56dbc7);
    const FLinearColor Gold = Color(0xe5ac48);

    for (int32 Y = 0; Y < Level.Size.Y; ++Y)
    {
        for (int32 X = 0; X < Level.Size.X; ++X)
        {
            const FIntPoint Position(X, Y);
            const FVector At = Grid(Position);
            const EVaultTile Tile = Level.TileAt(Position);
            const FLinearColor Tint = Slabs[Random.RandRange(0, 6)];
            if (Tile == EVaultTile::Water)
            {
                Channel.Quad(At + FVector(-50,-50,-3), At + FVector(50,-50,-3),
                    At + FVector(50,50,-3), At + FVector(-50,50,-3), FLinearColor::White);
                continue;
            }
            Stone.Box(At - FVector(0,0,9), {99.6,99.6,19}, 5, Tint);
            if (Tile == EVaultTile::Wall)
            {
                MasonryWall(Stone, At, Random, Y == Level.Size.Y - 1);
            }
            else if (Tile == EVaultTile::Socket)
            {
                EtchedPlate(Stone, Glow, At, Cyan, true);
            }
            else if (Tile == EVaultTile::Relay)
            {
                EtchedPlate(Stone, Glow, At, Gold, false);
                Power.Ring(At + FVector(0,0,8.5), 36, 41, 4, Cyan, .10f);
            }
            else if (Tile == EVaultTile::Mirror)
            {
                Stone.Beam(At, At + FVector(0,0,5), 42, 39, Color(0x838ba4), 12);
                Stone.Beam(At + FVector(0,0,5), At + FVector(0,0,6.5), 33, 32, Color(0x403d66), 12);
                Glow.Ring(At + FVector(0,0,6.2), 35, 38, 12, Color(0xb69aff), .12f);
                Glow.Ring(At + FVector(0,0,6.8), 16, 18, 6, Color(0xdec6ff));
                Flash.Ring(At + FVector(0,0,9), 39, 46, 12, Color(0xf2d1ff), .10f);
            }
            else if (Tile == EVaultTile::Gate)
            {
                const bool bAcrossX = Level.TileAt(Position + FIntPoint(-1,0)) == EVaultTile::Wall
                    && Level.TileAt(Position + FIntPoint(1,0)) == EVaultTile::Wall;
                const auto Across = [&](float Along, float Perp, float Z)
                {
                    return At + (bAcrossX ? FVector(Along, Perp, Z) : FVector(Perp, Along, Z));
                };

                Stone.Box(At + FVector(0,0,1), bAcrossX ? FVector(94,28,7) : FVector(28,94,7),
                    2, Color(0x566962));
                for (int32 Side : {-1, 1})
                {
                    Stone.Box(Across(Side * 42,0,14), {13,15,26}, 3, Color(0x6c7569));
                    Stone.Box(Across(Side * 42,0,28), {16,18,7}, 2, Color(0xb99153));
                    Glow.Gem(Across(Side * 42,0,34), {4,4,5}, Cyan, 4);
                }
                GateCells.Add(Position);
                UProceduralMeshComponent* Bars = NewMesh();
                BuildGate(*Bars, bAcrossX);
                Bars->SetRelativeLocation(At);
                GateBars.Add(Bars);
            }
            else if (Random.FRand() < .35f)
            {
                const FVector Trace = At + FVector(Random.FRandRange(-35, 29), Random.FRandRange(-35, 21), .7);
                const FVector Mid = Trace + FVector(9, 9, 0);
                const FVector End = Mid + FVector(11, 1, 0);
                const FVector Edge(1.5, -.3, 0);
                Stone.Quad(Trace, Trace + Edge, Mid + Edge, Mid, Tint * .79f);
                Stone.Quad(Mid, Mid + Edge, End + Edge, End, Tint * .86f);
            }
        }
    }
    Stone.Apply(*Terrain, 0, StoneMaterial);
    Glow.Apply(*Terrain, 1, RuneMaterial);
    Channel.Apply(*Terrain, 2, WaterMaterial);
    Flash.Apply(*Terrain, 3, RuneMaterial);
    Terrain->SetMeshSectionVisible(3, false);
    Power.Apply(*LiveRelays, 0, RuneMaterial);
}

void AVaultCourtyard::BuildGate(UProceduralMeshComponent& Component, bool bAcrossX)
{
    FMesh Stone, Glow;
    const FQuat Rotation = Yaw(bAcrossX ? 0 : 90);
    const FLinearColor Bronze = Color(0x8a6c40);
    const FLinearColor Verdigris = Color(0x517971);
    for (int32 Bar = -3; Bar <= 3; ++Bar)
    {
        const float X = Bar * 11.4f;
        Stone.Box(Rotation.RotateVector({X,0,15}), {4,7,30}, 1.5, Bronze, Rotation);
        Stone.Gem(Rotation.RotateVector({X,0,32}), {4,4,7}, Verdigris, 4);
    }
    Stone.Box({0,0,11}, {87,10,5}, 1.4, Verdigris, Rotation);
    Stone.Box({0,0,26}, {87,10,6}, 1.4, Verdigris, Rotation);
    Glow.Gem({0,0,34}, {6,6,8}, Color(0xf3a247), 4);
    Stone.Apply(Component, 0, StoneMaterial);
    Glow.Apply(Component, 1, RuneMaterial);
}

void AVaultCourtyard::BuildBattery(UProceduralMeshComponent& Component)
{
    FMesh Wood, Amber, Cyan;
    const FLinearColor Bronze = Color(0xa17c4c);
    Wood.Box({0,0,23}, {58,58,46}, 7, Color(0x70412d));
    Wood.Box({0,0,46}, {59,59,10}, 4, Color(0xc18b51));
    Wood.Box({0,0,5}, {59,59,8}, 3, Bronze);
    for (int32 X : {-1, 1})
    {
        for (int32 Y : {-1, 1})
        {
            Wood.Box({X * 24.,Y * 24.,25}, {8,8,40}, 2, Bronze);
            Wood.Gem({X * 24.,Y * 24.,52}, {3,3,2.5}, Color(0xe0b875), 4);
        }
        Wood.Box({0,X * 29.3,24}, {31,3.2,23}, 2, Color(0x9a613e));
        Wood.Box({X * 29.3,0,24}, {3.2,31,23}, 2, Color(0x9a613e));
    }
    Wood.Box({0,0,52}, {31,31,5}, 2, Color(0x5a5b51), Yaw(45));
    Amber.Gem({0,0,57}, {14,14,9}, Color(0xf6ae4c), 4);
    Cyan.Gem({0,0,57}, {14,14,9}, Color(0x6af2c7), 4);
    Wood.Apply(Component, 0, StoneMaterial);
    Amber.Apply(Component, 1, RuneMaterial);
    Cyan.Apply(Component, 2, RuneMaterial);
    Component.SetMeshSectionVisible(2, false);
}

void AVaultCourtyard::BuildWalker()
{
    FMesh Figure, Glow;
    const FLinearColor Tunic = Color(0x33484c);
    const FLinearColor Cloth = Color(0xad412a);
    const FLinearColor Leather = Color(0x48392f);
    const FLinearColor Skin = Color(0xe0ad75);
    for (int32 Side : {-1, 1})
    {
        Figure.Box({4,Side * 7.,5}, {15,10,10}, 3, Leather);
        Figure.Beam({0,Side * 7.,7}, {0,Side * 7.,25}, 5, 7, Tunic);
        Figure.Beam({0,Side * 12.,39}, {6,Side * 17.,24}, 7, 4, Cloth);
        Figure.Gem({7,Side * 17.,22}, {4.6,4.3,5}, Skin);
    }
    Figure.Beam({-2,0,13}, {0,0,42}, 17, 11, Cloth, 9);
    Figure.Beam({0,0,30}, {0,0,36}, 13, 12, Leather, 9);
    Figure.Beam({0,0,43}, {0,0,48}, 13, 10, Color(0xe4a453), 9);
    Figure.Gem({0,0,53}, {12,11,13}, Color(0x9f3027), 8);
    Figure.Box({8,0,52}, {7,13,11}, 3, Skin);
    Figure.Box({-9,0,34}, {12,19,21}, 4, Color(0x7b573a));
    Figure.Triangle({-5,-4,42}, {-19,3,36}, {-28,-4,42}, Cloth);
    Figure.Triangle({-28,-4,42}, {-19,3,36}, {-5,-4,42}, Cloth);
    Figure.Box({5,-14,32}, {7,7,12}, 2, Leather);
    Glow.Gem({6,-15,30}, {4,4,6}, Color(0x8be7ba), 4);
    Figure.Apply(*Walker, 0, StoneMaterial);
    Glow.Apply(*Walker, 1, RuneMaterial);
}

void AVaultCourtyard::Present(const FVaultLevel& Level, const FVaultState& State, FVector2D Motion, FVector2D BatteryMotion,
    FIntPoint MovingBattery, FIntPoint Facing, float Step, bool bMirrorFlash, FVector2D Viewport,
    FVector2D BoardCenter, float UiScale, bool bTitle)
{
    if (!Camera)
    {
        return;
    }

    Frame(Level.Size, Viewport, BoardCenter, UiScale, bTitle);
    const FVector Shift(Motion.X * Cell, Motion.Y * Cell, 0);
    const float Hop = FMath::Sin(Step * UE_PI) * 3.5f;
    Walker->SetRelativeLocation(Grid(State.Player) - Shift + FVector(0,0,Hop));
    Walker->SetRelativeRotation(FRotator(0, FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Facing.Y), static_cast<float>(Facing.X))), 0));
    if (Terrain->IsMeshSectionVisible(3) != bMirrorFlash)
    {
        Terrain->SetMeshSectionVisible(3, bMirrorFlash);
    }
    const FVector CrateShift(BatteryMotion.X * Cell, BatteryMotion.Y * Cell, 0);
    const float CrateHop = BatteryMotion.SizeSquared() > Motion.SizeSquared() + .01f
        ? FMath::Sin(Step * UE_PI) * 20.f : 0.f;
    int32 Index = 0;
    bool bPowered = false;
    for (FIntPoint Battery : State.Batteries)
    {
        const EVaultTile Tile = Level.TileAt(Battery);
        const bool bCharged = Tile == EVaultTile::Socket;
        UProceduralMeshComponent& Mesh = *Batteries[Index++];
        Mesh.SetRelativeLocation(Grid(Battery) + (Battery == MovingBattery ? FVector(0,0,CrateHop) - CrateShift : FVector::ZeroVector));
        if (Mesh.IsMeshSectionVisible(2) != bCharged)
        {
            Mesh.SetMeshSectionVisible(1, !bCharged);
            Mesh.SetMeshSectionVisible(2, bCharged);
        }
        bPowered |= Tile == EVaultTile::Relay;
    }
    LiveRelays->SetVisibility(bPowered);
    for (int32 Gate = 0; Gate < GateCells.Num(); ++Gate)
    {
        const FIntPoint Position = GateCells[Gate];
        const bool bOpen = bPowered || Position == State.Player || State.Batteries.Contains(Position);
        FVector GateLocation = Grid(Position);
        const float Destination = bOpen ? -39.f : 0.f;
        GateLocation.Z = FMath::FInterpTo(GateBars[Gate]->GetRelativeLocation().Z, Destination,
            GetWorld()->GetDeltaSeconds(), 16.f);
        GateBars[Gate]->SetRelativeLocation(GateLocation);
    }

    if (APlayerController* Controller = GetWorld()->GetFirstPlayerController();
        Controller && Controller->GetViewTarget() != Camera)
    {
        Controller->SetViewTarget(Camera);
    }
}

void AVaultCourtyard::Frame(FIntPoint Size, FVector2D Viewport, FVector2D BoardCenter, float UiScale, bool bTitle)
{
    constexpr float Tilt = 50.f;
    const FRotator Angle(-Tilt, -90, 0);
    const FRotationMatrix Orientation(Angle);
    const FVector Right = Orientation.GetScaledAxis(EAxis::Y);
    const FVector Up = Orientation.GetScaledAxis(EAxis::Z);
    const FVector Forward = Orientation.GetScaledAxis(EAxis::X);
    const FVector Center = Grid(Size - FIntPoint(1, 1)) * .5;
    const float FitWidth = bTitle ? 658 : 685;
    const float FitHeight = bTitle ? 398 : 492;
    const float PixelsPerUnit = UiScale * FMath::Min(FitWidth / (Size.X * Cell),
        FitHeight / (Size.Y * Cell * FMath::Sin(FMath::DegreesToRadians(Tilt)) + 45));
    const float UnitsPerPixel = 1 / PixelsPerUnit;
    const FVector2D Offset = BoardCenter - Viewport * .5;
    const FVector Target = Center - Right * Offset.X * UnitsPerPixel + Up * Offset.Y * UnitsPerPixel;
    Camera->GetCameraComponent()->SetOrthoWidth(Viewport.X * UnitsPerPixel);
    Camera->SetActorLocationAndRotation(Target - Forward * 5800.f, Angle);
}
