




#include "gfxTextRun.h"
#include "gfxGlyphExtents.h"
#include "gfxPlatformFontList.h"
#include "gfxUserFontSet.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/PathHelpers.h"
#include "nsGkAtoms.h"
#include "nsILanguageAtomService.h"
#include "nsServiceManagerUtils.h"

#include "gfxContext.h"
#include "gfxFontConstants.h"
#include "gfxFontMissingGlyphs.h"
#include "gfxScriptItemizer.h"
#include "nsUnicodeProperties.h"
#include "nsUnicodeRange.h"
#include "nsStyleConsts.h"
#include "mozilla/Likely.h"
#include "gfx2DGlue.h"

#include "cairo.h"

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::unicode;
using mozilla::services::GetObserverService;

static const char16_t kEllipsisChar[] = { 0x2026, 0x0 };
static const char16_t kASCIIPeriodsChar[] = { '.', '.', '.', 0x0 };

#ifdef DEBUG_roc
#define DEBUG_TEXT_RUN_STORAGE_METRICS
#endif

#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
extern uint32_t gTextRunStorageHighWaterMark;
extern uint32_t gTextRunStorage;
extern uint32_t gFontCount;
extern uint32_t gGlyphExtentsCount;
extern uint32_t gGlyphExtentsWidthsTotalSize;
extern uint32_t gGlyphExtentsSetupEagerSimple;
extern uint32_t gGlyphExtentsSetupEagerTight;
extern uint32_t gGlyphExtentsSetupLazyTight;
extern uint32_t gGlyphExtentsSetupFallBackToTight;
#endif

bool
gfxTextRun::GlyphRunIterator::NextRun()  {
    if (mNextIndex >= mTextRun->mGlyphRuns.Length())
        return false;
    mGlyphRun = &mTextRun->mGlyphRuns[mNextIndex];
    if (mGlyphRun->mCharacterOffset >= mEndOffset)
        return false;

    mStringStart = std::max(mStartOffset, mGlyphRun->mCharacterOffset);
    uint32_t last = mNextIndex + 1 < mTextRun->mGlyphRuns.Length()
        ? mTextRun->mGlyphRuns[mNextIndex + 1].mCharacterOffset : mTextRun->GetLength();
    mStringEnd = std::min(mEndOffset, last);

    ++mNextIndex;
    return true;
}

#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
static void
AccountStorageForTextRun(gfxTextRun *aTextRun, int32_t aSign)
{
    
    
    
    
    
    
    uint32_t length = aTextRun->GetLength();
    int32_t bytes = length * sizeof(gfxTextRun::CompressedGlyph);
    bytes += sizeof(gfxTextRun);
    gTextRunStorage += bytes*aSign;
    gTextRunStorageHighWaterMark = std::max(gTextRunStorageHighWaterMark, gTextRunStorage);
}
#endif

static bool
NeedsGlyphExtents(gfxTextRun *aTextRun)
{
    if (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_NEED_BOUNDING_BOX)
        return true;
    uint32_t numRuns;
    const gfxTextRun::GlyphRun *glyphRuns = aTextRun->GetGlyphRuns(&numRuns);
    for (uint32_t i = 0; i < numRuns; ++i) {
        if (glyphRuns[i].mFont->GetFontEntry()->IsUserFont())
            return true;
    }
    return false;
}




void *
gfxTextRun::AllocateStorageForTextRun(size_t aSize, uint32_t aLength)
{
    
    
    void *storage = malloc(aSize + aLength * sizeof(CompressedGlyph));
    if (!storage) {
        NS_WARNING("failed to allocate storage for text run!");
        return nullptr;
    }

    
    memset(reinterpret_cast<char*>(storage) + aSize, 0,
           aLength * sizeof(CompressedGlyph));

    return storage;
}

gfxTextRun *
gfxTextRun::Create(const gfxTextRunFactory::Parameters *aParams,
                   uint32_t aLength, gfxFontGroup *aFontGroup, uint32_t aFlags)
{
    void *storage = AllocateStorageForTextRun(sizeof(gfxTextRun), aLength);
    if (!storage) {
        return nullptr;
    }

    return new (storage) gfxTextRun(aParams, aLength, aFontGroup, aFlags);
}

gfxTextRun::gfxTextRun(const gfxTextRunFactory::Parameters *aParams,
                       uint32_t aLength, gfxFontGroup *aFontGroup, uint32_t aFlags)
    : gfxShapedText(aLength, aFlags, aParams->mAppUnitsPerDevUnit)
    , mUserData(aParams->mUserData)
    , mFontGroup(aFontGroup)
    , mReleasedFontGroup(false)
    , mShapingState(eShapingState_Normal)
{
    NS_ASSERTION(mAppUnitsPerDevUnit > 0, "Invalid app unit scale");
    MOZ_COUNT_CTOR(gfxTextRun);
    NS_ADDREF(mFontGroup);

#ifndef RELEASE_BUILD
    gfxTextPerfMetrics *tp = aFontGroup->GetTextPerfMetrics();
    if (tp) {
        tp->current.textrunConst++;
    }
#endif

    mCharacterGlyphs = reinterpret_cast<CompressedGlyph*>(this + 1);

    if (aParams->mSkipChars) {
        mSkipChars.TakeFrom(aParams->mSkipChars);
    }

#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
    AccountStorageForTextRun(this, 1);
#endif

    mSkipDrawing = mFontGroup->ShouldSkipDrawing();
}

gfxTextRun::~gfxTextRun()
{
#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
    AccountStorageForTextRun(this, -1);
#endif
#ifdef DEBUG
    
    mFlags = 0xFFFFFFFF;
#endif

    
    
    
    if (!mReleasedFontGroup) {
#ifndef RELEASE_BUILD
        gfxTextPerfMetrics *tp = mFontGroup->GetTextPerfMetrics();
        if (tp) {
            tp->current.textrunDestr++;
        }
#endif
        NS_RELEASE(mFontGroup);
    }

    MOZ_COUNT_DTOR(gfxTextRun);
}

void
gfxTextRun::ReleaseFontGroup()
{
    NS_ASSERTION(!mReleasedFontGroup, "doubly released!");
    NS_RELEASE(mFontGroup);
    mReleasedFontGroup = true;
}

bool
gfxTextRun::SetPotentialLineBreaks(uint32_t aStart, uint32_t aLength,
                                   uint8_t *aBreakBefore,
                                   gfxContext *aRefContext)
{
    NS_ASSERTION(aStart + aLength <= GetLength(), "Overflow");

    uint32_t changed = 0;
    uint32_t i;
    CompressedGlyph *charGlyphs = mCharacterGlyphs + aStart;
    for (i = 0; i < aLength; ++i) {
        uint8_t canBreak = aBreakBefore[i];
        if (canBreak && !charGlyphs[i].IsClusterStart()) {
            
            
            NS_WARNING("Break suggested inside cluster!");
            canBreak = CompressedGlyph::FLAG_BREAK_TYPE_NONE;
        }
        changed |= charGlyphs[i].SetCanBreakBefore(canBreak);
    }
    return changed != 0;
}

gfxTextRun::LigatureData
gfxTextRun::ComputeLigatureData(uint32_t aPartStart, uint32_t aPartEnd,
                                PropertyProvider *aProvider)
{
    NS_ASSERTION(aPartStart < aPartEnd, "Computing ligature data for empty range");
    NS_ASSERTION(aPartEnd <= GetLength(), "Character length overflow");
  
    LigatureData result;
    CompressedGlyph *charGlyphs = mCharacterGlyphs;

    uint32_t i;
    for (i = aPartStart; !charGlyphs[i].IsLigatureGroupStart(); --i) {
        NS_ASSERTION(i > 0, "Ligature at the start of the run??");
    }
    result.mLigatureStart = i;
    for (i = aPartStart + 1; i < GetLength() && !charGlyphs[i].IsLigatureGroupStart(); ++i) {
    }
    result.mLigatureEnd = i;

    int32_t ligatureWidth =
        GetAdvanceForGlyphs(result.mLigatureStart, result.mLigatureEnd);
    
    uint32_t totalClusterCount = 0;
    uint32_t partClusterIndex = 0;
    uint32_t partClusterCount = 0;
    for (i = result.mLigatureStart; i < result.mLigatureEnd; ++i) {
        
        
        
        if (i == result.mLigatureStart || charGlyphs[i].IsClusterStart()) {
            ++totalClusterCount;
            if (i < aPartStart) {
                ++partClusterIndex;
            } else if (i < aPartEnd) {
                ++partClusterCount;
            }
        }
    }
    NS_ASSERTION(totalClusterCount > 0, "Ligature involving no clusters??");
    result.mPartAdvance = partClusterIndex * (ligatureWidth / totalClusterCount);
    result.mPartWidth = partClusterCount * (ligatureWidth / totalClusterCount);

    
    
    
    if (aPartEnd == result.mLigatureEnd) {
        gfxFloat allParts = totalClusterCount * (ligatureWidth / totalClusterCount);
        result.mPartWidth += ligatureWidth - allParts;
    }

    if (partClusterCount == 0) {
        
        result.mClipBeforePart = result.mClipAfterPart = true;
    } else {
        
        
        
        
        result.mClipBeforePart = partClusterIndex > 0;
        
        
        result.mClipAfterPart = partClusterIndex + partClusterCount < totalClusterCount;
    }

    if (aProvider && (mFlags & gfxTextRunFactory::TEXT_ENABLE_SPACING)) {
        gfxFont::Spacing spacing;
        if (aPartStart == result.mLigatureStart) {
            aProvider->GetSpacing(aPartStart, 1, &spacing);
            result.mPartWidth += spacing.mBefore;
        }
        if (aPartEnd == result.mLigatureEnd) {
            aProvider->GetSpacing(aPartEnd - 1, 1, &spacing);
            result.mPartWidth += spacing.mAfter;
        }
    }

    return result;
}

gfxFloat
gfxTextRun::ComputePartialLigatureWidth(uint32_t aPartStart, uint32_t aPartEnd,
                                        PropertyProvider *aProvider)
{
    if (aPartStart >= aPartEnd)
        return 0;
    LigatureData data = ComputeLigatureData(aPartStart, aPartEnd, aProvider);
    return data.mPartWidth;
}

int32_t
gfxTextRun::GetAdvanceForGlyphs(uint32_t aStart, uint32_t aEnd)
{
    const CompressedGlyph *glyphData = mCharacterGlyphs + aStart;
    int32_t advance = 0;
    uint32_t i;
    for (i = aStart; i < aEnd; ++i, ++glyphData) {
        if (glyphData->IsSimpleGlyph()) {
            advance += glyphData->GetSimpleAdvance();   
        } else {
            uint32_t glyphCount = glyphData->GetGlyphCount();
            if (glyphCount == 0) {
                continue;
            }
            const DetailedGlyph *details = GetDetailedGlyphs(i);
            if (details) {
                uint32_t j;
                for (j = 0; j < glyphCount; ++j, ++details) {
                    advance += details->mAdvance;
                }
            }
        }
    }
    return advance;
}

static void
GetAdjustedSpacing(gfxTextRun *aTextRun, uint32_t aStart, uint32_t aEnd,
                   gfxTextRun::PropertyProvider *aProvider,
                   gfxTextRun::PropertyProvider::Spacing *aSpacing)
{
    if (aStart >= aEnd)
        return;

    aProvider->GetSpacing(aStart, aEnd - aStart, aSpacing);

#ifdef DEBUG
    

    const gfxTextRun::CompressedGlyph *charGlyphs = aTextRun->GetCharacterGlyphs();
    uint32_t i;

    for (i = aStart; i < aEnd; ++i) {
        if (!charGlyphs[i].IsLigatureGroupStart()) {
            NS_ASSERTION(i == aStart || aSpacing[i - aStart].mBefore == 0,
                         "Before-spacing inside a ligature!");
            NS_ASSERTION(i - 1 <= aStart || aSpacing[i - 1 - aStart].mAfter == 0,
                         "After-spacing inside a ligature!");
        }
    }
#endif
}

bool
gfxTextRun::GetAdjustedSpacingArray(uint32_t aStart, uint32_t aEnd,
                                    PropertyProvider *aProvider,
                                    uint32_t aSpacingStart, uint32_t aSpacingEnd,
                                    nsTArray<PropertyProvider::Spacing> *aSpacing)
{
    if (!aProvider || !(mFlags & gfxTextRunFactory::TEXT_ENABLE_SPACING))
        return false;
    if (!aSpacing->AppendElements(aEnd - aStart))
        return false;
    memset(aSpacing->Elements(), 0, sizeof(gfxFont::Spacing)*(aSpacingStart - aStart));
    GetAdjustedSpacing(this, aSpacingStart, aSpacingEnd, aProvider,
                       aSpacing->Elements() + aSpacingStart - aStart);
    memset(aSpacing->Elements() + aSpacingEnd - aStart, 0, sizeof(gfxFont::Spacing)*(aEnd - aSpacingEnd));
    return true;
}

void
gfxTextRun::ShrinkToLigatureBoundaries(uint32_t *aStart, uint32_t *aEnd)
{
    if (*aStart >= *aEnd)
        return;
  
    CompressedGlyph *charGlyphs = mCharacterGlyphs;

    while (*aStart < *aEnd && !charGlyphs[*aStart].IsLigatureGroupStart()) {
        ++(*aStart);
    }
    if (*aEnd < GetLength()) {
        while (*aEnd > *aStart && !charGlyphs[*aEnd].IsLigatureGroupStart()) {
            --(*aEnd);
        }
    }
}

void
gfxTextRun::DrawGlyphs(gfxFont *aFont, uint32_t aStart, uint32_t aEnd,
                       gfxPoint *aPt, PropertyProvider *aProvider,
                       uint32_t aSpacingStart, uint32_t aSpacingEnd,
                       TextRunDrawParams& aParams, uint16_t aOrientation)
{
    nsAutoTArray<PropertyProvider::Spacing,200> spacingBuffer;
    bool haveSpacing = GetAdjustedSpacingArray(aStart, aEnd, aProvider,
        aSpacingStart, aSpacingEnd, &spacingBuffer);
    aParams.spacing = haveSpacing ? spacingBuffer.Elements() : nullptr;
    aFont->Draw(this, aStart, aEnd, aPt, aParams, aOrientation);
}

static void
ClipPartialLigature(const gfxTextRun* aTextRun,
                    gfxFloat *aStart, gfxFloat *aEnd,
                    gfxFloat aOrigin,
                    gfxTextRun::LigatureData *aLigature)
{
    if (aLigature->mClipBeforePart) {
        if (aTextRun->IsRightToLeft()) {
            *aEnd = std::min(*aEnd, aOrigin);
        } else {
            *aStart = std::max(*aStart, aOrigin);
        }
    }
    if (aLigature->mClipAfterPart) {
        gfxFloat endEdge =
            aOrigin + aTextRun->GetDirection() * aLigature->mPartWidth;
        if (aTextRun->IsRightToLeft()) {
            *aStart = std::max(*aStart, endEdge);
        } else {
            *aEnd = std::min(*aEnd, endEdge);
        }
    }    
}

void
gfxTextRun::DrawPartialLigature(gfxFont *aFont, uint32_t aStart, uint32_t aEnd,
                                gfxPoint *aPt, PropertyProvider *aProvider,
                                TextRunDrawParams& aParams, uint16_t aOrientation)
{
    if (aStart >= aEnd) {
        return;
    }

    
    LigatureData data = ComputeLigatureData(aStart, aEnd, aProvider);
    gfxRect clipExtents = aParams.context->GetClipExtents();
    gfxFloat start, end;
    if (aParams.isVerticalRun) {
        start = clipExtents.Y() * mAppUnitsPerDevUnit;
        end = clipExtents.YMost() * mAppUnitsPerDevUnit;
        ClipPartialLigature(this, &start, &end, aPt->y, &data);
    } else {
        start = clipExtents.X() * mAppUnitsPerDevUnit;
        end = clipExtents.XMost() * mAppUnitsPerDevUnit;
        ClipPartialLigature(this, &start, &end, aPt->x, &data);
    }

    {
      
      
      
      Rect clipRect = aParams.isVerticalRun ?
          Rect(clipExtents.X(), start / mAppUnitsPerDevUnit,
               clipExtents.Width(), (end - start) / mAppUnitsPerDevUnit) :
          Rect(start / mAppUnitsPerDevUnit, clipExtents.Y(),
               (end - start) / mAppUnitsPerDevUnit, clipExtents.Height());
      MaybeSnapToDevicePixels(clipRect, *aParams.dt, true);

      aParams.context->Save();
      aParams.context->Clip(clipRect);
    }

    gfxPoint pt;
    if (aParams.isVerticalRun) {
        pt = gfxPoint(aPt->x, aPt->y - aParams.direction * data.mPartAdvance);
    } else {
        pt = gfxPoint(aPt->x - aParams.direction * data.mPartAdvance, aPt->y);
    }

    DrawGlyphs(aFont, data.mLigatureStart, data.mLigatureEnd, &pt,
               aProvider, aStart, aEnd, aParams, aOrientation);
    aParams.context->Restore();

    if (aParams.isVerticalRun) {
        aPt->y += aParams.direction * data.mPartWidth;
    } else {
        aPt->x += aParams.direction * data.mPartWidth;
    }
}


static bool
HasSyntheticBold(gfxTextRun *aRun, uint32_t aStart, uint32_t aLength)
{
    gfxTextRun::GlyphRunIterator iter(aRun, aStart, aLength);
    while (iter.NextRun()) {
        gfxFont *font = iter.GetGlyphRun()->mFont;
        if (font && font->IsSyntheticBold()) {
            return true;
        }
    }

    return false;
}



static bool
HasNonOpaqueColor(gfxContext *aContext, gfxRGBA& aCurrentColor)
{
    if (aContext->GetDeviceColor(aCurrentColor)) {
        if (aCurrentColor.a < 1.0 && aCurrentColor.a > 0.0) {
            return true;
        }
    }
        
    return false;
}


struct BufferAlphaColor {
    explicit BufferAlphaColor(gfxContext *aContext)
        : mContext(aContext)
        , mAlpha(0.0)
    {

    }

    ~BufferAlphaColor() {}

    void PushSolidColor(const gfxRect& aBounds, const gfxRGBA& aAlphaColor, uint32_t appsPerDevUnit)
    {
        mContext->Save();
        mContext->NewPath();
        mContext->Rectangle(gfxRect(aBounds.X() / appsPerDevUnit,
                    aBounds.Y() / appsPerDevUnit,
                    aBounds.Width() / appsPerDevUnit,
                    aBounds.Height() / appsPerDevUnit), true);
        mContext->Clip();
        mContext->SetColor(gfxRGBA(aAlphaColor.r, aAlphaColor.g, aAlphaColor.b));
        mContext->PushGroup(gfxContentType::COLOR_ALPHA);
        mAlpha = aAlphaColor.a;
    }

    void PopAlpha()
    {
        
        mContext->PopGroupToSource();
        mContext->SetOperator(gfxContext::OPERATOR_OVER);
        mContext->Paint(mAlpha);
        mContext->Restore();
    }

    gfxContext *mContext;
    gfxFloat mAlpha;
};

void
gfxTextRun::Draw(gfxContext *aContext, gfxPoint aPt, DrawMode aDrawMode,
                 uint32_t aStart, uint32_t aLength,
                 PropertyProvider *aProvider, gfxFloat *aAdvanceWidth,
                 gfxTextContextPaint *aContextPaint,
                 gfxTextRunDrawCallbacks *aCallbacks)
{
    NS_ASSERTION(aStart + aLength <= GetLength(), "Substring out of range");
    NS_ASSERTION(aDrawMode == DrawMode::GLYPH_PATH ||
                 !(int(aDrawMode) & int(DrawMode::GLYPH_PATH)),
                 "GLYPH_PATH cannot be used with GLYPH_FILL, GLYPH_STROKE or GLYPH_STROKE_UNDERNEATH");
    NS_ASSERTION(aDrawMode == DrawMode::GLYPH_PATH || !aCallbacks,
                 "callback must not be specified unless using GLYPH_PATH");

    bool skipDrawing = mSkipDrawing;
    if (aDrawMode == DrawMode::GLYPH_FILL) {
        gfxRGBA currentColor;
        if (aContext->GetDeviceColor(currentColor) && currentColor.a == 0) {
            skipDrawing = true;
        }
    }

    gfxFloat direction = GetDirection();

    if (skipDrawing) {
        
        
        if (aAdvanceWidth) {
            gfxTextRun::Metrics metrics = MeasureText(aStart, aLength,
                                                      gfxFont::LOOSE_INK_EXTENTS,
                                                      aContext, aProvider);
            *aAdvanceWidth = metrics.mAdvanceWidth * direction;
        }

        
        return;
    }

    
    
    TextRunDrawParams params;
    params.context = aContext;
    params.devPerApp = 1.0 / double(GetAppUnitsPerDevUnit());
    params.isVerticalRun = IsVertical();
    params.isRTL = IsRightToLeft();
    params.direction = direction;
    params.drawMode = aDrawMode;
    params.callbacks = aCallbacks;
    params.runContextPaint = aContextPaint;
    params.paintSVGGlyphs = !aCallbacks || aCallbacks->mShouldPaintSVGGlyphs;
    params.dt = aContext->GetDrawTarget();
    params.fontSmoothingBGColor = aContext->GetFontSmoothingBackgroundColor();

    
    
    BufferAlphaColor syntheticBoldBuffer(aContext);
    gfxRGBA currentColor;
    bool needToRestore = false;

    if (aDrawMode == DrawMode::GLYPH_FILL &&
        HasNonOpaqueColor(aContext, currentColor) &&
        HasSyntheticBold(this, aStart, aLength)) {
        needToRestore = true;
        
        gfxTextRun::Metrics metrics = MeasureText(aStart, aLength,
                                                  gfxFont::LOOSE_INK_EXTENTS,
                                                  aContext, aProvider);
        metrics.mBoundingBox.MoveBy(aPt);
        syntheticBoldBuffer.PushSolidColor(metrics.mBoundingBox, currentColor,
                                           GetAppUnitsPerDevUnit());
    }

    GlyphRunIterator iter(this, aStart, aLength);
    gfxFloat advance = 0.0;

    while (iter.NextRun()) {
        gfxFont *font = iter.GetGlyphRun()->mFont;
        uint32_t start = iter.GetStringStart();
        uint32_t end = iter.GetStringEnd();
        uint32_t ligatureRunStart = start;
        uint32_t ligatureRunEnd = end;
        ShrinkToLigatureBoundaries(&ligatureRunStart, &ligatureRunEnd);

        bool drawPartial = aDrawMode == DrawMode::GLYPH_FILL ||
                           (aDrawMode == DrawMode::GLYPH_PATH && aCallbacks);
        gfxPoint origPt = aPt;

        if (drawPartial) {
            DrawPartialLigature(font, start, ligatureRunStart, &aPt,
                                aProvider, params,
                                iter.GetGlyphRun()->mOrientation);
        }

        DrawGlyphs(font, ligatureRunStart, ligatureRunEnd, &aPt,
                   aProvider, ligatureRunStart, ligatureRunEnd, params,
                   iter.GetGlyphRun()->mOrientation);

        if (drawPartial) {
            DrawPartialLigature(font, ligatureRunEnd, end, &aPt,
                                aProvider, params,
                                iter.GetGlyphRun()->mOrientation);
        }

        if (params.isVerticalRun) {
            advance += (aPt.y - origPt.y) * params.direction;
        } else {
            advance += (aPt.x - origPt.x) * params.direction;
        }
    }

    
    if (needToRestore) {
        syntheticBoldBuffer.PopAlpha();
    }

    if (aAdvanceWidth) {
        *aAdvanceWidth = advance;
    }
}

void
gfxTextRun::AccumulateMetricsForRun(gfxFont *aFont,
                                    uint32_t aStart, uint32_t aEnd,
                                    gfxFont::BoundingBoxType aBoundingBoxType,
                                    gfxContext *aRefContext,
                                    PropertyProvider *aProvider,
                                    uint32_t aSpacingStart, uint32_t aSpacingEnd,
                                    uint16_t aOrientation,
                                    Metrics *aMetrics)
{
    nsAutoTArray<PropertyProvider::Spacing,200> spacingBuffer;
    bool haveSpacing = GetAdjustedSpacingArray(aStart, aEnd, aProvider,
        aSpacingStart, aSpacingEnd, &spacingBuffer);
    Metrics metrics = aFont->Measure(this, aStart, aEnd, aBoundingBoxType, aRefContext,
                                     haveSpacing ? spacingBuffer.Elements() : nullptr,
                                     aOrientation);
    aMetrics->CombineWith(metrics, IsRightToLeft());
}

void
gfxTextRun::AccumulatePartialLigatureMetrics(gfxFont *aFont,
    uint32_t aStart, uint32_t aEnd,
    gfxFont::BoundingBoxType aBoundingBoxType, gfxContext *aRefContext,
    PropertyProvider *aProvider, uint16_t aOrientation, Metrics *aMetrics)
{
    if (aStart >= aEnd)
        return;

    
    
    LigatureData data = ComputeLigatureData(aStart, aEnd, aProvider);

    
    Metrics metrics;
    AccumulateMetricsForRun(aFont, data.mLigatureStart, data.mLigatureEnd,
                            aBoundingBoxType, aRefContext,
                            aProvider, aStart, aEnd, aOrientation, &metrics);

    
    gfxFloat bboxLeft = metrics.mBoundingBox.X();
    gfxFloat bboxRight = metrics.mBoundingBox.XMost();
    
    gfxFloat origin = IsRightToLeft() ? metrics.mAdvanceWidth - data.mPartAdvance : 0;
    ClipPartialLigature(this, &bboxLeft, &bboxRight, origin, &data);
    metrics.mBoundingBox.x = bboxLeft;
    metrics.mBoundingBox.width = bboxRight - bboxLeft;

    
    
    metrics.mBoundingBox.x -=
        IsRightToLeft() ? metrics.mAdvanceWidth - (data.mPartAdvance + data.mPartWidth)
            : data.mPartAdvance;    
    metrics.mAdvanceWidth = data.mPartWidth;

    aMetrics->CombineWith(metrics, IsRightToLeft());
}

gfxTextRun::Metrics
gfxTextRun::MeasureText(uint32_t aStart, uint32_t aLength,
                        gfxFont::BoundingBoxType aBoundingBoxType,
                        gfxContext *aRefContext,
                        PropertyProvider *aProvider)
{
    NS_ASSERTION(aStart + aLength <= GetLength(), "Substring out of range");

    Metrics accumulatedMetrics;
    GlyphRunIterator iter(this, aStart, aLength);
    while (iter.NextRun()) {
        gfxFont *font = iter.GetGlyphRun()->mFont;
        uint32_t start = iter.GetStringStart();
        uint32_t end = iter.GetStringEnd();
        uint32_t ligatureRunStart = start;
        uint32_t ligatureRunEnd = end;
        ShrinkToLigatureBoundaries(&ligatureRunStart, &ligatureRunEnd);

        AccumulatePartialLigatureMetrics(font, start, ligatureRunStart,
            aBoundingBoxType, aRefContext, aProvider,
            iter.GetGlyphRun()->mOrientation, &accumulatedMetrics);

        
        
        
        
        
        AccumulateMetricsForRun(font,
            ligatureRunStart, ligatureRunEnd, aBoundingBoxType,
            aRefContext, aProvider, ligatureRunStart, ligatureRunEnd,
            iter.GetGlyphRun()->mOrientation, &accumulatedMetrics);

        AccumulatePartialLigatureMetrics(font, ligatureRunEnd, end,
            aBoundingBoxType, aRefContext, aProvider,
            iter.GetGlyphRun()->mOrientation, &accumulatedMetrics);
    }

    return accumulatedMetrics;
}

#define MEASUREMENT_BUFFER_SIZE 100

uint32_t
gfxTextRun::BreakAndMeasureText(uint32_t aStart, uint32_t aMaxLength,
                                bool aLineBreakBefore, gfxFloat aWidth,
                                PropertyProvider *aProvider,
                                SuppressBreak aSuppressBreak,
                                gfxFloat *aTrimWhitespace,
                                Metrics *aMetrics,
                                gfxFont::BoundingBoxType aBoundingBoxType,
                                gfxContext *aRefContext,
                                bool *aUsedHyphenation,
                                uint32_t *aLastBreak,
                                bool aCanWordWrap,
                                gfxBreakPriority *aBreakPriority)
{
    aMaxLength = std::min(aMaxLength, GetLength() - aStart);

    NS_ASSERTION(aStart + aMaxLength <= GetLength(), "Substring out of range");

    uint32_t bufferStart = aStart;
    uint32_t bufferLength = std::min<uint32_t>(aMaxLength, MEASUREMENT_BUFFER_SIZE);
    PropertyProvider::Spacing spacingBuffer[MEASUREMENT_BUFFER_SIZE];
    bool haveSpacing = aProvider && (mFlags & gfxTextRunFactory::TEXT_ENABLE_SPACING) != 0;
    if (haveSpacing) {
        GetAdjustedSpacing(this, bufferStart, bufferStart + bufferLength, aProvider,
                           spacingBuffer);
    }
    bool hyphenBuffer[MEASUREMENT_BUFFER_SIZE];
    bool haveHyphenation = aProvider &&
        (aProvider->GetHyphensOption() == NS_STYLE_HYPHENS_AUTO ||
         (aProvider->GetHyphensOption() == NS_STYLE_HYPHENS_MANUAL &&
          (mFlags & gfxTextRunFactory::TEXT_ENABLE_HYPHEN_BREAKS) != 0));
    if (haveHyphenation) {
        aProvider->GetHyphenationBreaks(bufferStart, bufferLength,
                                        hyphenBuffer);
    }

    gfxFloat width = 0;
    gfxFloat advance = 0;
    
    uint32_t trimmableChars = 0;
    
    gfxFloat trimmableAdvance = 0;
    int32_t lastBreak = -1;
    int32_t lastBreakTrimmableChars = -1;
    gfxFloat lastBreakTrimmableAdvance = -1;
    bool aborted = false;
    uint32_t end = aStart + aMaxLength;
    bool lastBreakUsedHyphenation = false;

    uint32_t ligatureRunStart = aStart;
    uint32_t ligatureRunEnd = end;
    ShrinkToLigatureBoundaries(&ligatureRunStart, &ligatureRunEnd);

    uint32_t i;
    for (i = aStart; i < end; ++i) {
        if (i >= bufferStart + bufferLength) {
            
            bufferStart = i;
            bufferLength = std::min(aStart + aMaxLength, i + MEASUREMENT_BUFFER_SIZE) - i;
            if (haveSpacing) {
                GetAdjustedSpacing(this, bufferStart, bufferStart + bufferLength, aProvider,
                                   spacingBuffer);
            }
            if (haveHyphenation) {
                aProvider->GetHyphenationBreaks(bufferStart, bufferLength,
                                                hyphenBuffer);
            }
        }

        
        
        
        
        if (aSuppressBreak != eSuppressAllBreaks &&
            (aSuppressBreak != eSuppressInitialBreak || i > aStart)) {
            bool atNaturalBreak = mCharacterGlyphs[i].CanBreakBefore() == 1;
            bool atHyphenationBreak =
                !atNaturalBreak && haveHyphenation && hyphenBuffer[i - bufferStart];
            bool atBreak = atNaturalBreak || atHyphenationBreak;
            bool wordWrapping =
                aCanWordWrap && mCharacterGlyphs[i].IsClusterStart() &&
                *aBreakPriority <= gfxBreakPriority::eWordWrapBreak;

            if (atBreak || wordWrapping) {
                gfxFloat hyphenatedAdvance = advance;
                if (atHyphenationBreak) {
                    hyphenatedAdvance += aProvider->GetHyphenWidth();
                }
            
                if (lastBreak < 0 || width + hyphenatedAdvance - trimmableAdvance <= aWidth) {
                    
                    lastBreak = i;
                    lastBreakTrimmableChars = trimmableChars;
                    lastBreakTrimmableAdvance = trimmableAdvance;
                    lastBreakUsedHyphenation = atHyphenationBreak;
                    *aBreakPriority = atBreak ? gfxBreakPriority::eNormalBreak
                                              : gfxBreakPriority::eWordWrapBreak;
                }

                width += advance;
                advance = 0;
                if (width - trimmableAdvance > aWidth) {
                    
                    aborted = true;
                    break;
                }
            }
        }
        
        gfxFloat charAdvance;
        if (i >= ligatureRunStart && i < ligatureRunEnd) {
            charAdvance = GetAdvanceForGlyphs(i, i + 1);
            if (haveSpacing) {
                PropertyProvider::Spacing *space = &spacingBuffer[i - bufferStart];
                charAdvance += space->mBefore + space->mAfter;
            }
        } else {
            charAdvance = ComputePartialLigatureWidth(i, i + 1, aProvider);
        }
        
        advance += charAdvance;
        if (aTrimWhitespace) {
            if (mCharacterGlyphs[i].CharIsSpace()) {
                ++trimmableChars;
                trimmableAdvance += charAdvance;
            } else {
                trimmableAdvance = 0;
                trimmableChars = 0;
            }
        }
    }

    if (!aborted) {
        width += advance;
    }

    
    
    
    
    uint32_t charsFit;
    bool usedHyphenation = false;
    if (width - trimmableAdvance <= aWidth) {
        charsFit = aMaxLength;
    } else if (lastBreak >= 0) {
        charsFit = lastBreak - aStart;
        trimmableChars = lastBreakTrimmableChars;
        trimmableAdvance = lastBreakTrimmableAdvance;
        usedHyphenation = lastBreakUsedHyphenation;
    } else {
        charsFit = aMaxLength;
    }

    if (aMetrics) {
        *aMetrics = MeasureText(aStart, charsFit,
            aBoundingBoxType, aRefContext, aProvider);
        if (trimmableChars) {
            Metrics trimMetrics =
                MeasureText(aStart + charsFit - trimmableChars,
                            trimmableChars, aBoundingBoxType,
                            aRefContext, aProvider);
            aMetrics->mAdvanceWidth -= trimMetrics.mAdvanceWidth;
        }
    }
    if (aTrimWhitespace) {
        *aTrimWhitespace = trimmableAdvance;
    }
    if (aUsedHyphenation) {
        *aUsedHyphenation = usedHyphenation;
    }
    if (aLastBreak && charsFit == aMaxLength) {
        if (lastBreak < 0) {
            *aLastBreak = UINT32_MAX;
        } else {
            *aLastBreak = lastBreak - aStart;
        }
    }

    return charsFit;
}

gfxFloat
gfxTextRun::GetAdvanceWidth(uint32_t aStart, uint32_t aLength,
                            PropertyProvider *aProvider,
                            PropertyProvider::Spacing* aSpacing)
{
    NS_ASSERTION(aStart + aLength <= GetLength(), "Substring out of range");

    uint32_t ligatureRunStart = aStart;
    uint32_t ligatureRunEnd = aStart + aLength;
    ShrinkToLigatureBoundaries(&ligatureRunStart, &ligatureRunEnd);

    gfxFloat result = ComputePartialLigatureWidth(aStart, ligatureRunStart, aProvider) +
                      ComputePartialLigatureWidth(ligatureRunEnd, aStart + aLength, aProvider);

    if (aSpacing) {
        aSpacing->mBefore = aSpacing->mAfter = 0;
    }

    
    
    if (aProvider && (mFlags & gfxTextRunFactory::TEXT_ENABLE_SPACING)) {
        uint32_t i;
        nsAutoTArray<PropertyProvider::Spacing,200> spacingBuffer;
        if (spacingBuffer.AppendElements(aLength)) {
            GetAdjustedSpacing(this, ligatureRunStart, ligatureRunEnd, aProvider,
                               spacingBuffer.Elements());
            for (i = 0; i < ligatureRunEnd - ligatureRunStart; ++i) {
                PropertyProvider::Spacing *space = &spacingBuffer[i];
                result += space->mBefore + space->mAfter;
            }
            if (aSpacing) {
                aSpacing->mBefore = spacingBuffer[0].mBefore;
                aSpacing->mAfter = spacingBuffer.LastElement().mAfter;
            }
        }
    }

    return result + GetAdvanceForGlyphs(ligatureRunStart, ligatureRunEnd);
}

bool
gfxTextRun::SetLineBreaks(uint32_t aStart, uint32_t aLength,
                          bool aLineBreakBefore, bool aLineBreakAfter,
                          gfxFloat *aAdvanceWidthDelta,
                          gfxContext *aRefContext)
{
    
    
    if (aAdvanceWidthDelta) {
        *aAdvanceWidthDelta = 0;
    }
    return false;
}

uint32_t
gfxTextRun::FindFirstGlyphRunContaining(uint32_t aOffset)
{
    NS_ASSERTION(aOffset <= GetLength(), "Bad offset looking for glyphrun");
    NS_ASSERTION(GetLength() == 0 || mGlyphRuns.Length() > 0,
                 "non-empty text but no glyph runs present!");
    if (aOffset == GetLength())
        return mGlyphRuns.Length();
    uint32_t start = 0;
    uint32_t end = mGlyphRuns.Length();
    while (end - start > 1) {
        uint32_t mid = (start + end)/2;
        if (mGlyphRuns[mid].mCharacterOffset <= aOffset) {
            start = mid;
        } else {
            end = mid;
        }
    }
    NS_ASSERTION(mGlyphRuns[start].mCharacterOffset <= aOffset,
                 "Hmm, something went wrong, aOffset should have been found");
    return start;
}

nsresult
gfxTextRun::AddGlyphRun(gfxFont *aFont, uint8_t aMatchType,
                        uint32_t aUTF16Offset, bool aForceNewRun,
                        uint16_t aOrientation)
{
    NS_ASSERTION(aFont, "adding glyph run for null font!");
    NS_ASSERTION(aOrientation != gfxTextRunFactory::TEXT_ORIENT_VERTICAL_MIXED,
                 "mixed orientation should have been resolved");
    if (!aFont) {
        return NS_OK;
    }    
    uint32_t numGlyphRuns = mGlyphRuns.Length();
    if (!aForceNewRun && numGlyphRuns > 0) {
        GlyphRun *lastGlyphRun = &mGlyphRuns[numGlyphRuns - 1];

        NS_ASSERTION(lastGlyphRun->mCharacterOffset <= aUTF16Offset,
                     "Glyph runs out of order (and run not forced)");

        
        if (lastGlyphRun->mFont == aFont &&
            lastGlyphRun->mMatchType == aMatchType &&
            lastGlyphRun->mOrientation == aOrientation)
        {
            return NS_OK;
        }

        
        
        if (lastGlyphRun->mCharacterOffset == aUTF16Offset) {

            
            
            
            if (numGlyphRuns > 1 &&
                mGlyphRuns[numGlyphRuns - 2].mFont == aFont &&
                mGlyphRuns[numGlyphRuns - 2].mMatchType == aMatchType &&
                mGlyphRuns[numGlyphRuns - 2].mOrientation == aOrientation)
            {
                mGlyphRuns.TruncateLength(numGlyphRuns - 1);
                return NS_OK;
            }

            lastGlyphRun->mFont = aFont;
            lastGlyphRun->mMatchType = aMatchType;
            lastGlyphRun->mOrientation = aOrientation;
            return NS_OK;
        }
    }

    NS_ASSERTION(aForceNewRun || numGlyphRuns > 0 || aUTF16Offset == 0,
                 "First run doesn't cover the first character (and run not forced)?");

    GlyphRun *glyphRun = mGlyphRuns.AppendElement();
    if (!glyphRun)
        return NS_ERROR_OUT_OF_MEMORY;
    glyphRun->mFont = aFont;
    glyphRun->mCharacterOffset = aUTF16Offset;
    glyphRun->mMatchType = aMatchType;
    glyphRun->mOrientation = aOrientation;
    return NS_OK;
}

void
gfxTextRun::SortGlyphRuns()
{
    if (mGlyphRuns.Length() <= 1)
        return;

    nsTArray<GlyphRun> runs(mGlyphRuns);
    GlyphRunOffsetComparator comp;
    runs.Sort(comp);

    
    mGlyphRuns.Clear();
    uint32_t i, count = runs.Length();
    for (i = 0; i < count; ++i) {
        
        
        if (i == 0 || runs[i].mFont != runs[i - 1].mFont ||
            runs[i].mOrientation != runs[i - 1].mOrientation) {
            mGlyphRuns.AppendElement(runs[i]);
            
            
            NS_ASSERTION(i == 0 ||
                         runs[i].mCharacterOffset !=
                         runs[i - 1].mCharacterOffset,
                         "Two fonts for the same run, glyph indices may not match the font");
        }
    }
}




void
gfxTextRun::SanitizeGlyphRuns()
{
    if (mGlyphRuns.Length() <= 1)
        return;

    
    
    
    
    int32_t i, lastRunIndex = mGlyphRuns.Length() - 1;
    const CompressedGlyph *charGlyphs = mCharacterGlyphs;
    for (i = lastRunIndex; i >= 0; --i) {
        GlyphRun& run = mGlyphRuns[i];
        while (charGlyphs[run.mCharacterOffset].IsLigatureContinuation() &&
               run.mCharacterOffset < GetLength()) {
            run.mCharacterOffset++;
        }
        
        if ((i < lastRunIndex &&
             run.mCharacterOffset >= mGlyphRuns[i+1].mCharacterOffset) ||
            (i == lastRunIndex && run.mCharacterOffset == GetLength())) {
            mGlyphRuns.RemoveElementAt(i);
            --lastRunIndex;
        }
    }
}

uint32_t
gfxTextRun::CountMissingGlyphs()
{
    uint32_t i;
    uint32_t count = 0;
    for (i = 0; i < GetLength(); ++i) {
        if (mCharacterGlyphs[i].IsMissing()) {
            ++count;
        }
    }
    return count;
}

void
gfxTextRun::CopyGlyphDataFrom(gfxShapedWord *aShapedWord, uint32_t aOffset)
{
    uint32_t wordLen = aShapedWord->GetLength();
    NS_ASSERTION(aOffset + wordLen <= GetLength(),
                 "word overruns end of textrun!");

    CompressedGlyph *charGlyphs = GetCharacterGlyphs();
    const CompressedGlyph *wordGlyphs = aShapedWord->GetCharacterGlyphs();
    if (aShapedWord->HasDetailedGlyphs()) {
        for (uint32_t i = 0; i < wordLen; ++i, ++aOffset) {
            const CompressedGlyph& g = wordGlyphs[i];
            if (g.IsSimpleGlyph()) {
                charGlyphs[aOffset] = g;
            } else {
                const DetailedGlyph *details =
                    g.GetGlyphCount() > 0 ?
                        aShapedWord->GetDetailedGlyphs(i) : nullptr;
                SetGlyphs(aOffset, g, details);
            }
        }
    } else {
        memcpy(charGlyphs + aOffset, wordGlyphs,
               wordLen * sizeof(CompressedGlyph));
    }
}

void
gfxTextRun::CopyGlyphDataFrom(gfxTextRun *aSource, uint32_t aStart,
                              uint32_t aLength, uint32_t aDest)
{
    NS_ASSERTION(aStart + aLength <= aSource->GetLength(),
                 "Source substring out of range");
    NS_ASSERTION(aDest + aLength <= GetLength(),
                 "Destination substring out of range");

    if (aSource->mSkipDrawing) {
        mSkipDrawing = true;
    }

    
    const CompressedGlyph *srcGlyphs = aSource->mCharacterGlyphs + aStart;
    CompressedGlyph *dstGlyphs = mCharacterGlyphs + aDest;
    for (uint32_t i = 0; i < aLength; ++i) {
        CompressedGlyph g = srcGlyphs[i];
        g.SetCanBreakBefore(!g.IsClusterStart() ?
            CompressedGlyph::FLAG_BREAK_TYPE_NONE :
            dstGlyphs[i].CanBreakBefore());
        if (!g.IsSimpleGlyph()) {
            uint32_t count = g.GetGlyphCount();
            if (count > 0) {
                DetailedGlyph *dst = AllocateDetailedGlyphs(i + aDest, count);
                if (dst) {
                    DetailedGlyph *src = aSource->GetDetailedGlyphs(i + aStart);
                    if (src) {
                        ::memcpy(dst, src, count * sizeof(DetailedGlyph));
                    } else {
                        g.SetMissing(0);
                    }
                } else {
                    g.SetMissing(0);
                }
            }
        }
        dstGlyphs[i] = g;
    }

    
    GlyphRunIterator iter(aSource, aStart, aLength);
#ifdef DEBUG
    GlyphRun *prevRun = nullptr;
#endif
    while (iter.NextRun()) {
        gfxFont *font = iter.GetGlyphRun()->mFont;
        NS_ASSERTION(!prevRun || prevRun->mFont != iter.GetGlyphRun()->mFont ||
                     prevRun->mMatchType != iter.GetGlyphRun()->mMatchType ||
                     prevRun->mOrientation != iter.GetGlyphRun()->mOrientation,
                     "Glyphruns not coalesced?");
#ifdef DEBUG
        prevRun = iter.GetGlyphRun();
        uint32_t end = iter.GetStringEnd();
#endif
        uint32_t start = iter.GetStringStart();

        
        
        
        
        
        
        
        
        
        NS_WARN_IF_FALSE(aSource->IsClusterStart(start),
                         "Started font run in the middle of a cluster");
        NS_WARN_IF_FALSE(end == aSource->GetLength() || aSource->IsClusterStart(end),
                         "Ended font run in the middle of a cluster");

        nsresult rv = AddGlyphRun(font, iter.GetGlyphRun()->mMatchType,
                                  start - aStart + aDest, false,
                                  iter.GetGlyphRun()->mOrientation);
        if (NS_FAILED(rv))
            return;
    }
}

void
gfxTextRun::ClearGlyphsAndCharacters()
{
    ResetGlyphRuns();
    memset(reinterpret_cast<char*>(mCharacterGlyphs), 0,
           mLength * sizeof(CompressedGlyph));
    mDetailedGlyphs = nullptr;
}

void
gfxTextRun::SetSpaceGlyph(gfxFont *aFont, gfxContext *aContext,
                          uint32_t aCharIndex, uint16_t aOrientation)
{
    if (SetSpaceGlyphIfSimple(aFont, aContext, aCharIndex, ' ',
                              aOrientation)) {
        return;
    }

    aFont->InitWordCache();
    static const uint8_t space = ' ';
    uint32_t flags = gfxTextRunFactory::TEXT_IS_8BIT |
                     gfxTextRunFactory::TEXT_IS_ASCII |
                     gfxTextRunFactory::TEXT_IS_PERSISTENT |
                     aOrientation;
    bool vertical =
        (GetFlags() & gfxTextRunFactory::TEXT_ORIENT_VERTICAL_UPRIGHT) != 0;
    gfxShapedWord *sw = aFont->GetShapedWord(aContext,
                                             &space, 1,
                                             gfxShapedWord::HashMix(0, ' '), 
                                             MOZ_SCRIPT_LATIN,
                                             vertical,
                                             mAppUnitsPerDevUnit,
                                             flags,
                                             nullptr);
    if (sw) {
        AddGlyphRun(aFont, gfxTextRange::kFontGroup, aCharIndex, false,
                    aOrientation);
        CopyGlyphDataFrom(sw, aCharIndex);
    }
}

bool
gfxTextRun::SetSpaceGlyphIfSimple(gfxFont *aFont, gfxContext *aContext,
                                  uint32_t aCharIndex, char16_t aSpaceChar,
                                  uint16_t aOrientation)
{
    uint32_t spaceGlyph = aFont->GetSpaceGlyph();
    if (!spaceGlyph || !CompressedGlyph::IsSimpleGlyphID(spaceGlyph)) {
        return false;
    }

    gfxFont::Orientation fontOrientation =
        (aOrientation & gfxTextRunFactory::TEXT_ORIENT_VERTICAL_UPRIGHT) ?
            gfxFont::eVertical : gfxFont::eHorizontal;
    uint32_t spaceWidthAppUnits =
        NS_lroundf(aFont->GetMetrics(fontOrientation).spaceWidth *
                   mAppUnitsPerDevUnit);
    if (!CompressedGlyph::IsSimpleAdvance(spaceWidthAppUnits)) {
        return false;
    }

    AddGlyphRun(aFont, gfxTextRange::kFontGroup, aCharIndex, false,
                aOrientation);
    CompressedGlyph g;
    g.SetSimpleGlyph(spaceWidthAppUnits, spaceGlyph);
    if (aSpaceChar == ' ') {
        g.SetIsSpace();
    }
    GetCharacterGlyphs()[aCharIndex] = g;
    return true;
}

void
gfxTextRun::FetchGlyphExtents(gfxContext *aRefContext)
{
    bool needsGlyphExtents = NeedsGlyphExtents(this);
    if (!needsGlyphExtents && !mDetailedGlyphs)
        return;

    uint32_t i, runCount = mGlyphRuns.Length();
    CompressedGlyph *charGlyphs = mCharacterGlyphs;
    for (i = 0; i < runCount; ++i) {
        const GlyphRun& run = mGlyphRuns[i];
        gfxFont *font = run.mFont;
        if (MOZ_UNLIKELY(font->GetStyle()->size == 0) ||
            MOZ_UNLIKELY(font->GetStyle()->sizeAdjust == 0.0f)) {
            continue;
        }

        uint32_t start = run.mCharacterOffset;
        uint32_t end = i + 1 < runCount ?
            mGlyphRuns[i + 1].mCharacterOffset : GetLength();
        bool fontIsSetup = false;
        uint32_t j;
        gfxGlyphExtents *extents = font->GetOrCreateGlyphExtents(mAppUnitsPerDevUnit);
  
        for (j = start; j < end; ++j) {
            const gfxTextRun::CompressedGlyph *glyphData = &charGlyphs[j];
            gfxFont::Orientation orientation =
                IsVertical() ? gfxFont::eVertical : gfxFont::eHorizontal;
            if (glyphData->IsSimpleGlyph()) {
                
                
                if (needsGlyphExtents) {
                    uint32_t glyphIndex = glyphData->GetSimpleGlyph();
                    if (!extents->IsGlyphKnown(glyphIndex)) {
                        if (!fontIsSetup) {
                            if (!font->SetupCairoFont(aRefContext)) {
                                NS_WARNING("failed to set up font for glyph extents");
                                break;
                            }
                            fontIsSetup = true;
                        }
#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
                        ++gGlyphExtentsSetupEagerSimple;
#endif
                        font->SetupGlyphExtents(aRefContext, orientation,
                                                glyphIndex, false, extents);
                    }
                }
            } else if (!glyphData->IsMissing()) {
                uint32_t glyphCount = glyphData->GetGlyphCount();
                if (glyphCount == 0) {
                    continue;
                }
                const gfxTextRun::DetailedGlyph *details = GetDetailedGlyphs(j);
                if (!details) {
                    continue;
                }
                for (uint32_t k = 0; k < glyphCount; ++k, ++details) {
                    uint32_t glyphIndex = details->mGlyphID;
                    if (!extents->IsGlyphKnownWithTightExtents(glyphIndex)) {
                        if (!fontIsSetup) {
                            if (!font->SetupCairoFont(aRefContext)) {
                                NS_WARNING("failed to set up font for glyph extents");
                                break;
                            }
                            fontIsSetup = true;
                        }
#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
                        ++gGlyphExtentsSetupEagerTight;
#endif
                        font->SetupGlyphExtents(aRefContext, orientation,
                                                glyphIndex, true, extents);
                    }
                }
            }
        }
    }
}


gfxTextRun::ClusterIterator::ClusterIterator(gfxTextRun *aTextRun)
    : mTextRun(aTextRun), mCurrentChar(uint32_t(-1))
{
}

void
gfxTextRun::ClusterIterator::Reset()
{
    mCurrentChar = uint32_t(-1);
}

bool
gfxTextRun::ClusterIterator::NextCluster()
{
    uint32_t len = mTextRun->GetLength();
    while (++mCurrentChar < len) {
        if (mTextRun->IsClusterStart(mCurrentChar)) {
            return true;
        }
    }

    mCurrentChar = uint32_t(-1);
    return false;
}

uint32_t
gfxTextRun::ClusterIterator::ClusterLength() const
{
    if (mCurrentChar == uint32_t(-1)) {
        return 0;
    }

    uint32_t i = mCurrentChar,
             len = mTextRun->GetLength();
    while (++i < len) {
        if (mTextRun->IsClusterStart(i)) {
            break;
        }
    }

    return i - mCurrentChar;
}

gfxFloat
gfxTextRun::ClusterIterator::ClusterAdvance(PropertyProvider *aProvider) const
{
    if (mCurrentChar == uint32_t(-1)) {
        return 0;
    }

    return mTextRun->GetAdvanceWidth(mCurrentChar, ClusterLength(), aProvider);
}

size_t
gfxTextRun::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf)
{
    
    
    size_t total = mGlyphRuns.SizeOfExcludingThis(aMallocSizeOf);

    if (mDetailedGlyphs) {
        total += mDetailedGlyphs->SizeOfIncludingThis(aMallocSizeOf);
    }

    return total;
}

size_t
gfxTextRun::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf)
{
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}


#ifdef DEBUG
void
gfxTextRun::Dump(FILE* aOutput) {
    if (!aOutput) {
        aOutput = stdout;
    }

    uint32_t i;
    fputc('[', aOutput);
    for (i = 0; i < mGlyphRuns.Length(); ++i) {
        if (i > 0) {
            fputc(',', aOutput);
        }
        gfxFont* font = mGlyphRuns[i].mFont;
        const gfxFontStyle* style = font->GetStyle();
        NS_ConvertUTF16toUTF8 fontName(font->GetName());
        nsAutoCString lang;
        style->language->ToUTF8String(lang);
        fprintf(aOutput, "%d: %s %f/%d/%d/%s", mGlyphRuns[i].mCharacterOffset,
                fontName.get(), style->size,
                style->weight, style->style, lang.get());
    }
    fputc(']', aOutput);
}
#endif

gfxFontGroup::gfxFontGroup(const FontFamilyList& aFontFamilyList,
                           const gfxFontStyle *aStyle,
                           gfxUserFontSet *aUserFontSet)
    : mFamilyList(aFontFamilyList)
    , mStyle(*aStyle)
    , mUnderlineOffset(UNDERLINE_OFFSET_NOT_SET)
    , mHyphenWidth(-1)
    , mUserFontSet(aUserFontSet)
    , mTextPerf(nullptr)
    , mPageLang(gfxPlatform::GetFontPrefLangFor(aStyle->language))
    , mSkipDrawing(false)
{
    
    
    mCurrGeneration = GetGeneration();
    BuildFontList();
}

gfxFontGroup::~gfxFontGroup()
{
}

void
gfxFontGroup::FindGenericFonts(FontFamilyType aGenericType,
                               nsIAtom *aLanguage,
                               void *aClosure)
{
    nsAutoTArray<nsString, 5> resolvedGenerics;
    ResolveGenericFontNames(aGenericType, aLanguage, resolvedGenerics);
    uint32_t g = 0, numGenerics = resolvedGenerics.Length();
    for (g = 0; g < numGenerics; g++) {
        FindPlatformFont(resolvedGenerics[g], false, aClosure);
    }
}

 void
gfxFontGroup::ResolveGenericFontNames(FontFamilyType aGenericType,
                                      nsIAtom *aLanguage,
                                      nsTArray<nsString>& aGenericFamilies)
{
    static const char kGeneric_serif[] = "serif";
    static const char kGeneric_sans_serif[] = "sans-serif";
    static const char kGeneric_monospace[] = "monospace";
    static const char kGeneric_cursive[] = "cursive";
    static const char kGeneric_fantasy[] = "fantasy";

    
    if (aGenericType == eFamily_moz_fixed) {
        aGenericType = eFamily_monospace;
    }

    
    NS_ASSERTION(aGenericType >= eFamily_serif &&
                 aGenericType <= eFamily_fantasy,
                 "standard generic font family type required");

    
    nsIAtom *langGroupAtom = nullptr;
    nsAutoCString langGroupString;
    if (aLanguage) {
        if (!gLangService) {
            CallGetService(NS_LANGUAGEATOMSERVICE_CONTRACTID, &gLangService);
        }
        if (gLangService) {
            nsresult rv;
            langGroupAtom = gLangService->GetLanguageGroup(aLanguage, &rv);
        }
    }
    if (!langGroupAtom) {
        langGroupAtom = nsGkAtoms::Unicode;
    }
    langGroupAtom->ToUTF8String(langGroupString);

    
    const char *generic = nullptr;
    switch (aGenericType) {
        case eFamily_serif:
            generic = kGeneric_serif;
            break;
        case eFamily_sans_serif:
            generic = kGeneric_sans_serif;
            break;
        case eFamily_monospace:
            generic = kGeneric_monospace;
            break;
        case eFamily_cursive:
            generic = kGeneric_cursive;
            break;
        case eFamily_fantasy:
            generic = kGeneric_fantasy;
            break;
        default:
            break;
    }

    if (!generic) {
        return;
    }

    aGenericFamilies.Clear();

    
    nsAutoCString prefFontName("font.name.");
    prefFontName.Append(generic);
    prefFontName.Append('.');
    prefFontName.Append(langGroupString);
    gfxFontUtils::AppendPrefsFontList(prefFontName.get(),
                                      aGenericFamilies);

    
    if (!aGenericFamilies.IsEmpty()) {
        nsAutoCString prefFontListName("font.name-list.");
        prefFontListName.Append(generic);
        prefFontListName.Append('.');
        prefFontListName.Append(langGroupString);
        gfxFontUtils::AppendPrefsFontList(prefFontListName.get(),
                                          aGenericFamilies);
    }

#if 0  
    printf("%s ===> ", prefFontName.get());
    for (uint32_t k = 0; k < aGenericFamilies.Length(); k++) {
        if (k > 0) printf(", ");
        printf("%s", NS_ConvertUTF16toUTF8(aGenericFamilies[k]).get());
    }
    printf("\n");
#endif
}

void gfxFontGroup::EnumerateFontList(nsIAtom *aLanguage, void *aClosure)
{
    
    const nsTArray<FontFamilyName>& fontlist = mFamilyList.GetFontlist();

    
    uint32_t i, numFonts = fontlist.Length();
    for (i = 0; i < numFonts; i++) {
        const FontFamilyName& name = fontlist[i];
        if (name.IsNamed()) {
            FindPlatformFont(name.mName, true, aClosure);
        } else {
            FindGenericFonts(name.mType, aLanguage, aClosure);
        }
    }

    
    if (mFamilyList.GetDefaultFontType() != eFamily_none &&
        !mFamilyList.HasDefaultGeneric()) {
        FindGenericFonts(mFamilyList.GetDefaultFontType(),
                         aLanguage,
                         aClosure);
    }
}

void
gfxFontGroup::BuildFontList()
{

#if defined(XP_MACOSX) || defined(XP_WIN) || defined(ANDROID)
    EnumerateFontList(mStyle.language);
#endif
}

void
gfxFontGroup::FindPlatformFont(const nsAString& aName,
                               bool aUseFontSet,
                               void *aClosure)
{
    bool needsBold;
    gfxFontFamily *family = nullptr;
    gfxFontEntry *fe = nullptr;

    if (aUseFontSet) {
        
        
        
        
        if (mUserFontSet) {
            
            
            family = mUserFontSet->LookupFamily(aName);
            if (family) {
                nsAutoTArray<gfxFontEntry*,4> userfonts;
                family->FindAllFontsForStyle(mStyle, userfonts, needsBold);
                
                uint32_t count = userfonts.Length();
                for (uint32_t i = 0; i < count; i++) {
                    fe = userfonts[i];
                    FamilyFace ff(family, fe, needsBold);
                    ff.CheckState(mSkipDrawing);
                    mFonts.AppendElement(ff);
                }
            }
        }
    }

    
    if (!family) {
        gfxPlatformFontList *fontList = gfxPlatformFontList::PlatformFontList();
        family = fontList->FindFamily(aName, mStyle.systemFont);
        if (family) {
            fe = family->FindFontForStyle(mStyle, needsBold);
        }
    }

    
    if (fe && !HasFont(fe)) {
        mFonts.AppendElement(FamilyFace(family, fe, needsBold));
    }
}

bool
gfxFontGroup::HasFont(const gfxFontEntry *aFontEntry)
{
    uint32_t count = mFonts.Length();
    for (uint32_t i = 0; i < count; ++i) {
        if (mFonts[i].FontEntry() == aFontEntry) {
            return true;
        }
    }
    return false;
}

gfxFont*
gfxFontGroup::GetFontAt(int32_t i, uint32_t aCh)
{
    if (uint32_t(i) >= mFonts.Length()) {
        return nullptr;
    }

    FamilyFace& ff = mFonts[i];
    if (ff.IsInvalid() || ff.IsLoading()) {
        return nullptr;
    }

    nsRefPtr<gfxFont> font = ff.Font();
    if (!font) {
        gfxFontEntry* fe = mFonts[i].FontEntry();
        gfxCharacterMap* unicodeRangeMap = nullptr;
        if (fe->mIsUserFontContainer) {
            gfxUserFontEntry* ufe = static_cast<gfxUserFontEntry*>(fe);
            if (ufe->LoadState() == gfxUserFontEntry::STATUS_NOT_LOADED &&
                ufe->CharacterInUnicodeRange(aCh) &&
                !FontLoadingForFamily(ff.Family(), aCh)) {
                ufe->Load();
                ff.CheckState(mSkipDrawing);
            }
            fe = ufe->GetPlatformFontEntry();
            if (!fe) {
                return nullptr;
            }
            unicodeRangeMap = ufe->GetUnicodeRangeMap();
        }
        font = fe->FindOrMakeFont(&mStyle, mFonts[i].NeedsBold(),
                                  unicodeRangeMap);
        if (!font || !font->Valid()) {
            ff.SetInvalid();
            return nullptr;
        }
        mFonts[i].SetFont(font);
    }
    return font.get();
}

void
gfxFontGroup::FamilyFace::CheckState(bool& aSkipDrawing)
{
    gfxFontEntry* fe = FontEntry();
    if (fe->mIsUserFontContainer) {
        gfxUserFontEntry* ufe = static_cast<gfxUserFontEntry*>(fe);
        gfxUserFontEntry::UserFontLoadState state = ufe->LoadState();
        switch (state) {
            case gfxUserFontEntry::STATUS_LOADING:
                SetLoading(true);
                break;
            case gfxUserFontEntry::STATUS_FAILED:
                SetInvalid();
                
            default:
                SetLoading(false);
        }
        if (ufe->WaitForUserFont()) {
            aSkipDrawing = true;
        }
    }
}

bool
gfxFontGroup::FamilyFace::EqualsUserFont(const gfxUserFontEntry* aUserFont) const
{
    gfxFontEntry* fe = FontEntry();
    
    if (mFontCreated) {
        gfxFontEntry* pfe = aUserFont->GetPlatformFontEntry();
        if (pfe == fe) {
            return true;
        }
    } else if (fe == aUserFont) {
        return true;
    }
    return false;
}

bool
gfxFontGroup::FontLoadingForFamily(gfxFontFamily* aFamily, uint32_t aCh) const
{
    uint32_t count = mFonts.Length();
    for (uint32_t i = 0; i < count; ++i) {
        const FamilyFace& ff = mFonts[i];
        if (ff.IsLoading() && ff.Family() == aFamily) {
            const gfxUserFontEntry* ufe =
                static_cast<gfxUserFontEntry*>(ff.FontEntry());
            if (ufe->CharacterInUnicodeRange(aCh)) {
                return true;
            }
        }
    }
    return false;
}

gfxFont*
gfxFontGroup::GetDefaultFont()
{
    if (mDefaultFont) {
        return mDefaultFont.get();
    }

    bool needsBold;
    gfxPlatformFontList *pfl = gfxPlatformFontList::PlatformFontList();
    gfxFontFamily *defaultFamily = pfl->GetDefaultFont(&mStyle);
    NS_ASSERTION(defaultFamily,
                 "invalid default font returned by GetDefaultFont");

    if (defaultFamily) {
        gfxFontEntry *fe = defaultFamily->FindFontForStyle(mStyle,
                                                           needsBold);
        if (fe) {
            mDefaultFont = fe->FindOrMakeFont(&mStyle, needsBold);
        }
    }

    if (!mDefaultFont) {
        
        
        
        
        
        nsAutoTArray<nsRefPtr<gfxFontFamily>,200> families;
        pfl->GetFontFamilyList(families);
        uint32_t count = families.Length();
        for (uint32_t i = 0; i < count; ++i) {
            gfxFontEntry *fe = families[i]->FindFontForStyle(mStyle,
                                                             needsBold);
            if (fe) {
                mDefaultFont = fe->FindOrMakeFont(&mStyle, needsBold);
            }
        }
    }

    if (!mDefaultFont) {
        
        
        char msg[256]; 
        nsAutoString families;
        mFamilyList.ToString(families);
        sprintf(msg, "unable to find a usable font (%.220s)",
                NS_ConvertUTF16toUTF8(families).get());
        NS_RUNTIMEABORT(msg);
    }

    return mDefaultFont.get();
}


gfxFont*
gfxFontGroup::GetFirstValidFont(uint32_t aCh)
{
    uint32_t count = mFonts.Length();
    for (uint32_t i = 0; i < count; ++i) {
        FamilyFace& ff = mFonts[i];
        if (ff.IsInvalid()) {
            continue;
        }

        
        gfxFont* font = ff.Font();
        if (font) {
            return font;
        }

        
        
        
        if (ff.IsUserFontContainer()) {
            gfxUserFontEntry* ufe =
                static_cast<gfxUserFontEntry*>(mFonts[i].FontEntry());
            bool inRange = ufe->CharacterInUnicodeRange(aCh);
            if (ufe->LoadState() == gfxUserFontEntry::STATUS_NOT_LOADED &&
                inRange && !FontLoadingForFamily(ff.Family(), aCh)) {
                ufe->Load();
                ff.CheckState(mSkipDrawing);
            }
            if (ufe->LoadState() != gfxUserFontEntry::STATUS_LOADED ||
                !inRange) {
                continue;
            }
        }

        font = GetFontAt(i, aCh);
        if (font) {
            return font;
        }
    }
    return GetDefaultFont();
}

gfxFont *
gfxFontGroup::GetFirstMathFont()
{
    uint32_t count = mFonts.Length();
    for (uint32_t i = 0; i < count; ++i) {
        gfxFont* font = GetFontAt(i);
        if (font && font->GetFontEntry()->TryGetMathTable()) {
            return font;
        }
    }
    return nullptr;
}

gfxFontGroup *
gfxFontGroup::Copy(const gfxFontStyle *aStyle)
{
    gfxFontGroup *fg = new gfxFontGroup(mFamilyList, aStyle, mUserFontSet);
    fg->SetTextPerfMetrics(mTextPerf);
    return fg;
}

bool 
gfxFontGroup::IsInvalidChar(uint8_t ch)
{
    return ((ch & 0x7f) < 0x20 || ch == 0x7f);
}

bool 
gfxFontGroup::IsInvalidChar(char16_t ch)
{
    
    if (ch >= ' ' && ch < 0x7f) {
        return false;
    }
    
    if (ch <= 0x9f) {
        return true;
    }
    return (((ch & 0xFF00) == 0x2000  &&
             (ch == 0x200B || ch == 0x2028 || ch == 0x2029)) ||
            IsBidiControl(ch));
}

gfxTextRun *
gfxFontGroup::MakeEmptyTextRun(const Parameters *aParams, uint32_t aFlags)
{
    aFlags |= TEXT_IS_8BIT | TEXT_IS_ASCII | TEXT_IS_PERSISTENT;
    return gfxTextRun::Create(aParams, 0, this, aFlags);
}

gfxTextRun *
gfxFontGroup::MakeSpaceTextRun(const Parameters *aParams, uint32_t aFlags)
{
    aFlags |= TEXT_IS_8BIT | TEXT_IS_ASCII | TEXT_IS_PERSISTENT;

    gfxTextRun *textRun = gfxTextRun::Create(aParams, 1, this, aFlags);
    if (!textRun) {
        return nullptr;
    }

    uint16_t orientation = aFlags & TEXT_ORIENT_MASK;
    if (orientation == TEXT_ORIENT_VERTICAL_MIXED) {
        orientation = TEXT_ORIENT_VERTICAL_SIDEWAYS_RIGHT;
    }

    gfxFont *font = GetFirstValidFont();
    if (MOZ_UNLIKELY(GetStyle()->size == 0) ||
        MOZ_UNLIKELY(GetStyle()->sizeAdjust == 0.0f)) {
        
        
        
        textRun->AddGlyphRun(font, gfxTextRange::kFontGroup, 0, false,
                             orientation);
    }
    else {
        if (font->GetSpaceGlyph()) {
            
            
            textRun->SetSpaceGlyph(font, aParams->mContext, 0, orientation);
        } else {
            
            
            uint8_t matchType;
            nsRefPtr<gfxFont> spaceFont =
                FindFontForChar(' ', 0, 0, MOZ_SCRIPT_LATIN, nullptr,
                                &matchType);
            if (spaceFont) {
                textRun->SetSpaceGlyph(spaceFont, aParams->mContext, 0,
                                       orientation);
            }
        }
    }

    
    
    
    return textRun;
}

gfxTextRun *
gfxFontGroup::MakeBlankTextRun(uint32_t aLength,
                               const Parameters *aParams, uint32_t aFlags)
{
    gfxTextRun *textRun =
        gfxTextRun::Create(aParams, aLength, this, aFlags);
    if (!textRun) {
        return nullptr;
    }

    uint16_t orientation = aFlags & TEXT_ORIENT_MASK;
    if (orientation == TEXT_ORIENT_VERTICAL_MIXED) {
        orientation = TEXT_ORIENT_VERTICAL_UPRIGHT;
    }
    textRun->AddGlyphRun(GetFirstValidFont(), gfxTextRange::kFontGroup, 0, false,
                         orientation);
    return textRun;
}

gfxTextRun *
gfxFontGroup::MakeHyphenTextRun(gfxContext *aCtx, uint32_t aAppUnitsPerDevUnit)
{
    
    
    
    static const char16_t hyphen = 0x2010;
    gfxFont *font = GetFirstValidFont(uint32_t(hyphen));
    if (font->HasCharacter(hyphen)) {
        return MakeTextRun(&hyphen, 1, aCtx, aAppUnitsPerDevUnit,
                           gfxFontGroup::TEXT_IS_PERSISTENT, nullptr);
    }

    static const uint8_t dash = '-';
    return MakeTextRun(&dash, 1, aCtx, aAppUnitsPerDevUnit,
                       gfxFontGroup::TEXT_IS_PERSISTENT, nullptr);
}

gfxFloat
gfxFontGroup::GetHyphenWidth(gfxTextRun::PropertyProvider *aProvider)
{
    if (mHyphenWidth < 0) {
        nsRefPtr<gfxContext> ctx(aProvider->GetContext());
        if (ctx) {
            nsAutoPtr<gfxTextRun>
                hyphRun(MakeHyphenTextRun(ctx,
                                          aProvider->GetAppUnitsPerDevUnit()));
            mHyphenWidth = hyphRun.get() ?
                hyphRun->GetAdvanceWidth(0, hyphRun->GetLength(), nullptr) : 0;
        }
    }
    return mHyphenWidth;
}

gfxTextRun *
gfxFontGroup::MakeTextRun(const uint8_t *aString, uint32_t aLength,
                          const Parameters *aParams, uint32_t aFlags,
                          gfxMissingFontRecorder *aMFR)
{
    if (aLength == 0) {
        return MakeEmptyTextRun(aParams, aFlags);
    }
    if (aLength == 1 && aString[0] == ' ') {
        return MakeSpaceTextRun(aParams, aFlags);
    }

    aFlags |= TEXT_IS_8BIT;

    if (MOZ_UNLIKELY(GetStyle()->size == 0) ||
        MOZ_UNLIKELY(GetStyle()->sizeAdjust == 0.0f)) {
        
        
        
        return MakeBlankTextRun(aLength, aParams, aFlags);
    }

    gfxTextRun *textRun = gfxTextRun::Create(aParams, aLength,
                                             this, aFlags);
    if (!textRun) {
        return nullptr;
    }

    InitTextRun(aParams->mContext, textRun, aString, aLength, aMFR);

    textRun->FetchGlyphExtents(aParams->mContext);

    return textRun;
}

gfxTextRun *
gfxFontGroup::MakeTextRun(const char16_t *aString, uint32_t aLength,
                          const Parameters *aParams, uint32_t aFlags,
                          gfxMissingFontRecorder *aMFR)
{
    if (aLength == 0) {
        return MakeEmptyTextRun(aParams, aFlags);
    }
    if (aLength == 1 && aString[0] == ' ') {
        return MakeSpaceTextRun(aParams, aFlags);
    }
    if (MOZ_UNLIKELY(GetStyle()->size == 0) ||
        MOZ_UNLIKELY(GetStyle()->sizeAdjust == 0.0f)) {
        return MakeBlankTextRun(aLength, aParams, aFlags);
    }

    gfxTextRun *textRun = gfxTextRun::Create(aParams, aLength,
                                             this, aFlags);
    if (!textRun) {
        return nullptr;
    }

    InitTextRun(aParams->mContext, textRun, aString, aLength, aMFR);

    textRun->FetchGlyphExtents(aParams->mContext);

    return textRun;
}

template<typename T>
void
gfxFontGroup::InitTextRun(gfxContext *aContext,
                          gfxTextRun *aTextRun,
                          const T *aString,
                          uint32_t aLength,
                          gfxMissingFontRecorder *aMFR)
{
    NS_ASSERTION(aLength > 0, "don't call InitTextRun for a zero-length run");

    
    
    int32_t numOption = gfxPlatform::GetPlatform()->GetBidiNumeralOption();
    nsAutoArrayPtr<char16_t> transformedString;
    if (numOption != IBMBIDI_NUMERAL_NOMINAL) {
        
        
        
        bool prevIsArabic =
            (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_INCOMING_ARABICCHAR) != 0;
        for (uint32_t i = 0; i < aLength; ++i) {
            char16_t origCh = aString[i];
            char16_t newCh = HandleNumberInChar(origCh, prevIsArabic, numOption);
            if (newCh != origCh) {
                if (!transformedString) {
                    transformedString = new char16_t[aLength];
                    if (sizeof(T) == sizeof(char16_t)) {
                        memcpy(transformedString.get(), aString, i * sizeof(char16_t));
                    } else {
                        for (uint32_t j = 0; j < i; ++j) {
                            transformedString[j] = aString[j];
                        }
                    }
                }
            }
            if (transformedString) {
                transformedString[i] = newCh;
            }
            prevIsArabic = IS_ARABIC_CHAR(newCh);
        }
    }

#ifdef PR_LOGGING
    PRLogModuleInfo *log = (mStyle.systemFont ?
                            gfxPlatform::GetLog(eGfxLog_textrunui) :
                            gfxPlatform::GetLog(eGfxLog_textrun));
#endif

    
    bool redo;
    do {
        redo = false;

        if (sizeof(T) == sizeof(uint8_t) && !transformedString) {

#ifdef PR_LOGGING
            if (MOZ_UNLIKELY(PR_LOG_TEST(log, PR_LOG_WARNING))) {
                nsAutoCString lang;
                mStyle.language->ToUTF8String(lang);
                nsAutoString families;
                mFamilyList.ToString(families);
                nsAutoCString str((const char*)aString, aLength);
                PR_LOG(log, PR_LOG_WARNING,\
                       ("(%s) fontgroup: [%s] default: %s lang: %s script: %d "
                        "len %d weight: %d width: %d style: %s size: %6.2f %d-byte "
                        "TEXTRUN [%s] ENDTEXTRUN\n",
                        (mStyle.systemFont ? "textrunui" : "textrun"),
                        NS_ConvertUTF16toUTF8(families).get(),
                        (mFamilyList.GetDefaultFontType() == eFamily_serif ?
                         "serif" :
                         (mFamilyList.GetDefaultFontType() == eFamily_sans_serif ?
                          "sans-serif" : "none")),
                        lang.get(), MOZ_SCRIPT_LATIN, aLength,
                        uint32_t(mStyle.weight), uint32_t(mStyle.stretch),
                        (mStyle.style & NS_FONT_STYLE_ITALIC ? "italic" :
                        (mStyle.style & NS_FONT_STYLE_OBLIQUE ? "oblique" :
                                                                "normal")),
                        mStyle.size,
                        sizeof(T),
                        str.get()));
            }
#endif

            
            
            InitScriptRun(aContext, aTextRun, aString,
                          0, aLength, MOZ_SCRIPT_LATIN, aMFR);
        } else {
            const char16_t *textPtr;
            if (transformedString) {
                textPtr = transformedString.get();
            } else {
                
                
                textPtr = reinterpret_cast<const char16_t*>(aString);
            }

            
            
            gfxScriptItemizer scriptRuns(textPtr, aLength);

            uint32_t runStart = 0, runLimit = aLength;
            int32_t runScript = MOZ_SCRIPT_LATIN;
            while (scriptRuns.Next(runStart, runLimit, runScript)) {

    #ifdef PR_LOGGING
                if (MOZ_UNLIKELY(PR_LOG_TEST(log, PR_LOG_WARNING))) {
                    nsAutoCString lang;
                    mStyle.language->ToUTF8String(lang);
                    nsAutoString families;
                    mFamilyList.ToString(families);
                    uint32_t runLen = runLimit - runStart;
                    PR_LOG(log, PR_LOG_WARNING,\
                           ("(%s) fontgroup: [%s] default: %s lang: %s script: %d "
                            "len %d weight: %d width: %d style: %s size: %6.2f "
                            "%d-byte TEXTRUN [%s] ENDTEXTRUN\n",
                            (mStyle.systemFont ? "textrunui" : "textrun"),
                            NS_ConvertUTF16toUTF8(families).get(),
                            (mFamilyList.GetDefaultFontType() == eFamily_serif ?
                             "serif" :
                             (mFamilyList.GetDefaultFontType() == eFamily_sans_serif ?
                              "sans-serif" : "none")),
                            lang.get(), runScript, runLen,
                            uint32_t(mStyle.weight), uint32_t(mStyle.stretch),
                            (mStyle.style & NS_FONT_STYLE_ITALIC ? "italic" :
                            (mStyle.style & NS_FONT_STYLE_OBLIQUE ? "oblique" :
                                                                    "normal")),
                            mStyle.size,
                            sizeof(T),
                            NS_ConvertUTF16toUTF8(textPtr + runStart, runLen).get()));
                }
    #endif

                InitScriptRun(aContext, aTextRun, textPtr + runStart,
                              runStart, runLimit - runStart, runScript, aMFR);
            }
        }

        
        
        if (aTextRun->GetShapingState() == gfxTextRun::eShapingState_Aborted) {
            redo = true;
            aTextRun->SetShapingState(
                gfxTextRun::eShapingState_ForceFallbackFeature);
            aTextRun->ClearGlyphsAndCharacters();
        }

    } while (redo);

    if (sizeof(T) == sizeof(char16_t) && aLength > 0) {
        gfxTextRun::CompressedGlyph *glyph = aTextRun->GetCharacterGlyphs();
        if (!glyph->IsSimpleGlyph()) {
            glyph->SetClusterStart(true);
        }
    }

    
    
    
    
    
    
    
    
    
    aTextRun->SanitizeGlyphRuns();

    aTextRun->SortGlyphRuns();
}

static inline bool
IsPUA(uint32_t aUSV)
{
    
    
    return (aUSV >= 0xE000 && aUSV <= 0xF8FF) || (aUSV >= 0xF0000);
}

template<typename T>
void
gfxFontGroup::InitScriptRun(gfxContext *aContext,
                            gfxTextRun *aTextRun,
                            const T *aString, 
                                              
                            uint32_t aOffset, 
                                              
                            uint32_t aLength, 
                            int32_t aRunScript,
                            gfxMissingFontRecorder *aMFR)
{
    NS_ASSERTION(aLength > 0, "don't call InitScriptRun for a 0-length run");
    NS_ASSERTION(aTextRun->GetShapingState() != gfxTextRun::eShapingState_Aborted,
                 "don't call InitScriptRun with aborted shaping state");

#if defined(XP_MACOSX) || defined(XP_WIN) || defined(ANDROID)
    
    
    if (mUserFontSet && mCurrGeneration != mUserFontSet->GetGeneration()) {
        UpdateUserFonts();
    }
#endif

    gfxFont *mainFont = GetFirstValidFont();

    uint32_t runStart = 0;
    nsAutoTArray<gfxTextRange,3> fontRanges;
    ComputeRanges(fontRanges, aString, aLength, aRunScript,
                  aTextRun->GetFlags() & gfxTextRunFactory::TEXT_ORIENT_MASK);
    uint32_t numRanges = fontRanges.Length();
    bool missingChars = false;

    for (uint32_t r = 0; r < numRanges; r++) {
        const gfxTextRange& range = fontRanges[r];
        uint32_t matchedLength = range.Length();
        gfxFont *matchedFont = range.font;
        bool vertical =
            range.orientation == gfxTextRunFactory::TEXT_ORIENT_VERTICAL_UPRIGHT;
        
        if (matchedFont && mStyle.noFallbackVariantFeatures) {
            
            
            aTextRun->AddGlyphRun(matchedFont, range.matchType,
                                  aOffset + runStart, (matchedLength > 0),
                                  range.orientation);
            if (!matchedFont->SplitAndInitTextRun(aContext, aTextRun,
                                                  aString + runStart,
                                                  aOffset + runStart,
                                                  matchedLength,
                                                  aRunScript,
                                                  vertical)) {
                
                matchedFont = nullptr;
            }
        } else if (matchedFont) {
            
            bool petiteToSmallCaps = false;
            bool syntheticLower = false;
            bool syntheticUpper = false;

            if (mStyle.variantSubSuper != NS_FONT_VARIANT_POSITION_NORMAL &&
                (aTextRun->GetShapingState() ==
                     gfxTextRun::eShapingState_ForceFallbackFeature ||
                 !matchedFont->SupportsSubSuperscript(mStyle.variantSubSuper,
                                                      aString, aLength,
                                                      aRunScript)))
            {
                

                
                
                gfxTextRun::ShapingState ss = aTextRun->GetShapingState();

                if (ss == gfxTextRun::eShapingState_Normal) {
                    aTextRun->SetShapingState(gfxTextRun::eShapingState_ShapingWithFallback);
                } else if (ss == gfxTextRun::eShapingState_ShapingWithFeature) {
                    aTextRun->SetShapingState(gfxTextRun::eShapingState_Aborted);
                    return;
                }

                nsRefPtr<gfxFont> subSuperFont =
                    matchedFont->GetSubSuperscriptFont(aTextRun->GetAppUnitsPerDevUnit());
                aTextRun->AddGlyphRun(subSuperFont, range.matchType,
                                      aOffset + runStart, (matchedLength > 0),
                                      range.orientation);
                if (!subSuperFont->SplitAndInitTextRun(aContext, aTextRun,
                                                       aString + runStart,
                                                       aOffset + runStart,
                                                       matchedLength,
                                                       aRunScript,
                                                       vertical)) {
                    
                    matchedFont = nullptr;
                }
            } else if (mStyle.variantCaps != NS_FONT_VARIANT_CAPS_NORMAL &&
                       !matchedFont->SupportsVariantCaps(aRunScript,
                                                         mStyle.variantCaps,
                                                         petiteToSmallCaps,
                                                         syntheticLower,
                                                         syntheticUpper))
            {
                
                if (!matchedFont->InitFakeSmallCapsRun(aContext, aTextRun,
                                                       aString + runStart,
                                                       aOffset + runStart,
                                                       matchedLength,
                                                       range.matchType,
                                                       range.orientation,
                                                       aRunScript,
                                                       syntheticLower,
                                                       syntheticUpper)) {
                    matchedFont = nullptr;
                }
            } else {
                
                gfxTextRun::ShapingState ss = aTextRun->GetShapingState();

                
                if (ss == gfxTextRun::eShapingState_Normal) {
                    aTextRun->SetShapingState(gfxTextRun::eShapingState_ShapingWithFeature);
                } else if (ss == gfxTextRun::eShapingState_ShapingWithFallback) {
                    
                    aTextRun->SetShapingState(gfxTextRun::eShapingState_Aborted);
                    return;
                }

                
                aTextRun->AddGlyphRun(matchedFont, range.matchType,
                                      aOffset + runStart, (matchedLength > 0),
                                      range.orientation);
                if (!matchedFont->SplitAndInitTextRun(aContext, aTextRun,
                                                      aString + runStart,
                                                      aOffset + runStart,
                                                      matchedLength,
                                                      aRunScript,
                                                      vertical)) {
                    
                    matchedFont = nullptr;
                }
            }
        } else {
            aTextRun->AddGlyphRun(mainFont, gfxTextRange::kFontGroup,
                                  aOffset + runStart, (matchedLength > 0),
                                  range.orientation);
        }

        if (!matchedFont) {
            
            
            
            aTextRun->SetupClusterBoundaries(aOffset + runStart, aString + runStart,
                                             matchedLength);

            
            
            uint32_t runLimit = runStart + matchedLength;
            for (uint32_t index = runStart; index < runLimit; index++) {
                T ch = aString[index];

                
                
                if (ch == '\n') {
                    aTextRun->SetIsNewline(aOffset + index);
                    continue;
                }
                if (ch == '\t') {
                    aTextRun->SetIsTab(aOffset + index);
                    continue;
                }

                
                
                if (sizeof(T) == sizeof(char16_t)) {
                    if (NS_IS_HIGH_SURROGATE(ch) &&
                        index + 1 < aLength &&
                        NS_IS_LOW_SURROGATE(aString[index + 1]))
                    {
                        uint32_t usv =
                            SURROGATE_TO_UCS4(ch, aString[index + 1]);
                        aTextRun->SetMissingGlyph(aOffset + index,
                                                  usv,
                                                  mainFont);
                        index++;
                        if (!mSkipDrawing && !IsPUA(usv)) {
                            missingChars = true;
                        }
                        continue;
                    }

                    
                    
                    gfxFloat wid = mainFont->SynthesizeSpaceWidth(ch);
                    if (wid >= 0.0) {
                        nscoord advance =
                            aTextRun->GetAppUnitsPerDevUnit() * floor(wid + 0.5);
                        if (gfxShapedText::CompressedGlyph::IsSimpleAdvance(advance)) {
                            aTextRun->GetCharacterGlyphs()[aOffset + index].
                                SetSimpleGlyph(advance,
                                               mainFont->GetSpaceGlyph());
                        } else {
                            gfxTextRun::DetailedGlyph detailedGlyph;
                            detailedGlyph.mGlyphID = mainFont->GetSpaceGlyph();
                            detailedGlyph.mAdvance = advance;
                            detailedGlyph.mXOffset = detailedGlyph.mYOffset = 0;
                            gfxShapedText::CompressedGlyph g;
                            g.SetComplex(true, true, 1);
                            aTextRun->SetGlyphs(aOffset + index,
                                                g, &detailedGlyph);
                        }
                        continue;
                    }
                }

                if (IsInvalidChar(ch)) {
                    
                    continue;
                }

                
                aTextRun->SetMissingGlyph(aOffset + index, ch, mainFont);
                if (!mSkipDrawing && !IsPUA(ch)) {
                    missingChars = true;
                }
            }
        }

        runStart += matchedLength;
    }

    if (aMFR && missingChars) {
        aMFR->RecordScript(aRunScript);
    }
}

gfxTextRun *
gfxFontGroup::GetEllipsisTextRun(int32_t aAppUnitsPerDevPixel,
                                 LazyReferenceContextGetter& aRefContextGetter)
{
    if (mCachedEllipsisTextRun &&
        mCachedEllipsisTextRun->GetAppUnitsPerDevUnit() == aAppUnitsPerDevPixel) {
        return mCachedEllipsisTextRun;
    }

    
    
    gfxFont* firstFont = GetFirstValidFont(uint32_t(kEllipsisChar[0]));
    nsString ellipsis = firstFont->HasCharacter(kEllipsisChar[0])
        ? nsDependentString(kEllipsisChar,
                            ArrayLength(kEllipsisChar) - 1)
        : nsDependentString(kASCIIPeriodsChar,
                            ArrayLength(kASCIIPeriodsChar) - 1);

    nsRefPtr<gfxContext> refCtx = aRefContextGetter.GetRefContext();
    Parameters params = {
        refCtx, nullptr, nullptr, nullptr, 0, aAppUnitsPerDevPixel
    };
    gfxTextRun* textRun =
        MakeTextRun(ellipsis.get(), ellipsis.Length(), &params,
                    TEXT_IS_PERSISTENT, nullptr);
    if (!textRun) {
        return nullptr;
    }
    mCachedEllipsisTextRun = textRun;
    textRun->ReleaseFontGroup(); 
                                 
    return textRun;
}

already_AddRefed<gfxFont>
gfxFontGroup::FindNonItalicFaceForChar(gfxFontFamily* aFamily, uint32_t aCh)
{
    NS_ASSERTION(mStyle.style != NS_FONT_STYLE_NORMAL,
                 "should only be called in the italic/oblique case");

    if (!aFamily->TestCharacterMap(aCh)) {
        return nullptr;
    }

    gfxFontStyle regularStyle = mStyle;
    regularStyle.style = NS_FONT_STYLE_NORMAL;
    bool needsBold;
    gfxFontEntry *fe = aFamily->FindFontForStyle(regularStyle, needsBold);
    NS_ASSERTION(!fe->mIsUserFontContainer,
                 "should only be searching platform fonts");
    if (!fe->HasCharacter(aCh)) {
        return nullptr;
    }

    nsRefPtr<gfxFont> font = fe->FindOrMakeFont(&mStyle, needsBold);
    if (!font->Valid()) {
        return nullptr;
    }
    return font.forget();
}

gfxFloat
gfxFontGroup::GetUnderlineOffset()
{
    if (mUnderlineOffset == UNDERLINE_OFFSET_NOT_SET) {
        
        
        uint32_t len = mFonts.Length();
        for (uint32_t i = 0; i < len; i++) {
            FamilyFace& ff = mFonts[i];
            if (!ff.IsUserFontContainer() &&
                !ff.FontEntry()->IsUserFont() &&
                ff.Family() &&
                ff.Family()->IsBadUnderlineFamily()) {
                nsRefPtr<gfxFont> font = GetFontAt(i);
                if (!font) {
                    continue;
                }
                gfxFloat bad = font->GetMetrics(gfxFont::eHorizontal).
                                         underlineOffset;
                gfxFloat first =
                    GetFirstValidFont()->GetMetrics(gfxFont::eHorizontal).
                                             underlineOffset;
                mUnderlineOffset = std::min(first, bad);
                return mUnderlineOffset;
            }
        }

        
        mUnderlineOffset = GetFirstValidFont()->
            GetMetrics(gfxFont::eHorizontal).underlineOffset;
    }

    return mUnderlineOffset;
}

already_AddRefed<gfxFont>
gfxFontGroup::FindFontForChar(uint32_t aCh, uint32_t aPrevCh, uint32_t aNextCh,
                              int32_t aRunScript, gfxFont *aPrevMatchedFont,
                              uint8_t *aMatchType)
{
    
    
    
    if (aPrevMatchedFont && IsClusterExtender(aCh) &&
        aPrevMatchedFont->HasCharacter(aCh)) {
        nsRefPtr<gfxFont> ret = aPrevMatchedFont;
        return ret.forget();
    }

    
    
    uint32_t nextIndex = 0;
    bool isJoinControl = gfxFontUtils::IsJoinControl(aCh);
    bool wasJoinCauser = gfxFontUtils::IsJoinCauser(aPrevCh);
    bool isVarSelector = gfxFontUtils::IsVarSelector(aCh);

    if (!isJoinControl && !wasJoinCauser && !isVarSelector) {
        nsRefPtr<gfxFont> firstFont = GetFontAt(0, aCh);
        if (firstFont) {
            if (firstFont->HasCharacter(aCh)) {
                *aMatchType = gfxTextRange::kFontGroup;
                return firstFont.forget();
            }

            
            
            if (mStyle.style != NS_FONT_STYLE_NORMAL &&
                !firstFont->GetFontEntry()->IsUserFont()) {
                nsRefPtr<gfxFont> font =
                    FindNonItalicFaceForChar(mFonts[0].Family(), aCh);
                if (font) {
                    *aMatchType = gfxTextRange::kFontGroup;
                    return font.forget();
                }
            }
        }

        
        ++nextIndex;
    }

    if (aPrevMatchedFont) {
        
        
        
        if (isJoinControl ||
            GetGeneralCategory(aCh) == HB_UNICODE_GENERAL_CATEGORY_CONTROL) {
            nsRefPtr<gfxFont> ret = aPrevMatchedFont;
            return ret.forget();
        }

        
        
        if (wasJoinCauser) {
            if (aPrevMatchedFont->HasCharacter(aCh)) {
                nsRefPtr<gfxFont> ret = aPrevMatchedFont;
                return ret.forget();
            }
        }
    }

    
    
    
    if (isVarSelector) {
        if (aPrevMatchedFont) {
            nsRefPtr<gfxFont> ret = aPrevMatchedFont;
            return ret.forget();
        }
        
        return nullptr;
    }

    
    uint32_t fontListLength = mFonts.Length();
    for (uint32_t i = nextIndex; i < fontListLength; i++) {
        FamilyFace& ff = mFonts[i];
        if (ff.IsInvalid() || ff.IsLoading()) {
            continue;
        }

        
        nsRefPtr<gfxFont> font = ff.Font();
        if (font) {
            if (font->HasCharacter(aCh)) {
                return font.forget();
            }
            continue;
        }

        
        gfxFontEntry *fe = ff.FontEntry();
        if (fe->mIsUserFontContainer) {
            
            
            gfxUserFontEntry* ufe = static_cast<gfxUserFontEntry*>(fe);

            
            if (!ufe->CharacterInUnicodeRange(aCh)) {
                continue;
            }

            
            
            if (ufe->LoadState() == gfxUserFontEntry::STATUS_NOT_LOADED &&
                !FontLoadingForFamily(ff.Family(), aCh)) {
                ufe->Load();
                ff.CheckState(mSkipDrawing);
            }
            gfxFontEntry* pfe = ufe->GetPlatformFontEntry();
            if (pfe && pfe->HasCharacter(aCh)) {
                font = GetFontAt(i, aCh);
                if (font) {
                    *aMatchType = gfxTextRange::kFontGroup;
                    return font.forget();
                }
            }
        } else if (fe->HasCharacter(aCh)) {
            
            
            font = GetFontAt(i, aCh);
            if (font) {
                *aMatchType = gfxTextRange::kFontGroup;
                return font.forget();
            }
        }

        
        
        if (mStyle.style != NS_FONT_STYLE_NORMAL &&
            !ff.FontEntry()->IsUserFont()) {
            font = FindNonItalicFaceForChar(mFonts[i].Family(), aCh);
            if (font) {
                *aMatchType = gfxTextRange::kFontGroup;
                return font.forget();
            }
        }
    }

    if (fontListLength == 0) {
        nsRefPtr<gfxFont> defaultFont = GetDefaultFont();
        if (defaultFont->HasCharacter(aCh)) {
            *aMatchType = gfxTextRange::kFontGroup;
            return defaultFont.forget();
        }
    }

    
    if ((aCh >= 0xE000  && aCh <= 0xF8FF) || (aCh >= 0xF0000 && aCh <= 0x10FFFD))
        return nullptr;

    
    nsRefPtr<gfxFont> font = WhichPrefFontSupportsChar(aCh);
    if (font) {
        *aMatchType = gfxTextRange::kPrefsFallback;
        return font.forget();
    }

    
    
    if (aPrevMatchedFont && aPrevMatchedFont->HasCharacter(aCh)) {
        *aMatchType = gfxTextRange::kSystemFallback;
        nsRefPtr<gfxFont> ret = aPrevMatchedFont;
        return ret.forget();
    }

    
    if (aRunScript == HB_SCRIPT_UNKNOWN) {
        return nullptr;
    }

    
    
    if (GetGeneralCategory(aCh) ==
            HB_UNICODE_GENERAL_CATEGORY_SPACE_SEPARATOR &&
        GetFirstValidFont()->SynthesizeSpaceWidth(aCh) >= 0.0)
    {
        return nullptr;
    }

    
    *aMatchType = gfxTextRange::kSystemFallback;
    font = WhichSystemFontSupportsChar(aCh, aNextCh, aRunScript);
    return font.forget();
}

template<typename T>
void gfxFontGroup::ComputeRanges(nsTArray<gfxTextRange>& aRanges,
                                 const T *aString, uint32_t aLength,
                                 int32_t aRunScript, uint16_t aOrientation)
{
    NS_ASSERTION(aRanges.Length() == 0, "aRanges must be initially empty");
    NS_ASSERTION(aLength > 0, "don't call ComputeRanges for zero-length text");

    uint32_t prevCh = 0;
    uint32_t nextCh = aString[0];
    if (sizeof(T) == sizeof(char16_t)) {
        if (aLength > 1 && NS_IS_HIGH_SURROGATE(nextCh) &&
                           NS_IS_LOW_SURROGATE(aString[1])) {
            nextCh = SURROGATE_TO_UCS4(nextCh, aString[1]);
        }
    }
    int32_t lastRangeIndex = -1;

    
    
    
    gfxFont *prevFont = GetFirstValidFont();

    
    
    uint8_t matchType = gfxTextRange::kFontGroup;

    for (uint32_t i = 0; i < aLength; i++) {

        const uint32_t origI = i; 

        
        uint32_t ch = nextCh;

        
        

        if (sizeof(T) == sizeof(char16_t)) {
            
            if (ch > 0xffffu) {
                i++;
            }
            if (i < aLength - 1) {
                nextCh = aString[i + 1];
                if ((i + 2 < aLength) && NS_IS_HIGH_SURROGATE(nextCh) &&
                                         NS_IS_LOW_SURROGATE(aString[i + 2])) {
                    nextCh = SURROGATE_TO_UCS4(nextCh, aString[i + 2]);
                }
            } else {
                nextCh = 0;
            }
        } else {
            
            nextCh = i < aLength - 1 ? aString[i + 1] : 0;
        }

        if (ch == 0xa0) {
            ch = ' ';
        }

        
        nsRefPtr<gfxFont> font =
            FindFontForChar(ch, prevCh, nextCh, aRunScript, prevFont,
                            &matchType);

#ifndef RELEASE_BUILD
        if (MOZ_UNLIKELY(mTextPerf)) {
            if (matchType == gfxTextRange::kPrefsFallback) {
                mTextPerf->current.fallbackPrefs++;
            } else if (matchType == gfxTextRange::kSystemFallback) {
                mTextPerf->current.fallbackSystem++;
            }
        }
#endif

        prevCh = ch;

        uint16_t orient = aOrientation;
        if (aOrientation == gfxTextRunFactory::TEXT_ORIENT_VERTICAL_MIXED) {
            
            
            switch (GetVerticalOrientation(ch)) {
            case VERTICAL_ORIENTATION_U:
            case VERTICAL_ORIENTATION_Tr:
            case VERTICAL_ORIENTATION_Tu:
                orient = TEXT_ORIENT_VERTICAL_UPRIGHT;
                break;
            case VERTICAL_ORIENTATION_R:
                orient = TEXT_ORIENT_VERTICAL_SIDEWAYS_RIGHT;
                break;
            }
        }

        if (lastRangeIndex == -1) {
            
            aRanges.AppendElement(gfxTextRange(0, 1, font, matchType, orient));
            lastRangeIndex++;
            prevFont = font;
        } else {
            
            gfxTextRange& prevRange = aRanges[lastRangeIndex];
            if (prevRange.font != font || prevRange.matchType != matchType ||
                prevRange.orientation != orient) {
                
                prevRange.end = origI;
                aRanges.AppendElement(gfxTextRange(origI, i + 1,
                                                   font, matchType, orient));
                lastRangeIndex++;

                
                
                
                if (sizeof(T) == sizeof(uint8_t) ||
                    !gfxFontUtils::IsJoinCauser(ch))
                {
                    prevFont = font;
                }
            }
        }
    }

    aRanges[lastRangeIndex].end = aLength;

#if 0
    
    if (mStyle.systemFont) return;
    for (size_t i = 0, i_end = aRanges.Length(); i < i_end; i++) {
        const gfxTextRange& r = aRanges[i];
        printf("fontmatch %zd:%zd font: %s (%d)\n",
               r.start, r.end,
               (r.font.get() ?
                    NS_ConvertUTF16toUTF8(r.font->GetName()).get() : "<null>"),
               r.matchType);
    }
#endif
}

gfxUserFontSet*
gfxFontGroup::GetUserFontSet()
{
    return mUserFontSet;
}

void
gfxFontGroup::SetUserFontSet(gfxUserFontSet *aUserFontSet)
{
    if (aUserFontSet == mUserFontSet) {
        return;
    }
    mUserFontSet = aUserFontSet;
    mCurrGeneration = GetGeneration() - 1;
    UpdateUserFonts();
}

uint64_t
gfxFontGroup::GetGeneration()
{
    if (!mUserFontSet)
        return 0;
    return mUserFontSet->GetGeneration();
}

uint64_t
gfxFontGroup::GetRebuildGeneration()
{
    if (!mUserFontSet)
        return 0;
    return mUserFontSet->GetRebuildGeneration();
}



void
gfxFontGroup::UpdateUserFonts()
{
    if (mCurrGeneration < GetRebuildGeneration()) {
        
        mFonts.Clear();
        mUnderlineOffset = UNDERLINE_OFFSET_NOT_SET;
        mSkipDrawing = false;
        BuildFontList();
        mCurrGeneration = GetGeneration();
        mCachedEllipsisTextRun = nullptr;
    } else if (mCurrGeneration != GetGeneration()) {
        
        mSkipDrawing = false;
        mUnderlineOffset = UNDERLINE_OFFSET_NOT_SET;
        mCachedEllipsisTextRun = nullptr;

        uint32_t len = mFonts.Length();
        for (uint32_t i = 0; i < len; i++) {
            FamilyFace& ff = mFonts[i];
            if (ff.Font() || !ff.IsUserFontContainer()) {
                continue;
            }
            ff.CheckState(mSkipDrawing);
        }

        mCurrGeneration = GetGeneration();
    }
}

bool
gfxFontGroup::ContainsUserFont(const gfxUserFontEntry* aUserFont)
{
    UpdateUserFonts();
    
    uint32_t len = mFonts.Length();
    for (uint32_t i = 0; i < len; i++) {
        FamilyFace& ff = mFonts[i];
        if (ff.EqualsUserFont(aUserFont)) {
            return true;
        }
    }
    return false;
}

struct PrefFontCallbackData {
    explicit PrefFontCallbackData(nsTArray<nsRefPtr<gfxFontFamily> >& aFamiliesArray)
        : mPrefFamilies(aFamiliesArray)
    {}

    nsTArray<nsRefPtr<gfxFontFamily> >& mPrefFamilies;

    static bool AddFontFamilyEntry(eFontPrefLang aLang, const nsAString& aName, void *aClosure)
    {
        PrefFontCallbackData *prefFontData = static_cast<PrefFontCallbackData*>(aClosure);

        gfxFontFamily *family = gfxPlatformFontList::PlatformFontList()->FindFamily(aName);
        if (family) {
            prefFontData->mPrefFamilies.AppendElement(family);
        }
        return true;
    }
};

already_AddRefed<gfxFont>
gfxFontGroup::WhichPrefFontSupportsChar(uint32_t aCh)
{
    nsRefPtr<gfxFont> font;

    
    uint32_t unicodeRange = FindCharUnicodeRange(aCh);
    eFontPrefLang charLang = gfxPlatform::GetPlatform()->GetFontPrefLangFor(unicodeRange);

    
    if (mLastPrefFont && charLang == mLastPrefLang &&
        mLastPrefFirstFont && mLastPrefFont->HasCharacter(aCh)) {
        font = mLastPrefFont;
        return font.forget();
    }

    
    eFontPrefLang prefLangs[kMaxLenPrefLangList];
    uint32_t i, numLangs = 0;

    gfxPlatform::GetPlatform()->GetLangPrefs(prefLangs, numLangs, charLang, mPageLang);

    for (i = 0; i < numLangs; i++) {
        nsAutoTArray<nsRefPtr<gfxFontFamily>, 5> families;
        eFontPrefLang currentLang = prefLangs[i];

        gfxPlatformFontList *fontList = gfxPlatformFontList::PlatformFontList();

        
        if (!fontList->GetPrefFontFamilyEntries(currentLang, &families)) {
            eFontPrefLang prefLangsToSearch[1] = { currentLang };
            PrefFontCallbackData prefFontData(families);
            gfxPlatform::ForEachPrefFont(prefLangsToSearch, 1, PrefFontCallbackData::AddFontFamilyEntry,
                                           &prefFontData);
            fontList->SetPrefFontFamilyEntries(currentLang, families);
        }

        
        uint32_t  j, numPrefs;
        numPrefs = families.Length();
        for (j = 0; j < numPrefs; j++) {
            
            gfxFontFamily *family = families[j];
            if (!family) continue;

            
            
            
            
            if (family == mLastPrefFamily && mLastPrefFont->HasCharacter(aCh)) {
                font = mLastPrefFont;
                return font.forget();
            }

            bool needsBold;
            gfxFontEntry *fe = family->FindFontForStyle(mStyle, needsBold);
            
            if (fe && fe->HasCharacter(aCh)) {
                nsRefPtr<gfxFont> prefFont = fe->FindOrMakeFont(&mStyle, needsBold);
                if (!prefFont) continue;
                mLastPrefFamily = family;
                mLastPrefFont = prefFont;
                mLastPrefLang = charLang;
                mLastPrefFirstFont = (i == 0 && j == 0);
                return prefFont.forget();
            }

        }
    }

    return nullptr;
}

already_AddRefed<gfxFont>
gfxFontGroup::WhichSystemFontSupportsChar(uint32_t aCh, uint32_t aNextCh,
                                          int32_t aRunScript)
{
    gfxFontEntry *fe = 
        gfxPlatformFontList::PlatformFontList()->
            SystemFindFontForChar(aCh, aNextCh, aRunScript, &mStyle);
    if (fe) {
        bool wantBold = mStyle.ComputeWeight() >= 6;
        nsRefPtr<gfxFont> font =
            fe->FindOrMakeFont(&mStyle, wantBold && !fe->IsBold());
        return font.forget();
    }

    return nullptr;
}

 void
gfxFontGroup::Shutdown()
{
    NS_IF_RELEASE(gLangService);
}

nsILanguageAtomService* gfxFontGroup::gLangService = nullptr;

void
gfxMissingFontRecorder::Flush()
{
    static bool mNotifiedFontsInitialized = false;
    static uint32_t mNotifiedFonts[gfxMissingFontRecorder::kNumScriptBitsWords];
    if (!mNotifiedFontsInitialized) {
        memset(&mNotifiedFonts, 0, sizeof(mNotifiedFonts));
        mNotifiedFontsInitialized = true;
    }

    nsAutoString fontNeeded;
    for (uint32_t i = 0; i < kNumScriptBitsWords; ++i) {
        mMissingFonts[i] &= ~mNotifiedFonts[i];
        if (!mMissingFonts[i]) {
            continue;
        }
        for (uint32_t j = 0; j < 32; ++j) {
            if (!(mMissingFonts[i] & (1 << j))) {
                continue;
            }
            mNotifiedFonts[i] |= (1 << j);
            if (!fontNeeded.IsEmpty()) {
                fontNeeded.Append(PRUnichar(','));
            }
            uint32_t tag = GetScriptTagForCode(i * 32 + j);
            fontNeeded.Append(char16_t(tag >> 24));
            fontNeeded.Append(char16_t((tag >> 16) & 0xff));
            fontNeeded.Append(char16_t((tag >> 8) & 0xff));
            fontNeeded.Append(char16_t(tag & 0xff));
        }
        mMissingFonts[i] = 0;
    }
    if (!fontNeeded.IsEmpty()) {
        nsCOMPtr<nsIObserverService> service = GetObserverService();
        service->NotifyObservers(nullptr, "font-needed", fontNeeded.get());
    }
}
