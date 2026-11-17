#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"

namespace VaultArt
{
    inline FLinearColor Color(uint32 RGB)
    {
        return FLinearColor(FColor((RGB >> 16) & 255, (RGB >> 8) & 255, RGB & 255));
    }

    // The procedural mesh uses clockwise winding, with one normal per carved face.
    struct FMesh
    {
        void Triangle(FVector A, FVector B, FVector C, FLinearColor Tint)
        {
            Face(A, B, C, C, Tint, false);
        }

        void Quad(FVector A, FVector B, FVector C, FVector D, FLinearColor Tint)
        {
            Face(A, B, C, D, Tint, true);
        }

        void Box(FVector At, FVector Size, float Bevel, FLinearColor Tint, FQuat Rotation = FQuat::Identity);
        void Rock(FVector At, FVector Radius, FLinearColor Tint, FRandomStream& Random);
        void Ring(FVector At, float Inner, float Outer, int32 Sides, FLinearColor Tint,
            float Opening = 0.f);
        void Beam(FVector From, FVector To, float Start, float End, FLinearColor Tint, int32 Sides = 7);
        void Gem(FVector At, FVector Radius, FLinearColor Tint, int32 Sides = 6);
        void Apply(UProceduralMeshComponent& Component, int32 Section, UMaterialInterface* Material) const;

    private:
        TArray<FVector> Vertices;
        TArray<int32> Indices;
        TArray<FVector> Normals;
        TArray<FVector2D> UVs;
        TArray<FLinearColor> Colors;
        TArray<FProcMeshTangent> Tangents;

        void Face(FVector A, FVector B, FVector C, FVector D, FLinearColor Tint, bool bQuad);
        void Profile(FVector At, TConstArrayView<FVector2D> Shape, TConstArrayView<FVector> Rings,
            FLinearColor Tint, FQuat Rotation);
    };
}
