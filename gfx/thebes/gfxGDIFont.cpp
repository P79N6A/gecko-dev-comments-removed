




#include "gfxGDIFont.h"

#include "mozilla/MemoryReporting.h"
#include "mozilla/WindowsVersion.h"

#include <algorithm>
#include "gfxWindowsPlatform.h"
#include "gfxContext.h"
#include "mozilla/Preferences.h"
#include "nsUnicodeProperties.h"
#include "gfxFontConstants.h"
#include "gfxTextRun.h"

#include "cairo-win32.h"

#define ROUND(x) floor((x) + 0.5)

using namespace mozilla;
using namespace mozilla::unicode;

static inline cairo_antialias_t
GetCairoAntialiasOption(gfxFont::AntialiasOption anAntialiasOption)
{
    switch (anAntialiasOption) {
    default:
    case gfxFont::kAntialiasDefault:
        return CAIRO_ANTIALIAS_DEFAULT;
    case gfxFont::kAntialiasNone:
        return CAIRO_ANTIALIAS_NONE;
    case gfxFont::kAntialiasGrayscale:
        return CAIRO_ANTIALIAS_GRAY;
    case gfxFont::kAntialiasSubpixel:
        return CAIRO_ANTIALIAS_SUBPIXEL;
    }
}

gfxGDIFont::gfxGDIFont(GDIFontEntry *aFontEntry,
                       const gfxFontStyle *aFontStyle,
                       bool aNeedsBold,
                       AntialiasOption anAAOption)
    : gfxFont(aFontEntry, aFontStyle, anAAOption),
      mFont(nullptr),
      mFontFace(nullptr),
      mMetrics(nullptr),
      mSpaceGlyph(0),
      mNeedsBold(aNeedsBold),
      mScriptCache(nullptr)
{
}

gfxGDIFont::~gfxGDIFont()
{
    if (mScaledFont) {
        cairo_scaled_font_destroy(mScaledFont);
    }
    if (mFontFace) {
        cairo_font_face_destroy(mFontFace);
    }
    if (mFont) {
        ::DeleteObject(mFont);
    }
    if (mScriptCache) {
        ScriptFreeCache(&mScriptCache);
    }
    delete mMetrics;
}

gfxFont*
gfxGDIFont::CopyWithAntialiasOption(AntialiasOption anAAOption)
{
    return new gfxGDIFont(static_cast<GDIFontEntry*>(mFontEntry.get()),
                          &mStyle, mNeedsBold, anAAOption);
}

bool
gfxGDIFont::ShapeText(gfxContext     *aContext,
                      const char16_t *aText,
                      uint32_t        aOffset,
                      uint32_t        aLength,
                      int32_t         aScript,
                      bool            aVertical,
                      gfxShapedText  *aShapedText)
{
    if (!mMetrics) {
        Initialize();
    }
    if (!mIsValid) {
        NS_WARNING("invalid font! expect incorrect text rendering");
        return false;
    }

    
    
    
    
    if (!SetupCairoFont(aContext)) {
        return false;
    }

    return gfxFont::ShapeText(aContext, aText, aOffset, aLength, aScript,
                              aVertical, aShapedText);
}

const gfxFont::Metrics&
gfxGDIFont::GetHorizontalMetrics()
{
    if (!mMetrics) {
        Initialize();
    }
    return *mMetrics;
}

uint32_t
gfxGDIFont::GetSpaceGlyph()
{
    if (!mMetrics) {
        Initialize();
    }
    return mSpaceGlyph;
}

bool
gfxGDIFont::SetupCairoFont(gfxContext *aContext)
{
    if (!mMetrics) {
        Initialize();
    }
    if (!mScaledFont ||
        cairo_scaled_font_status(mScaledFont) != CAIRO_STATUS_SUCCESS) {
        
        
        return false;
    }
    cairo_set_scaled_font(aContext->GetCairo(), mScaledFont);
    return true;
}

gfxFont::RunMetrics
gfxGDIFont::Measure(gfxTextRun *aTextRun,
                    uint32_t aStart, uint32_t aEnd,
                    BoundingBoxType aBoundingBoxType,
                    gfxContext *aRefContext,
                    Spacing *aSpacing,
                    uint16_t aOrientation)
{
    gfxFont::RunMetrics metrics =
        gfxFont::Measure(aTextRun, aStart, aEnd,
                         aBoundingBoxType, aRefContext, aSpacing,
                         aOrientation);

    
    
    
    
    
    if (aBoundingBoxType == LOOSE_INK_EXTENTS &&
        mAntialiasOption != kAntialiasNone &&
        metrics.mBoundingBox.width > 0) {
        metrics.mBoundingBox.x -= aTextRun->GetAppUnitsPerDevUnit();
        metrics.mBoundingBox.width += aTextRun->GetAppUnitsPerDevUnit() * 3;
    }

    return metrics;
}

void
gfxGDIFont::Initialize()
{
    NS_ASSERTION(!mMetrics, "re-creating metrics? this will leak");

    LOGFONTW logFont;

    
    GDIFontEntry* fe = static_cast<GDIFontEntry*>(GetFontEntry());
    bool wantFakeItalic =
        (mStyle.style & (NS_FONT_STYLE_ITALIC | NS_FONT_STYLE_OBLIQUE)) &&
        !fe->IsItalic() && mStyle.allowSyntheticStyle;

    
    
    
    
    
    
    
    
    
    bool useCairoFakeItalic = wantFakeItalic && fe->mFamilyHasItalicFace;

    if (mAdjustedSize == 0.0) {
        mAdjustedSize = mStyle.size;
        if (mStyle.sizeAdjust > 0.0 && mAdjustedSize > 0.0) {
            
            FillLogFont(logFont, mAdjustedSize,
                        wantFakeItalic && !useCairoFakeItalic);
            mFont = ::CreateFontIndirectW(&logFont);

            
            Initialize();

            
            
            gfxFloat aspect = mMetrics->xHeight / mMetrics->emHeight;
            mAdjustedSize = mStyle.GetAdjustedSize(aspect);

            
            ::DeleteObject(mFont);
            mFont = nullptr;
            delete mMetrics;
            mMetrics = nullptr;
        } else if (mStyle.sizeAdjust == 0.0) {
            mAdjustedSize = 0.0;
        }
    }

    
    
    
    if (mNeedsBold && GetFontEntry()->IsLocalUserFont()) {
        mApplySyntheticBold = true;
    }

    
    mAdjustedSize = ROUND(mAdjustedSize);
    FillLogFont(logFont, mAdjustedSize, wantFakeItalic && !useCairoFakeItalic);
    mFont = ::CreateFontIndirectW(&logFont);

    mMetrics = new gfxFont::Metrics;
    ::memset(mMetrics, 0, sizeof(*mMetrics));

    AutoDC dc;
    SetGraphicsMode(dc.GetDC(), GM_ADVANCED);
    AutoSelectFont selectFont(dc.GetDC(), mFont);

    
    if (mAdjustedSize > 0.0) {

        OUTLINETEXTMETRIC oMetrics;
        TEXTMETRIC& metrics = oMetrics.otmTextMetrics;

        if (0 < GetOutlineTextMetrics(dc.GetDC(), sizeof(oMetrics), &oMetrics)) {
            mMetrics->strikeoutSize = (double)oMetrics.otmsStrikeoutSize;
            mMetrics->strikeoutOffset = (double)oMetrics.otmsStrikeoutPosition;
            mMetrics->underlineSize = (double)oMetrics.otmsUnderscoreSize;
            mMetrics->underlineOffset = (double)oMetrics.otmsUnderscorePosition;

            const MAT2 kIdentityMatrix = { {0, 1}, {0, 0}, {0, 0}, {0, 1} };
            GLYPHMETRICS gm;
            DWORD len = GetGlyphOutlineW(dc.GetDC(), char16_t('x'), GGO_METRICS, &gm, 0, nullptr, &kIdentityMatrix);
            if (len == GDI_ERROR || gm.gmptGlyphOrigin.y <= 0) {
                
                mMetrics->xHeight =
                    ROUND((double)metrics.tmAscent * DEFAULT_XHEIGHT_FACTOR);
            } else {
                mMetrics->xHeight = gm.gmptGlyphOrigin.y;
            }
            mMetrics->emHeight = metrics.tmHeight - metrics.tmInternalLeading;
            gfxFloat typEmHeight = (double)oMetrics.otmAscent - (double)oMetrics.otmDescent;
            mMetrics->emAscent = ROUND(mMetrics->emHeight * (double)oMetrics.otmAscent / typEmHeight);
            mMetrics->emDescent = mMetrics->emHeight - mMetrics->emAscent;
            if (oMetrics.otmEMSquare > 0) {
                mFUnitsConvFactor = float(mAdjustedSize / oMetrics.otmEMSquare);
            }
        } else {
            
            

            
            
            BOOL result = GetTextMetrics(dc.GetDC(), &metrics);
            if (!result) {
                NS_WARNING("Missing or corrupt font data, fasten your seatbelt");
                mIsValid = false;
                memset(mMetrics, 0, sizeof(*mMetrics));
                return;
            }

            mMetrics->xHeight =
                ROUND((float)metrics.tmAscent * DEFAULT_XHEIGHT_FACTOR);
            mMetrics->strikeoutSize = 1;
            mMetrics->strikeoutOffset = ROUND(mMetrics->xHeight * 0.5f); 
            mMetrics->underlineSize = 1;
            mMetrics->underlineOffset = -ROUND((float)metrics.tmDescent * 0.30f); 
            mMetrics->emHeight = metrics.tmHeight - metrics.tmInternalLeading;
            mMetrics->emAscent = metrics.tmAscent - metrics.tmInternalLeading;
            mMetrics->emDescent = metrics.tmDescent;
        }

        mMetrics->internalLeading = metrics.tmInternalLeading;
        mMetrics->externalLeading = metrics.tmExternalLeading;
        mMetrics->maxHeight = metrics.tmHeight;
        mMetrics->maxAscent = metrics.tmAscent;
        mMetrics->maxDescent = metrics.tmDescent;
        mMetrics->maxAdvance = metrics.tmMaxCharWidth;
        mMetrics->aveCharWidth = std::max<gfxFloat>(1, metrics.tmAveCharWidth);
        
        
        if (!(metrics.tmPitchAndFamily & TMPF_FIXED_PITCH)) {
            mMetrics->maxAdvance = mMetrics->aveCharWidth;
        }

        
        SIZE size;
        GetTextExtentPoint32W(dc.GetDC(), L" ", 1, &size);
        mMetrics->spaceWidth = ROUND(size.cx);

        
        
        
        
        if (GetTextExtentPoint32W(dc.GetDC(), L"0", 1, &size)) {
            mMetrics->zeroOrAveCharWidth = ROUND(size.cx);
        } else {
            mMetrics->zeroOrAveCharWidth = mMetrics->aveCharWidth;
        }

        WORD glyph;
        DWORD ret = GetGlyphIndicesW(dc.GetDC(), L" ", 1, &glyph,
                                     GGI_MARK_NONEXISTING_GLYPHS);
        if (ret != GDI_ERROR && glyph != 0xFFFF) {
            mSpaceGlyph = glyph;
        }

        SanitizeMetrics(mMetrics, GetFontEntry()->mIsBadUnderlineFont);
    }

    if (IsSyntheticBold()) {
        mMetrics->aveCharWidth += GetSyntheticBoldOffset();
        mMetrics->maxAdvance += GetSyntheticBoldOffset();
    }

    mFontFace = cairo_win32_font_face_create_for_logfontw_hfont(&logFont,
                                                                mFont);

    cairo_matrix_t sizeMatrix, ctm;
    cairo_matrix_init_identity(&ctm);
    cairo_matrix_init_scale(&sizeMatrix, mAdjustedSize, mAdjustedSize);

    if (useCairoFakeItalic) {
        
        
        double skewfactor = OBLIQUE_SKEW_FACTOR;
        cairo_matrix_t style;
        cairo_matrix_init(&style,
                          1,                
                          0,                
                          -1 * skewfactor,  
                          1,                
                          0,                
                          0);               
        cairo_matrix_multiply(&sizeMatrix, &sizeMatrix, &style);
    }

    cairo_font_options_t *fontOptions = cairo_font_options_create();
    if (mAntialiasOption != kAntialiasDefault) {
        cairo_font_options_set_antialias(fontOptions,
            GetCairoAntialiasOption(mAntialiasOption));
    }
    mScaledFont = cairo_scaled_font_create(mFontFace, &sizeMatrix,
                                           &ctm, fontOptions);
    cairo_font_options_destroy(fontOptions);

    if (!mScaledFont ||
        cairo_scaled_font_status(mScaledFont) != CAIRO_STATUS_SUCCESS) {
#ifdef DEBUG
        char warnBuf[1024];
        sprintf(warnBuf, "Failed to create scaled font: %s status: %d",
                NS_ConvertUTF16toUTF8(mFontEntry->Name()).get(),
                mScaledFont ? cairo_scaled_font_status(mScaledFont) : 0);
        NS_WARNING(warnBuf);
#endif
        mIsValid = false;
    } else {
        mIsValid = true;
    }

#if 0
    printf("Font: %p (%s) size: %f adjusted size: %f valid: %s\n", this,
           NS_ConvertUTF16toUTF8(GetName()).get(), mStyle.size, mAdjustedSize, (mIsValid ? "yes" : "no"));
    printf("    emHeight: %f emAscent: %f emDescent: %f\n", mMetrics->emHeight, mMetrics->emAscent, mMetrics->emDescent);
    printf("    maxAscent: %f maxDescent: %f maxAdvance: %f\n", mMetrics->maxAscent, mMetrics->maxDescent, mMetrics->maxAdvance);
    printf("    internalLeading: %f externalLeading: %f\n", mMetrics->internalLeading, mMetrics->externalLeading);
    printf("    spaceWidth: %f aveCharWidth: %f xHeight: %f\n", mMetrics->spaceWidth, mMetrics->aveCharWidth, mMetrics->xHeight);
    printf("    uOff: %f uSize: %f stOff: %f stSize: %f\n",
           mMetrics->underlineOffset, mMetrics->underlineSize, mMetrics->strikeoutOffset, mMetrics->strikeoutSize);
#endif
}

void
gfxGDIFont::FillLogFont(LOGFONTW& aLogFont, gfxFloat aSize,
                        bool aUseGDIFakeItalic)
{
    GDIFontEntry *fe = static_cast<GDIFontEntry*>(GetFontEntry());

    uint16_t weight;
    if (fe->IsUserFont()) {
        if (fe->IsLocalUserFont()) {
            
            
            
            weight = 0;
        } else {
            
            
            weight = mNeedsBold ? 700 : 200;
        }
    } else {
        weight = mNeedsBold ? 700 : fe->Weight();
    }

    fe->FillLogFont(&aLogFont, weight, aSize, 
                    (mAntialiasOption == kAntialiasSubpixel) ? true : false);

    
    if (aUseGDIFakeItalic) {
        aLogFont.lfItalic = 1;
    }
}

uint32_t
gfxGDIFont::GetGlyph(uint32_t aUnicode, uint32_t aVarSelector)
{
    

    
    
    
    if (aUnicode > 0xffff || aVarSelector) {
        return 0;
    }

    if (!mGlyphIDs) {
        mGlyphIDs = new nsDataHashtable<nsUint32HashKey,uint32_t>(64);
    }

    uint32_t gid;
    if (mGlyphIDs->Get(aUnicode, &gid)) {
        return gid;
    }

    wchar_t ch = aUnicode;
    WORD glyph;
    DWORD ret = ScriptGetCMap(nullptr, &mScriptCache, &ch, 1, 0, &glyph);
    if (ret == E_PENDING) {
        AutoDC dc;
        AutoSelectFont fs(dc.GetDC(), GetHFONT());
        ret = ScriptGetCMap(dc.GetDC(), &mScriptCache, &ch, 1, 0, &glyph);
    }
    if (ret != S_OK) {
        glyph = 0;
    }

    mGlyphIDs->Put(aUnicode, glyph);
    return glyph;
}

int32_t
gfxGDIFont::GetGlyphWidth(DrawTarget& aDrawTarget, uint16_t aGID)
{
    if (!mGlyphWidths) {
        mGlyphWidths = new nsDataHashtable<nsUint32HashKey,int32_t>(128);
    }

    int32_t width;
    if (mGlyphWidths->Get(aGID, &width)) {
        return width;
    }

    DCFromDrawTarget dc(aDrawTarget);
    AutoSelectFont fs(dc, GetHFONT());

    int devWidth;
    if (GetCharWidthI(dc, aGID, 1, nullptr, &devWidth)) {
        
        devWidth = std::min(std::max(0, devWidth), 0x7fff);
        width = devWidth << 16;
        mGlyphWidths->Put(aGID, width);
        return width;
    }

    return -1;
}

void
gfxGDIFont::AddSizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                   FontCacheSizes* aSizes) const
{
    gfxFont::AddSizeOfExcludingThis(aMallocSizeOf, aSizes);
    aSizes->mFontInstances += aMallocSizeOf(mMetrics);
    if (mGlyphWidths) {
        aSizes->mFontInstances +=
            mGlyphWidths->SizeOfIncludingThis(nullptr, aMallocSizeOf);
    }
}

void
gfxGDIFont::AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                   FontCacheSizes* aSizes) const
{
    aSizes->mFontInstances += aMallocSizeOf(this);
    AddSizeOfExcludingThis(aMallocSizeOf, aSizes);
}
