#include "VaultGeometry.h"

using namespace VaultArt;

namespace
{
    TArray<FVector2D, TInlineAllocator<16>> Circle(int32 Sides)
    {
        TArray<FVector2D, TInlineAllocator<16>> Shape;
        for (int32 Side = 0; Side < Sides; ++Side)
        {
            const float Angle = 2.f * UE_PI * Side / Sides;
            Shape.Emplace(FMath::Cos(Angle), FMath::Sin(Angle));
        }
        return Shape;
    }
}

void FMesh::Face(FVector A, FVector B, FVector C, FVector D, FLinearColor Tint, bool bQuad)
{
    const FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
    if (Normal.IsNearlyZero())
    {
        return;
    }

    const int32 First = Vertices.Num();
    const FVector Corners[] = { A, B, C, D };
    const FVector2D UV[] = { {0, 0}, {1, 0}, {1, 1}, {0, 1} };
    const FProcMeshTangent Tangent((B - A).GetSafeNormal(), false);
    for (int32 Index = 0; Index < (bQuad ? 4 : 3); ++Index)
    {
        Vertices.Add(Corners[Index]);
        Normals.Add(Normal);
        UVs.Add(UV[Index]);
        Colors.Add(Tint);
        Tangents.Add(Tangent);
    }
    Indices.Append({ First, First + 2, First + 1 });
    if (bQuad)
    {
        Indices.Append({ First, First + 3, First + 2 });
    }
}

void FMesh::Profile(FVector At, TConstArrayView<FVector2D> Shape, TConstArrayView<FVector> Rings,
    FLinearColor Tint, FQuat Rotation)
{
    const auto Point = [&](int32 Ring, int32 Side)
    {
        const FVector R = Rings[Ring];
        const FVector2D P = Shape[Side];
        return At + Rotation.RotateVector(FVector(P.X * R.X, P.Y * R.Y, R.Z));
    };
    for (int32 Side = 0; Side < Shape.Num(); ++Side)
    {
        const int32 Next = (Side + 1) % Shape.Num();
        for (int32 Ring = 1; Ring < Rings.Num(); ++Ring)
        {
            Quad(Point(Ring - 1, Side), Point(Ring - 1, Next),
                Point(Ring, Next), Point(Ring, Side), Tint);
        }
        const int32 Last = Rings.Num() - 1;
        Triangle(At + Rotation.RotateVector(FVector(0, 0, Rings[0].Z)),
            Point(0, Next), Point(0, Side), Tint);
        Triangle(At + Rotation.RotateVector(FVector(0, 0, Rings[Last].Z)),
            Point(Last, Side), Point(Last, Next), Tint);
    }
}

void FMesh::Box(FVector At, FVector Size, float Bevel, FLinearColor Tint, FQuat Rotation)
{
    const FVector R = Size * 0.5;
    const float B = FMath::Min3(Bevel, static_cast<float>(R.Z * 0.85),
        static_cast<float>(FMath::Min(R.X, R.Y) * 0.42));
    const float BX = B / R.X;
    const float BY = B / R.Y;
    const FVector2D Shape[] = {
        {-1 + BX, -1}, {1 - BX, -1}, {1, -1 + BY}, {1, 1 - BY},
        {1 - BX, 1}, {-1 + BX, 1}, {-1, 1 - BY}, {-1, -1 + BY}
    };
    const FVector Outline[] = {
        {R.X - B, R.Y - B, -R.Z}, {R.X, R.Y, -R.Z + B}, {R.X, R.Y, R.Z - B},
        {R.X - B, R.Y - B, R.Z}
    };
    Profile(At, Shape, Outline, Tint, Rotation);
}

void FMesh::Rock(FVector At, FVector Radius, FLinearColor Tint, FRandomStream& Random)
{
    auto Shape = Circle(7);
    for (FVector2D& Point : Shape)
    {
        Point *= Random.FRandRange(0.76f, 1.13f);
    }
    const FVector Outline[] = {
        { Radius.X * .67, Radius.Y * .70, -Radius.Z },
        { Radius.X, Radius.Y, -Radius.Z * .44 },
        { Radius.X * .89, Radius.Y * .90, Radius.Z * .58 },
        { Radius.X * .51, Radius.Y * .54, Radius.Z }
    };
    const FQuat Rotation = FRotator(Random.FRandRange(-9, 9), Random.FRandRange(0, 360),
        Random.FRandRange(-9, 9)).Quaternion();
    Profile(At, Shape, Outline, Tint, Rotation);
}

void FMesh::Ring(FVector At, float Inner, float Outer, int32 Sides, FLinearColor Tint, float Opening)
{
    for (int32 Side = 0; Side < Sides; ++Side)
    {
        const float Start = 2.f * UE_PI * (Side + Opening) / Sides;
        const float End = 2.f * UE_PI * (Side + 1 - Opening) / Sides;
        const FVector A(FMath::Cos(Start), FMath::Sin(Start), 0);
        const FVector B(FMath::Cos(End), FMath::Sin(End), 0);
        Quad(At + A * Inner, At + A * Outer, At + B * Outer, At + B * Inner, Tint);
    }
}

void FMesh::Beam(FVector From, FVector To, float Start, float End, FLinearColor Tint, int32 Sides)
{
    const FVector Axis = To - From;
    const FQuat Rotation = FQuat::FindBetweenNormals(FVector::UpVector, Axis.GetSafeNormal());
    const FVector Outline[] = { { Start, Start, 0 }, { End, End, Axis.Length() } };
    Profile(From, Circle(Sides), Outline, Tint, Rotation);
}

void FMesh::Gem(FVector At, FVector Radius, FLinearColor Tint, int32 Sides)
{
    const FVector Outline[] = {
        { Radius.X * .07, Radius.Y * .07, -Radius.Z },
        { Radius.X, Radius.Y, -Radius.Z * .28 },
        { Radius.X * .8, Radius.Y * .8, Radius.Z * .37 },
        { Radius.X * .06, Radius.Y * .06, Radius.Z }
    };
    Profile(At, Circle(Sides), Outline, Tint, FQuat::Identity);
}

void FMesh::Apply(UProceduralMeshComponent& Component, int32 Section, UMaterialInterface* Material) const
{
    Component.CreateMeshSection_LinearColor(Section, Vertices, Indices, Normals,
        UVs, Colors, Tangents, false, false);
    Component.SetMaterial(Section, Material);
}
