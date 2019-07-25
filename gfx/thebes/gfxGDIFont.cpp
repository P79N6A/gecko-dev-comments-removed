








































#include "gfxGDIFont.h"
#include "gfxGDIShaper.h"
#include "gfxUniscribeShaper.h"
#include "gfxHarfBuzzShaper.h"
#include "gfxWindowsPlatform.h"
#include "gfxContext.h"
#include "gfxUnicodeProperties.h"

#include "cairo-win32.h"

#define ROUND(x) floor((x) + 0.5)

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
                       PRBool aNeedsBold,
                       AntialiasOption anAAOption)
    : gfxFont(aFontEntry, aFontStyle, anAAOption),
      mFont(NULL),
      mFontFace(nsnull),
      mScaledFont(nsnull),
      mMetrics(nsnull),
      mSpaceGlyph(0),
      mNeedsBold(aNeedsBold)
{
    if (FontCanSupportHarfBuzz()) {
        mHarfBuzzShaper = new gfxHarfBuzzShaper(this);
    }
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
    delete mMetrics;
}

void
gfxGDIFont::CreatePlatformShaper()
{
    mPlatformShaper = new gfxGDIShaper(this);
}

gfxFont*
gfxGDIFont::CopyWithAntialiasOption(AntialiasOption anAAOption)
{
    return new gfxGDIFont(static_cast<GDIFontEntry*>(mFontEntry.get()),
                          &mStyle, mNeedsBold, anAAOption);
}

static PRBool
UseUniscribe(gfxTextRun *aTextRun,
             const PRUnichar *aString,
             PRUint32 aRunStart,
             PRUint32 aRunLength)
{
    PRUint32 flags = aTextRun->GetFlags();
    PRBool useGDI;

    PRBool isXP = (gfxWindowsPlatform::WindowsOSVersion() 
                       < gfxWindowsPlatform::kWindowsVista);

    
    

    useGDI = isXP &&
             (flags &
               (gfxTextRunFactory::TEXT_OPTIMIZE_SPEED | 
                gfxTextRunFactory::TEXT_IS_RTL)
             ) == gfxTextRunFactory::TEXT_OPTIMIZE_SPEED;

    return !useGDI ||
        ScriptIsComplex(aString + aRunStart, aRunLength, SIC_COMPLEX) == S_OK;
}

PRBool
gfxGDIFont::InitTextRun(gfxContext *aContext,
                        gfxTextRun *aTextRun,
                        const PRUnichar *aString,
                        PRUint32 aRunStart,
                        PRUint32 aRunLength,
                        PRInt32 aRunScript,
                        PRBool aPreferPlatformShaping)
{
    if (!mMetrics) {
        Initialize();
    }
    if (!mIsValid) {
        NS_WARNING("invalid font! expect incorrect text rendering");
        return PR_FALSE;
    }

    PRBool ok = PR_FALSE;

    
    
    SetupCairoFont(aContext);

    if (mHarfBuzzShaper) {
        if (gfxPlatform::GetPlatform()->UseHarfBuzzForScript(aRunScript)) {
            ok = mHarfBuzzShaper->InitTextRun(aContext, aTextRun, aString,
                                              aRunStart, aRunLength, 
                                              aRunScript);
        }
    }

    if (!ok) {
        GDIFontEntry *fe = static_cast<GDIFontEntry*>(GetFontEntry());
        PRBool preferUniscribe =
            (!fe->IsTrueType() || fe->IsSymbolFont()) && !fe->mForceGDI;

        if (preferUniscribe ||
            UseUniscribe(aTextRun, aString, aRunStart, aRunLength))
        {
            
            if (!mUniscribeShaper) {
                mUniscribeShaper = new gfxUniscribeShaper(this);
            }

            ok = mUniscribeShaper->InitTextRun(aContext, aTextRun, aString,
                                               aRunStart, aRunLength, 
                                               aRunScript);
            if (ok) {
                return PR_TRUE;
            }

            
            if (!mPlatformShaper) {
                CreatePlatformShaper();
            }

            ok = mPlatformShaper->InitTextRun(aContext, aTextRun, aString,
                                              aRunStart, aRunLength, 
                                              aRunScript);
        } else {
            
            if (!mPlatformShaper) {
                CreatePlatformShaper();
            }

            ok = mPlatformShaper->InitTextRun(aContext, aTextRun, aString,
                                              aRunStart, aRunLength, 
                                              aRunScript);

            if (ok) {
                return PR_TRUE;
            }

            
            if (!mUniscribeShaper) {
                mUniscribeShaper = new gfxUniscribeShaper(this);
            }

            
            ok = mUniscribeShaper->InitTextRun(aContext, aTextRun, aString,
                                               aRunStart, aRunLength, 
                                               aRunScript);
        }

#if DEBUG
        if (!ok) {
            NS_ConvertUTF16toUTF8 name(GetName());
            char msg[256];

            sprintf(msg, 
                    "text shaping with both uniscribe and GDI failed for"
                    " font: %s",
                    name.get());
            NS_WARNING(msg);
        }
#endif
    }

    return ok;
}

const gfxFont::Metrics&
gfxGDIFont::GetMetrics()
{
    if (!mMetrics) {
        Initialize();
    }
    return *mMetrics;
}

PRUint32
gfxGDIFont::GetSpaceGlyph()
{
    if (!mMetrics) {
        Initialize();
    }
    return mSpaceGlyph;
}

PRBool
gfxGDIFont::SetupCairoFont(gfxContext *aContext)
{
    if (!mMetrics) {
        Initialize();
    }
    if (!mScaledFont ||
        cairo_scaled_font_status(mScaledFont) != CAIRO_STATUS_SUCCESS) {
        
        
        return PR_FALSE;
    }
    cairo_set_scaled_font(aContext->GetCairo(), mScaledFont);
    return PR_TRUE;
}

void
gfxGDIFont::Initialize()
{
    NS_ASSERTION(!mMetrics, "re-creating metrics? this will leak");

    LOGFONTW logFont;

    if (mAdjustedSize == 0.0) {
        mAdjustedSize = mStyle.size;
        if (mStyle.sizeAdjust != 0.0 && mAdjustedSize > 0.0) {
            
            FillLogFont(logFont, mAdjustedSize);
            mFont = ::CreateFontIndirectW(&logFont);

            
            Initialize();

            
            
            gfxFloat aspect = mMetrics->xHeight / mMetrics->emHeight;
            mAdjustedSize = mStyle.GetAdjustedSize(aspect);

            
            ::DeleteObject(mFont);
            mFont = nsnull;
            delete mMetrics;
            mMetrics = nsnull;
        }
    }

    FillLogFont(logFont, mAdjustedSize);
    mFont = ::CreateFontIndirectW(&logFont);

    mMetrics = new gfxFont::Metrics;
    ::memset(mMetrics, 0, sizeof(*mMetrics));

    AutoDC dc;
    SetGraphicsMode(dc.GetDC(), GM_ADVANCED);
    AutoSelectFont selectFont(dc.GetDC(), mFont);

    
    OUTLINETEXTMETRIC oMetrics;
    TEXTMETRIC& metrics = oMetrics.otmTextMetrics;

    if (0 < GetOutlineTextMetrics(dc.GetDC(), sizeof(oMetrics), &oMetrics)) {
        mMetrics->superscriptOffset = (double)oMetrics.otmptSuperscriptOffset.y;
        
        mMetrics->subscriptOffset = fabs((double)oMetrics.otmptSubscriptOffset.y);
        mMetrics->strikeoutSize = (double)oMetrics.otmsStrikeoutSize;
        mMetrics->strikeoutOffset = (double)oMetrics.otmsStrikeoutPosition;
        mMetrics->underlineSize = (double)oMetrics.otmsUnderscoreSize;
        mMetrics->underlineOffset = (double)oMetrics.otmsUnderscorePosition;

        const MAT2 kIdentityMatrix = { {0, 1}, {0, 0}, {0, 0}, {0, 1} };
        GLYPHMETRICS gm;
        DWORD len = GetGlyphOutlineW(dc.GetDC(), PRUnichar('x'), GGO_METRICS, &gm, 0, nsnull, &kIdentityMatrix);
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
            mIsValid = PR_FALSE;
            memset(mMetrics, 0, sizeof(*mMetrics));
            return;
        }

        mMetrics->xHeight =
            ROUND((float)metrics.tmAscent * DEFAULT_XHEIGHT_FACTOR);
        mMetrics->superscriptOffset = mMetrics->xHeight;
        mMetrics->subscriptOffset = mMetrics->xHeight;
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
    mMetrics->aveCharWidth = NS_MAX<gfxFloat>(1, metrics.tmAveCharWidth);
    
    
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

    mSpaceGlyph = 0;
    if (metrics.tmPitchAndFamily & TMPF_TRUETYPE) {
        WORD glyph;
        DWORD ret = GetGlyphIndicesW(dc.GetDC(), L" ", 1, &glyph,
                                     GGI_MARK_NONEXISTING_GLYPHS);
        if (ret != GDI_ERROR && glyph != 0xFFFF) {
            mSpaceGlyph = glyph;
        }
    }

    SanitizeMetrics(mMetrics, GetFontEntry()->mIsBadUnderlineFont);

    mFontFace = cairo_win32_font_face_create_for_logfontw_hfont(&logFont,
                                                                mFont);

    cairo_matrix_t sizeMatrix, ctm;
    cairo_matrix_init_identity(&ctm);
    cairo_matrix_init_scale(&sizeMatrix, mAdjustedSize, mAdjustedSize);

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
    }

    mIsValid = PR_TRUE;

#if 0
    printf("Font: %p (%s) size: %f\n", this,
           NS_ConvertUTF16toUTF8(GetName()).get(), mStyle.size);
    printf("    emHeight: %f emAscent: %f emDescent: %f\n", mMetrics.emHeight, mMetrics.emAscent, mMetrics.emDescent);
    printf("    maxAscent: %f maxDescent: %f maxAdvance: %f\n", mMetrics.maxAscent, mMetrics.maxDescent, mMetrics.maxAdvance);
    printf("    internalLeading: %f externalLeading: %f\n", mMetrics.internalLeading, mMetrics.externalLeading);
    printf("    spaceWidth: %f aveCharWidth: %f xHeight: %f\n", mMetrics.spaceWidth, mMetrics.aveCharWidth, mMetrics.xHeight);
    printf("    uOff: %f uSize: %f stOff: %f stSize: %f supOff: %f subOff: %f\n",
           mMetrics.underlineOffset, mMetrics.underlineSize, mMetrics.strikeoutOffset, mMetrics.strikeoutSize,
           mMetrics.superscriptOffset, mMetrics.subscriptOffset);
#endif
}

void
gfxGDIFont::FillLogFont(LOGFONTW& aLogFont, gfxFloat aSize)
{
    GDIFontEntry *fe = static_cast<GDIFontEntry*>(GetFontEntry());

    PRUint16 weight = mNeedsBold ? 700 : fe->Weight();
    PRBool italic = (mStyle.style & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE));

    
    
    if (fe->mIsUserFont) {
        if (fe->IsItalic())
            italic = PR_FALSE; 
        if (fe->IsBold() || !mNeedsBold) {
            
            
            weight = 200; 
        }
    }

    fe->FillLogFont(&aLogFont, italic, weight, aSize, 
                    (mAntialiasOption == kAntialiasSubpixel) ? PR_TRUE : PR_FALSE);
}

PRInt32
gfxGDIFont::GetGlyphWidth(gfxContext *aCtx, PRUint16 aGID)
{
    if (!mGlyphWidths.IsInitialized()) {
        mGlyphWidths.Init(200);
    }

    PRInt32 width;
    if (mGlyphWidths.Get(aGID, &width)) {
        return width;
    }

    DCFromContext dc(aCtx);
    AutoSelectFont fs(dc, GetHFONT());

    int devWidth;
    if (GetCharWidthI(dc, aGID, 1, NULL, &devWidth)) {
        
        width = (devWidth & 0x7fff) << 16;
        mGlyphWidths.Put(aGID, width);
        return width;
    }

    return -1;
}
