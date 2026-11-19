#include "VaultHUD.h"

#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Fonts/CompositeFont.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/Paths.h"
#include "Rendering/SlateRenderer.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SWindow.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    FLinearColor Pigment(uint32 RGB)
    {
        return FLinearColor(FColor((RGB >> 16) & 255, (RGB >> 8) & 255, RGB & 255));
    }

    const FLinearColor Ink = Pigment(0x2f3024);
    const FLinearColor FadedInk = Pigment(0x635441);
    const FLinearColor Rule = Pigment(0xae8c60);
    const FLinearColor PaperEdge = Pigment(0x644e33);
    const FLinearColor Jade = Pigment(0x28554e);
    const FLinearColor Gold = Pigment(0xb7793c);
    const FLinearColor Light = Pigment(0xffefce);
    const FLinearColor Vermilion = Pigment(0x964332);
}

class STideJournalText : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(STideJournalText) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& Args)
    {
        SetVisibility(EVisibility::HitTestInvisible);
    }

    void ResetFrame(FVector2D Viewport)
    {
        PixelSize = Viewport;
        Labels.Reset();
    }

    void Add(FString Value, FVector2D Position, FSlateFontInfo Font, FLinearColor Color)
    {
        Labels.Add({MoveTemp(Value), Position, MoveTemp(Font), Color});
    }

    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override
    {
        return FVector2D::ZeroVector;
    }

    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
        const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 Layer,
        const FWidgetStyle& Style, bool bParentEnabled) const override
    {
        if (PixelSize.X <= 0 || PixelSize.Y <= 0)
        {
            return Layer;
        }

        const FVector2D LocalPerPixel = AllottedGeometry.GetLocalSize() / PixelSize;
        for (const FLabel& Entry : Labels)
        {
            const FVector2f At(Entry.Position * LocalPerPixel);
            FSlateFontInfo Font = Entry.Font;
            Font.Size = FMath::Max(4.f, Font.Size * static_cast<float>(LocalPerPixel.Y));
            FSlateDrawElement::MakeText(Elements, Layer,
                AllottedGeometry.ToPaintGeometry(FVector2f(1, 1), FSlateLayoutTransform(At)),
                Entry.Value, Font, ESlateDrawEffect::None, Entry.Color);
        }
        return Layer;
    }

private:
    struct FLabel
    {
        FString Value;
        FVector2D Position;
        FSlateFontInfo Font;
        FLinearColor Color;
    };

    FVector2D PixelSize = FVector2D::ZeroVector;
    TArray<FLabel> Labels;
};

AVaultHUD::AVaultHUD()
{
    static ConstructorHelpers::FObjectFinder<UTexture2D> Surface(TEXT("/Game/Art/UI/T_JournalPaper"));
    JournalPaper = Surface.Object;
}

void AVaultHUD::PrepareTypefaces()
{
    const FString Folder = FPaths::ProjectContentDir() / TEXT("Slate/Fonts");
    BodyTypeface = MakeShared<FCompositeFont>(TEXT("Regular"), Folder / TEXT("NotoSansSC-Regular.otf"),
        EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
    TitleTypeface = MakeShared<FCompositeFont>(TEXT("Semibold"), Folder / TEXT("NotoSerifSC-SemiBold.otf"),
        EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
}

void AVaultHUD::BeginJournalTextFrame()
{
    if (!JournalText)
    {
        UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
        if (!Viewport)
        {
            return;
        }
        JournalViewport = Viewport;
        SAssignNew(JournalText, STideJournalText);
        Viewport->AddViewportWidgetContent(JournalText.ToSharedRef(), 5);
        if (GetWorld()->WorldType == EWorldType::Game)
        {
            if (const TSharedPtr<SWindow> Window = Viewport->GetWindow())
            {
                Window->SetTitle(FText::FromString(TEXT("暮潮引灯人")));
            }
        }
    }
    JournalText->ResetFrame(FVector2D(Canvas->SizeX, Canvas->SizeY));
}

void AVaultHUD::FinishJournalTextFrame()
{
    if (JournalText)
    {
        JournalText->Invalidate(EInvalidateWidgetReason::Paint);
    }
}

void AVaultHUD::EndPlay(const EEndPlayReason::Type Reason)
{
    if (JournalText && JournalViewport.IsValid())
    {
        JournalViewport->RemoveViewportWidgetContent(JournalText.ToSharedRef());
    }
    JournalText.Reset();
    Super::EndPlay(Reason);
}

FSlateFontInfo AVaultHUD::FontAt(float Height, bool bHeading) const
{
    // Canvas font sizes are in points and follow the current viewport scale.
    return FSlateFontInfo(bHeading ? TitleTypeface : BodyTypeface, FMath::Max(4.f, Height * Scale * .75f));
}

float AVaultHUD::TextWidth(const FString& Value, float Height, bool bHeading) const
{
    const auto FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    return FontMeasure->Measure(Value, FontAt(Height, bHeading)).X / Scale;
}

void AVaultHUD::Label(const FString& Value, float X, float Y, float Height, FLinearColor Color, bool bHeading)
{
    if (JournalText && !Value.IsEmpty())
    {
        JournalText->Add(Value, Origin + FVector2D(X, Y) * Scale, FontAt(Height, bHeading), Color);
    }
}

void AVaultHUD::Paragraph(const FString& Value, float X, float Y, float Width, float Height, FLinearColor Color)
{
    float SoftWidth = Width - Height;
    if (!Value.Contains(TEXT("\n")))
    {
        const float FullWidth = TextWidth(Value, Height);
        if (FullWidth <= Width)
        {
            Label(Value, X, Y, Height, Color);
            return;
        }
        if (FullWidth < Width * 1.55f)
        {
            SoftWidth = FMath::Clamp(FullWidth * .58f, Width * .64f, SoftWidth);
        }
    }

    // Break CJK lines by glyph, keeping closing punctuation beside the preceding glyph.
    const FString Closing = TEXT("，。！？、；：）》」");
    FString Line;
    for (TCHAR Glyph : Value)
    {
        if (Glyph == TEXT('\n'))
        {
            Label(Line, X, Y, Height, Color);
            Y += Height * 1.42f;
            Line.Reset();
            continue;
        }

        const FString Candidate = Line + Glyph;
        if (!Line.IsEmpty() && !Closing.Contains(FString::Chr(Glyph)) && TextWidth(Candidate, Height) > SoftWidth)
        {
            Label(Line, X, Y, Height, Color);
            Y += Height * 1.42f;
            Line = FString::Chr(Glyph);
        }
        else
        {
            Line = Candidate;
        }
    }
    Label(Line, X, Y, Height, Color);
}

void AVaultHUD::Paper(float X, float Y, float W, float H)
{
    Rect(X + 5, Y + 7, W, H, FLinearColor(.017f, .024f, .018f, .44f));
    Rect(X, Y, W, H, Pigment(0xe9d5ab));
    if (JournalPaper)
    {
        DrawTexture(JournalPaper, Origin.X + X * Scale, Origin.Y + Y * Scale,
            W * Scale, H * Scale, 0, 0, 1, 1);
    }
    Frame(X, Y, W, H, PaperEdge);
    Frame(X + 7, Y + 7, W - 14, H - 14, Rule);
    for (int32 SX : { -1, 1 })
    {
        for (int32 SY : { -1, 1 })
        {
            const float CX = SX < 0 ? X + 11 : X + W - 11;
            const float CY = SY < 0 ? Y + 11 : Y + H - 11;
            Rect(SX < 0 ? CX : CX - 9, CY, 9, 1, PaperEdge);
            Rect(CX, SY < 0 ? CY : CY - 9, 1, 9, PaperEdge);
        }
    }
}

void AVaultHUD::LanternSeal(float X, float Y, float Side)
{
    const FLinearColor Rim = Pigment(0xefbf75);
    Rect(X, Y, Side, Side, Vermilion);
    Frame(X + 3, Y + 3, Side - 6, Side - 6, Rim);
    const auto Mark = [&](float AX, float AY, float BX, float BY)
    {
        DrawLine(Origin.X + (X + AX * Side) * Scale, Origin.Y + (Y + AY * Side) * Scale,
            Origin.X + (X + BX * Side) * Scale, Origin.Y + (Y + BY * Side) * Scale, Light, 1.6f * Scale);
    };
    Mark(.43f, .38f, .43f, .27f);
    Mark(.43f, .27f, .57f, .27f);
    Mark(.57f, .27f, .57f, .38f);
    Mark(.34f, .38f, .66f, .38f);
    Mark(.34f, .38f, .30f, .67f);
    Mark(.66f, .38f, .70f, .67f);
    Mark(.30f, .67f, .70f, .67f);
    Mark(.41f, .75f, .59f, .75f);
    Mark(.50f, .46f, .50f, .60f);
}

void AVaultHUD::DrawJournalChrome()
{
    if (Screen == EScreen::Home)
    {
        return;
    }
    Paper(48, 37, 257, 55);
    LanternSeal(64, 48, 32);
    Label(TEXT("暮潮引灯人"), 109, 48, 27, Ink, true);
}

void AVaultHUD::Footnote(const FString& Value)
{
    const float Width = FMath::Min(1180.f, TextWidth(Value, 17) + 36.f);
    Rect(50, 739, Width, 40, FLinearColor(Jade.R, Jade.G, Jade.B, .97f));
    Frame(50, 739, Width, 40, Pigment(0xc7aa76));
    Label(Value, 66, 747, 17, Light);
}

void AVaultHUD::DrawHome()
{
    Paper(52, 190, 376, 400);
    LanternSeal(78, 217, 42);
    Label(TEXT("暮潮引灯人"), 79, 278, 51, Ink, true);
    Rect(79, 353, 322, 1, Rule);
    Paragraph(TEXT("渡船迟迟未归。玛拉带上父亲的灯石，\n沿着手记点亮两岸航标。"),
        79, 372, 322, 17, FadedInk);

    Button(TEXT("开始游玩"), 79, 459, 322, 44, EAction::Start, 0, true);
    Button(TEXT("退出游戏"), 79, 516, 322, 40, EAction::Quit);
}

void AVaultHUD::DrawDispatch()
{
    const TArray<FVaultLevel>& Levels = FVaultLevel::Campaign();
    const bool bEnding = Cleared.Contains(Levels.Num() - 1);
    if (bEnding)
    {
        Paper(48, 113, 405, 84);
        Paragraph(TEXT("对岸应了一声铃响，一盏灯慢慢靠近。"), 65, 133, 371, 17, Ink);
    }

    Paper(934, 112, 296, 549);
    Label(TEXT("渡口手记"), 956, 132, 29, Ink, true);
    Rect(956, 178, 252, 1, Rule);

    for (int32 Index = 0; Index < Levels.Num(); ++Index)
    {
        const FString Row = FString::Printf(TEXT("%02d    %s"), Index + 1, *Levels[Index].Title);
        const float Y = 190 + Index * 34;
        Button(Row, 956, Y, 252, 30, EAction::Campaign, Index, Index == 0);
        if (Cleared.Contains(Index))
        {
            Label(TEXT("已点亮"), 1151, Y + 5, 15, Index == 0 ? Light : Jade);
        }
    }

    Rect(956, 434, 252, 1, Rule);
    Button(TEXT("编辑关卡"), 956, 445, 252, 36, EAction::Workshop);
    Label(TEXT("自制关卡"), 956, 494, 18, Ink, true);
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Slots); ++Index)
    {
        const FString SlotLabel = Slots[Index] ? FString::Printf(TEXT("存档 %d"), Index + 1)
            : FString::Printf(TEXT("空档 %d"), Index + 1);
        Button(SlotLabel, 956 + Index * 85, 527, 82, 32, EAction::Custom, Index, false, Slots[Index]);
    }
    Button(TEXT("返回首页"), 956, 601, 252, 35, EAction::Back);
    Footnote(TEXT("回车  第一章　　E  编辑关卡"));
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
    const FString Mode = bTesting ? TEXT("草稿试玩")
        : (Stage == INDEX_NONE ? TEXT("自制关卡") : FString::Printf(TEXT("第 %d 章  /  共 %d 章"),
            Stage + 1, FVaultLevel::Campaign().Num()));

    Paper(824, 97, 405, 621);
    Label(Mode, 851, 119, 17, Vermilion);
    Label(Level.Title, 849, 155, 40, Ink, true);
    Rect(849, 221, 355, 1, Rule);
    Label(TEXT("航标"), 849, 243, 17, FadedInk);
    Label(TEXT("行动次数"), 974, 243, 17, FadedInk);
    Label(TEXT("推灯次数"), 1090, 243, 17, FadedInk);
    Label(FString::Printf(TEXT("%d / %d"), Board.ChargedCount(), State.Batteries.Num()),
        849, 270, 29, Jade, true);
    Label(FString::FromInt(State.Moves), 974, 270, 29, Ink, true);
    Label(FString::FromInt(State.Pushes), 1090, 270, 29, Ink, true);
    Rect(849, 325, 355, 1, Rule);

    if (bComplete)
    {
        LanternSeal(848, 353, 46);
        Label(bEnding ? TEXT("渡船应铃") : TEXT("航标已点亮"), 909, 352, 33, Ink, true);
        Paragraph(bEnding ? TEXT("铃声越过水面，船头的灯摇了两下。渡船回来了。")
            : TEXT("又一盏灯照亮了归途。"), 849, 421, 355, 20, FadedInk);
        const bool bHasNext = Stage != INDEX_NONE && FVaultLevel::Campaign().IsValidIndex(Stage + 1);
        Button(bTesting ? TEXT("返回编辑　[回车]") : (bHasNext ? TEXT("下一章　[回车]")
            : TEXT("返回渡口　[回车]")), 849, 501, 355, 46, EAction::Advance, 0, true);
    }
    else
    {
        Rect(849, 344, 355, 44, Pigment(0xdfc79a));
        Rect(849, 344, 3, 44, bHasRelay && !bPowered ? Vermilion : Jade);
        Label(bHasRelay ? (bPowered ? TEXT("闸门已开启") : TEXT("闸门已关闭"))
            : (bHasMirrors ? TEXT("潮镜") : (bHasWater ? TEXT("潮汐水道") : TEXT("渡口"))),
            863, 354, 18, bHasRelay && !bPowered ? Vermilion : Jade);
        if (bHasMirrors)
        {
            const bool bCanExchange = Board.CanExchange();
            Button(TEXT("潮镜交换　[Q]"), 1028, 347, 173, 38, EAction::Exchange, 0, bCanExchange, bCanExchange);
        }
        Paragraph(Level.Hint, 849, 403, 355, 18, Ink);
        if (!Level.Journal.IsEmpty())
        {
            Rect(849, 478, 355, 83, Pigment(0xf2e2ba));
            Rect(849, 478, 2, 83, Gold);
            Label(TEXT("渡口日志"), 862, 485, 16, Vermilion, true);
            Paragraph(Level.Journal, 862, 512, 331, 16, FadedInk);
        }
    }

    Button(TEXT("撤销　[Z]"), 849, 578, 171, 43, EAction::Undo, 0, false, State.Moves > 0);
    Button(TEXT("重开　[R]"), 1031, 578, 173, 43, EAction::Reset);
    Button(bTesting ? TEXT("返回编辑　[退格]") : TEXT("返回渡口　[退格]"),
        849, 639, 355, 43, EAction::Back);
    Footnote(bHasMirrors ? TEXT("方向键或 WASD  移动　　Q  在潮镜边交换")
        : TEXT("方向键或 WASD  移动"));
}

void AVaultHUD::DrawWorkshop()
{
    if (bWorkshopPreview)
    {
        Paper(51, 126, 164, 48);
        Label(TEXT("立体预览"), 70, 134, 23, Ink, true);
    }
    else
    {
        Paper(51, 127, 752, 591);
        Label(TEXT("关卡草图"), 75, 136, 23, Ink, true);
        DrawDraft();
    }
    if (bWorkshopPreview)
    {
        Paper(599, 126, 196, 48);
    }
    Button(bWorkshopPreview ? TEXT("返回草图") : TEXT("立体预览"),
        611, 134, 172, 31, EAction::TogglePreview, 0, bWorkshopPreview);

    Paper(824, 96, 405, 622);
    Label(TEXT("关卡编辑"), 849, 114, 33, Ink, true);
    Rect(849, 163, 355, 1, Rule);
    const TCHAR* Names[] = { TEXT("干地"), TEXT("石墙"), TEXT("航标"), TEXT("石印"),
        TEXT("闸门"), TEXT("水道"), TEXT("潮镜"), TEXT("灯石"), TEXT("玛拉") };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
    {
        Button(Names[Index], 849 + (Index % 2) * 180, 179 + (Index / 2) * 35, 171, 31,
            EAction::Tool, Index, static_cast<int32>(Tool) == Index);
    }
    Button(TEXT("清空棋盘"), 1029, 319, 171, 31, EAction::Clear);
    Rect(849, 363, 351, 1, Rule);

    const auto Dimension = [this](int32 X, const TCHAR* Name, int32 Size, EAction Action)
    {
        Label(FString::Printf(TEXT("%s %d"), Name, Size), X, 381, 17, FadedInk);
        Button(TEXT("−"), X + 80, 375, 38, 32, Action, -1, false, Size > FVaultLevel::MinSide);
        Button(TEXT("+"), X + 125, 375, 38, 32, Action, 1, false, Size < FVaultLevel::MaxSide);
    };
    Dimension(849, TEXT("宽"), Draft.Size.X, EAction::Width);
    Dimension(1033, TEXT("高"), Draft.Size.Y, EAction::Height);

    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Slots); ++Index)
    {
        const FString Name = FString::Printf(TEXT("存档 %d%s"), Index + 1, Slots[Index] ? TEXT(" *") : TEXT(""));
        Button(Name, 849 + Index * 120, 425, 111, 34, EAction::Slot, Index, Slot == Index);
    }

    Button(TEXT("保存"), 849, 473, 171, 40, EAction::Save);
    Button(TEXT("载入"), 1029, 473, 171, 40, EAction::Open, 0, false, Slots[Slot]);
    Button(TEXT("试玩　[回车]"), 849, 528, 351, 46, EAction::Test, 0, true);
    Paragraph(Notice, 849, 590, 351, 17, bError ? Vermilion : Jade);
    Button(TEXT("返回渡口　[退格]"), 849, 644, 351, 40, EAction::Back);
    Footnote(bWorkshopPreview ? TEXT("选择工具返回草图绘制　　回车  试玩")
        : TEXT("点击或拖动  绘制　　右键  擦除　　Z  撤销编辑"));
}

void AVaultHUD::Button(const FString& Value, float X, float Y, float W, float H, EAction Action,
    int32 Argument, bool bAccent, bool bEnabled)
{
    const FBox2D Bounds({ X, Y }, { X + W, Y + H });
    const bool bHover = bEnabled && Bounds.IsInside(Mouse);
    const bool bChapter = Action == EAction::Campaign;
    if (bAccent || bHover || !bChapter)
    {
        Rect(X, Y, W, H, bAccent ? (bHover ? Pigment(0x376d62) : Jade)
            : (bHover ? Pigment(0xf8eaca) : Pigment(0xe1c99d)));
    }
    if (bChapter)
    {
        Rect(X, Y + H - 1, W, 1, Rule);
    }
    else
    {
        Frame(X, Y, W, H, bAccent ? Pigment(0xc7aa76) : Rule);
    }

    const bool bTool = Action == EAction::Tool;
    if (bTool)
    {
        const float IconSize = H - 10;
        const float IY = Y + 5;
        const ETool PaletteItem = static_cast<ETool>(Argument);
        if (PaletteItem == ETool::Battery || PaletteItem == ETool::Robot)
        {
            DrawTile(EVaultTile::Floor, X + 10, IY, IconSize, false, false);
            if (PaletteItem == ETool::Battery) { DrawBattery(X + 10, IY, IconSize, EVaultTile::Floor); }
            else { DrawRobot(X + 10, IY, IconSize); }
        }
        else
        {
            DrawTile(static_cast<EVaultTile>(PaletteItem), X + 10, IY, IconSize, false, false);
        }
    }
    const float Height = FMath::Min(20.f, H * .54f);
    const float Baseline = Y + (H - Height) * .5f - 1;
    const float LabelX = bTool ? X + 42
        : (bChapter ? X + 13 : X + (W - TextWidth(Value, Height)) * .5f);
    Label(Value, LabelX, Baseline, Height, bAccent ? Light : (bEnabled ? Ink : FadedInk));
    if (bEnabled)
    {
        Buttons.Add({ Bounds, Action, Argument });
    }
}
