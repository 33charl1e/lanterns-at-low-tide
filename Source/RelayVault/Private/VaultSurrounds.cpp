#include "VaultCourtyard.h"

#include "Materials/MaterialInterface.h"
#include "VaultGeometry.h"

using namespace VaultArt;

namespace
{
    FQuat Turn(float Yaw)
    {
        return FRotator(0, Yaw, 0).Quaternion();
    }

    void Leaf(FMesh& Mesh, FVector At, float Length, float Width, FLinearColor Tint, FRandomStream& Random)
    {
        const float Angle = Random.FRandRange(0.f, 2.f * UE_PI);
        const FVector Axis(FMath::Cos(Angle), FMath::Sin(Angle), Random.FRandRange(-.52f, .52f));
        const FVector Cross = FVector::CrossProduct(FVector::UpVector, Axis);
        const FVector Tip = At + Axis * Length;
        const FVector Fold = At + Axis * (Length * .47f) + FVector(0,0,Width * .35f);
        const FVector Left = Fold + Cross * Width - FVector(0,0,Width * .35f);
        const FVector Right = Fold - Cross * Width - FVector(0,0,Width * .35f);
        Mesh.Quad(At, Right, Tip, Fold, Tint);
        Mesh.Quad(At, Fold, Tip, Left, Tint * .85f);
        Mesh.Quad(At, Fold, Tip, Right, Tint * .46f);
        Mesh.Quad(At, Left, Tip, Fold, Tint * .46f);
    }

    void Maple(FMesh& Mesh, FVector Base, float Size, int32 Season, FRandomStream& Random)
    {
        const FLinearColor Bark = Color(0x503c34);
        const FLinearColor Palette[][5] = {
            { Color(0x9e3922), Color(0xbe5826), Color(0xcd7028), Color(0xe39434), Color(0xf2ba55) },
            { Color(0x7d352e), Color(0xaa482d), Color(0xd46335), Color(0xdf8d4b), Color(0xe9a15a) },
            { Color(0x615f36), Color(0x79713a), Color(0xa18e42), Color(0xcab45e), Color(0xe3c977) }
        };

        const FVector Root = Base + FVector(0,0,Size * .15);
        const FVector Fork = Base + FVector(-Size * .11,5,Size * .63);
        const FVector Crown = Base + FVector(Size * .07,3,Size);
        Mesh.Beam(Base - FVector(0,0,9), Root, Size * .12, Size * .06, Bark);
        Mesh.Beam(Root, Fork, Size * .06, Size * .034, Bark);
        Mesh.Beam(Fork, Crown, Size * .034, Size * .009, Bark);
        for (int32 Branch = 0; Branch < 11; ++Branch)
        {
            const float Angle = Branch * 2.39996f;
            const float Span = Size * Random.FRandRange(.21f, .50f);
            const FVector Reach(FMath::Cos(Angle) * Span, FMath::Sin(Angle) * Span,
                Size * Random.FRandRange(.03f, .25f));
            const FVector Source = FMath::Lerp(Fork, Crown, Random.FRandRange(0.04f, .95f));
            const FVector End = Source + Reach;
            Mesh.Beam(Source, End, Size * .024, Size * .004, Bark, 6);
            for (int32 Cluster = 0; Cluster < 3; ++Cluster)
            {
                const FVector Twig = End + FVector(Random.FRandRange(-.16f, .16f) * Size,
                    Random.FRandRange(-.16f, .16f) * Size, Random.FRandRange(.02f, .19f) * Size);
                Mesh.Beam(End - Reach * .25, Twig, Size * .009, Size * .002, Bark, 5);
                for (int32 Index = 0; Index < 32; ++Index)
                {
                    FVector Spread = Random.VRand() * (Size * Random.FRandRange(.015f, .15f));
                    Spread.Z *= .61;
                    const FLinearColor Tint = Palette[Season][Random.RandRange(0, 4)];
                    Leaf(Mesh, Twig + Spread, Size * Random.FRandRange(.053f, .123f),
                        Size * Random.FRandRange(.019f, .039f), Tint, Random);
                }
            }
        }
    }

    void Grass(FMesh& Mesh, FVector At, float Height, int32 Density, FRandomStream& Random)
    {
        for (int32 Index = 0; Index < Density; ++Index)
        {
            const FVector Foot = At + FVector(Random.FRandRange(-13, 13), Random.FRandRange(-13, 13), 0);
            const FVector Tip = Foot + FVector(Random.FRandRange(-9, 9), Random.FRandRange(-9, 9),
                Random.FRandRange(.25f, 1.f) * Height);
            const FVector Half = FVector::CrossProduct(Tip - Foot, FVector::UpVector).GetSafeNormal() * 1.9;
            const FLinearColor Tint = Color(Index % 3 == 0 ? 0x9c8d51 : 0x646847);
            Mesh.Triangle(Foot - Half, Foot + Half, Tip, Tint);
            Mesh.Triangle(Tip, Foot + Half, Foot - Half, Tint * .76f);
        }
    }

    void Column(FMesh& Mesh, FVector At, float Height, bool bCapital)
    {
        const FLinearColor Stone = Color(0xa09478);
        const FLinearColor Band = Color(0x797d6a);
        Mesh.Box(At + FVector(0,0,6), {65,65,15}, 5, Stone);
        Mesh.Box(At + FVector(0,0,18), {49,49,12}, 3, Band);
        for (float Z = 24; Z < Height - 4; Z += 27)
        {
            const float BlockHeight = FMath::Min(26.5f, Height - Z);
            Mesh.Box(At + FVector(0,0,Z + BlockHeight * .5), {39,39,BlockHeight}, 3.5,
                Stone * (.92f + .06f * FMath::Sin(Z)));
        }
        if (bCapital)
        {
            Mesh.Box(At + FVector(0,0,Height - 1), {52,55,14}, 4.5, Color(0xb4a485));
        }
    }

    void Lantern(FMesh& Stone, FMesh& Glimmer, FVector At)
    {
        const FLinearColor Dark = Color(0x54675d);
        Column(Stone, At, 57, false);
        Stone.Box(At + FVector(0,0,61), {48,47,8}, 3, Dark);
        Stone.Box(At + FVector(0,0,85), {52,48,9}, 3, Dark, Turn(8));
        Stone.Gem(At + FVector(0,0,94), {20,20,12}, Color(0x819181), 4);
        for (int32 X : {-1, 1})
        {
            for (int32 Y : {-1, 1})
            {
                Stone.Box(At + FVector(X * 16., Y * 16., 74), {4,4,22}, 1.0, Dark);
            }
        }
        Glimmer.Gem(At + FVector(0,0,74), {11,11,10}, Color(0xffbb67), 4);
    }

    void BrokenArch(FMesh& Stone, FMesh& Glow, FVector At)
    {
        constexpr float Radius = 91.f;
        constexpr float Eave = 130.f;
        Column(Stone, At + FVector(-Radius,0,0), Eave, true);
        Column(Stone, At + FVector(Radius,0,0), Eave, true);
        Stone.Box(At + FVector(0,0,1), {289,124,23}, 10, Color(0x848774));
        for (int32 Index = 0; Index < 10; ++Index)
        {
            const float Angle = UE_PI * (static_cast<float>(Index) + .5f) / 10.f;
            const FVector Point(FMath::Cos(Angle) * Radius, 0, Eave + FMath::Sin(Angle) * Radius);
            const FQuat Tangent(FVector::YAxisVector, -Angle - .5f * UE_PI);
            Stone.Box(At + Point, {28,43,37}, 5,
                Color(Index % 2 == 0 ? 0xa99b80 : 0xb9a88c), Tangent);
        }
        Stone.Box(At + FVector(0,3,Eave + Radius + 4), {30,57,40}, 8, Color(0x63796d));
        Glow.Gem(At + FVector(0,33,Eave + Radius + 6), {10,5,16}, Color(0x78cfb1), 4);
    }
}

void AVaultCourtyard::BuildSurrounds(FIntPoint Size, int32 Sector)
{
    const float Right = (Size.X - 1) * 100.f;
    const float Front = (Size.Y - 1) * 100.f;
    const FVector Center(Right * .5, Front * .5, 0);

    FMesh Water;
    Water.Quad(Center + FVector(-18000,-18000,-236), Center + FVector(18000,-18000,-236),
        Center + FVector(18000,18000,-236), Center + FVector(-18000,18000,-236), FLinearColor::White);
    Water.Apply(*Basin, 0, WaterMaterial);

    FMesh Land, Lights;
    FRandomStream Random(9007 + Sector * 23);
    const FLinearColor Rock[] = {
        Color(0x666b5d), Color(0x7c7965), Color(0x89907c), Color(0x737761), Color(0x5b6e60)
    };
    Land.Box(Center - FVector(0,0,101), {Right + 141, Front + 139, 186}, 31, Color(0x697466));
    Land.Box(Center - FVector(0,0,47), {Right + 145, Front + 144, 31}, 10, Color(0x8b8971));
    for (int32 X = -1; X <= Size.X; ++X)
    {
        for (int32 Side : {-1, 1})
        {
            const FVector At(X * 100.f + Random.FRandRange(-25, 25),
                Side < 0 ? -110. : Front + 82., Random.FRandRange(-160.f, -125.f));
            Land.Rock(At, {Random.FRandRange(60,95), Random.FRandRange(63,104),
                Random.FRandRange(52,79)}, Rock[Random.RandRange(0, 4)], Random);
        }
    }
    for (int32 Y = 0; Y < Size.Y; ++Y)
    {
        for (int32 Side : {-1, 1})
        {
            const FVector At(Side < 0 ? -80. : Right + 89., Y * 100.f, Random.FRandRange(-142, -108));
            Land.Rock(At, {Random.FRandRange(57, 93), Random.FRandRange(69, 91), 90},
                Rock[Random.RandRange(0, 4)], Random);
            if (Y % 2 == 0)
            {
                Grass(Land, At + FVector(Side * 20,0,86), 17, 9, Random);
            }
        }
    }

    Land.Box({Center.X,-159,-44}, {380,244,81}, 15, Color(0x737b69));
    for (int32 X = -2; X <= 2; ++X)
    {
        Land.Box({Center.X + X * 62.,-137,-.9}, {61,137,12}, 4, Color(0x98947c));
    }
    BrokenArch(Land, Lights, {Center.X,-196,4});
    Lantern(Land, Lights, {-77,Front - 12,-20});
    Lantern(Land, Lights, {Right + 80,-29,-14});

    const FVector TreeSites[] = {
        {-102,-177,-42}, {Right + 132,-168,-62}, {Right + 188, Front * .41,-83},
        {-133, Front + 82,-140}
    };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(TreeSites); ++Index)
    {
        const FVector Site = TreeSites[Index];
        Land.Rock(Site - FVector(0,0,46), {115,106,63}, Rock[Index], Random);
        Maple(Land, Site, Index == 3 ? 104 : 169 + Index * 4, (Index + Sector) % 3, Random);
        for (int32 Plant = 0; Plant < 5; ++Plant)
        {
            Grass(Land, Site + FVector(Random.FRandRange(-72,72), Random.FRandRange(-59,59), 4),
                25, 11, Random);
        }
    }
    Column(Land, {Right + 313, Front * .67,-80}, 104, false);
    Land.Rock({Right + 318, Front * .67,-173}, {123,110,104}, Rock[1], Random);
    Column(Land, {-252,-322,-132}, 162, false);
    Land.Rock({-270,-311,-199}, {140,147,133}, Rock[0], Random);
    for (int32 Pebble = 0; Pebble < 23; ++Pebble)
    {
        const int32 Side = Pebble % 4;
        const float Gap = Random.FRandRange(150, 370);
        const float X = Side == 0 ? -Gap : Side == 1 ? Right + Gap : Random.FRandRange(-70, Right + 70);
        const float Y = Side == 2 ? -Gap : Side == 3 ? Front + Gap : Random.FRandRange(-70, Front + 70);
        const float Scale = Random.FRandRange(16, 49);
        Land.Rock({X,Y,-233}, {Scale * 1.32,Scale,Scale * .62}, Rock[Pebble % 5], Random);
    }
    for (int32 LeafIndex = 0; LeafIndex < Size.X * 4; ++LeafIndex)
    {
        const float X = Random.FRandRange(0, Right);
        const float Y = LeafIndex % 2 == 0 ? Random.FRandRange(-69, 38) : Random.FRandRange(Front - 36, Front + 50);
        const float Z = FMath::IsWithinInclusive(Y, -40.f, Front + 40.f) ? 46.5f : -14.5f;
        Leaf(Land, {X,Y,Z}, Random.FRandRange(4, 13), 3.4f, Color(0xb77a45), Random);
    }
    Land.Apply(*Surrounds, 0, StoneMaterial);
    Lights.Apply(*Surrounds, 1, RuneMaterial);
}
