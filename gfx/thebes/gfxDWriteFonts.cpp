




































#include "gfxDWriteFonts.h"
#include "gfxDWriteShaper.h"
#include "gfxHarfBuzzShaper.h"
#include "gfxDWriteFontList.h"
#include "gfxContext.h"
#include <dwrite.h>

#include "gfxDWriteTextAnalysis.h"

#include "harfbuzz/hb-blob.h"


#define OBLIQUE_SKEW_FACTOR 0.3




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




#ifndef SPI_GETFONTSMOOTHINGTYPE
#define SPI_GETFONTSMOOTHINGTYPE 0x200a
#endif
#ifndef FE_FONTSMOOTHINGCLEARTYPE
#define FE_FONTSMOOTHINGCLEARTYPE 2
#endif

static bool
HasClearType()
{
    OSVERSIONINFO versionInfo;
    versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    return (GetVersionEx(&versionInfo) &&
            (versionInfo.dwMajorVersion > 5 ||
             (versionInfo.dwMajorVersion == 5 &&
              versionInfo.dwMinorVersion >= 1))); 
}

static bool
UsingClearType()
{
    BOOL fontSmoothing;
    if (!SystemParametersInfo(SPI_GETFONTSMOOTHING, 0, &fontSmoothing, 0) ||
        !fontSmoothing)
    {
        return false;    
    }

    if (!HasClearType()) {
        return false;
    }

    UINT type;
    if (SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE, 0, &type, 0) &&
        type == FE_FONTSMOOTHINGCLEARTYPE)
    {
        return true;
    }
    return false;
}



gfxDWriteFont::gfxDWriteFont(gfxFontEntry *aFontEntry,
                             const gfxFontStyle *aFontStyle,
                             PRBool aNeedsBold,
                             AntialiasOption anAAOption)
    : gfxFont(aFontEntry, aFontStyle, anAAOption)
    , mCairoFontFace(nsnull)
    , mCairoScaledFont(nsnull)
    , mMetrics(nsnull)
    , mNeedsOblique(PR_FALSE)
    , mNeedsBold(aNeedsBold)
    , mUseSubpixelPositions(PR_FALSE)
    , mAllowManualShowGlyphs(PR_TRUE)
{
    gfxDWriteFontEntry *fe =
        static_cast<gfxDWriteFontEntry*>(aFontEntry);
    nsresult rv;
    DWRITE_FONT_SIMULATIONS sims = DWRITE_FONT_SIMULATIONS_NONE;
    if ((GetStyle()->style & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE)) &&
        !fe->IsItalic()) {
            
            
            mNeedsOblique = PR_TRUE;
    }
    if (aNeedsBold) {
        sims |= DWRITE_FONT_SIMULATIONS_BOLD;
    }

    rv = fe->CreateFontFace(getter_AddRefs(mFontFace), sims);

    if (NS_FAILED(rv)) {
        mIsValid = PR_FALSE;
        return;
    }

    if ((anAAOption == gfxFont::kAntialiasDefault &&
         UsingClearType() &&
         GetMeasuringMode() == DWRITE_MEASURING_MODE_NATURAL) ||
        anAAOption == gfxFont::kAntialiasSubpixel)
    {
        mUseSubpixelPositions = PR_TRUE;
        
        
    }

    ComputeMetrics();

    if (FontCanSupportHarfBuzz()) {
        mHarfBuzzShaper = new gfxHarfBuzzShaper(this);
    }
}

gfxDWriteFont::~gfxDWriteFont()
{
    if (mCairoFontFace) {
        cairo_font_face_destroy(mCairoFontFace);
    }
    if (mCairoScaledFont) {
        cairo_scaled_font_destroy(mCairoScaledFont);
    }
    delete mMetrics;
}

gfxFont*
gfxDWriteFont::CopyWithAntialiasOption(AntialiasOption anAAOption)
{
    return new gfxDWriteFont(static_cast<gfxDWriteFontEntry*>(mFontEntry.get()),
                             &mStyle, mNeedsBold, anAAOption);
}

void
gfxDWriteFont::CreatePlatformShaper()
{
    mPlatformShaper = new gfxDWriteShaper(this);
}

const gfxFont::Metrics&
gfxDWriteFont::GetMetrics()
{
    return *mMetrics;
}

PRBool
gfxDWriteFont::GetFakeMetricsForArialBlack(DWRITE_FONT_METRICS *aFontMetrics)
{
    gfxFontStyle style(mStyle);
    style.weight = 700;
    PRBool needsBold;
    gfxFontEntry *fe = mFontEntry->Family()->FindFontForStyle(style, needsBold);
    if (!fe || fe == mFontEntry) {
        return PR_FALSE;
    }

    nsRefPtr<gfxFont> font = fe->FindOrMakeFont(&style, needsBold);
    gfxDWriteFont *dwFont = static_cast<gfxDWriteFont*>(font.get());
    dwFont->mFontFace->GetMetrics(aFontMetrics);

    return PR_TRUE;
}

void
gfxDWriteFont::ComputeMetrics()
{
    DWRITE_FONT_METRICS fontMetrics;
    if (!(mFontEntry->Weight() == 900 &&
          !mFontEntry->IsUserFont() &&
          mFontEntry->FamilyName().EqualsLiteral("Arial") &&
          GetFakeMetricsForArialBlack(&fontMetrics)))
    {
        mFontFace->GetMetrics(&fontMetrics);
    }

    if (mStyle.sizeAdjust != 0.0) {
        gfxFloat aspect = (gfxFloat)fontMetrics.xHeight /
                   fontMetrics.designUnitsPerEm;
        mAdjustedSize = mStyle.GetAdjustedSize(aspect);
    } else {
        mAdjustedSize = mStyle.size;
    }

    gfxDWriteFontEntry *fe =
        static_cast<gfxDWriteFontEntry*>(mFontEntry.get());
    if (fe->IsCJKFont() && HasBitmapStrikeForSize(NS_lround(mAdjustedSize))) {
        mAdjustedSize = NS_lround(mAdjustedSize);
        mUseSubpixelPositions = PR_FALSE;
        
        
        
        
        
        mAllowManualShowGlyphs = PR_FALSE;
    }

    mMetrics = new gfxFont::Metrics;
    ::memset(mMetrics, 0, sizeof(*mMetrics));

    mFUnitsConvFactor = float(mAdjustedSize / fontMetrics.designUnitsPerEm);

    mMetrics->xHeight = fontMetrics.xHeight * mFUnitsConvFactor;

    mMetrics->maxAscent = ceil(fontMetrics.ascent * mFUnitsConvFactor);
    mMetrics->maxDescent = ceil(fontMetrics.descent * mFUnitsConvFactor);
    mMetrics->maxHeight = mMetrics->maxAscent + mMetrics->maxDescent;

    mMetrics->emHeight = mAdjustedSize;
    mMetrics->emAscent = mMetrics->emHeight *
        mMetrics->maxAscent / mMetrics->maxHeight;
    mMetrics->emDescent = mMetrics->emHeight - mMetrics->emAscent;

    mMetrics->maxAdvance = mAdjustedSize;

    
    PRUint8 *tableData;
    PRUint32 len;
    void *tableContext = NULL;
    BOOL exists;
    HRESULT hr =
        mFontFace->TryGetFontTable(DWRITE_MAKE_OPENTYPE_TAG('h', 'h', 'e', 'a'),
                                   (const void**)&tableData,
                                   &len,
                                   &tableContext,
                                   &exists);
    if (SUCCEEDED(hr)) {
        if (exists && len >= sizeof(mozilla::HheaTable)) {
            const mozilla::HheaTable* hhea =
                reinterpret_cast<const mozilla::HheaTable*>(tableData);
            mMetrics->maxAdvance =
                PRUint16(hhea->advanceWidthMax) * mFUnitsConvFactor;
        }
        mFontFace->ReleaseFontTable(tableContext);
    }

    mMetrics->internalLeading = NS_MAX(mMetrics->maxHeight - mMetrics->emHeight, 0.0);
    mMetrics->externalLeading = ceil(fontMetrics.lineGap * mFUnitsConvFactor);

    UINT16 glyph = (PRUint16)GetSpaceGlyph();
    mMetrics->spaceWidth = MeasureGlyphWidth(glyph);

    
    
    if (mUseSubpixelPositions) {
        mMetrics->aveCharWidth = 0;
        hr = mFontFace->TryGetFontTable(DWRITE_MAKE_OPENTYPE_TAG('O', 'S', '/', '2'),
                                        (const void**)&tableData,
                                        &len,
                                        &tableContext,
                                        &exists);
        if (SUCCEEDED(hr)) {
            if (exists && len >= 4) {
                
                
                
                const mozilla::OS2Table* os2 =
                    reinterpret_cast<const mozilla::OS2Table*>(tableData);
                mMetrics->aveCharWidth =
                    PRInt16(os2->xAvgCharWidth) * mFUnitsConvFactor;
            }
            mFontFace->ReleaseFontTable(tableContext);
        }
    }

    UINT32 ucs;
    if (mMetrics->aveCharWidth < 1) {
        ucs = L'x';
        if (SUCCEEDED(mFontFace->GetGlyphIndicesA(&ucs, 1, &glyph))) {
            mMetrics->aveCharWidth = MeasureGlyphWidth(glyph);
        }
        if (mMetrics->aveCharWidth < 1) {
            
            mMetrics->aveCharWidth = fontMetrics.xHeight * mFUnitsConvFactor;
        }
    }

    ucs = L'0';
    if (SUCCEEDED(mFontFace->GetGlyphIndicesA(&ucs, 1, &glyph))) {
        mMetrics->zeroOrAveCharWidth = MeasureGlyphWidth(glyph);
    }
    if (mMetrics->zeroOrAveCharWidth < 1) {
        mMetrics->zeroOrAveCharWidth = mMetrics->aveCharWidth;
    }

    mMetrics->underlineOffset =
        fontMetrics.underlinePosition * mFUnitsConvFactor;
    mMetrics->underlineSize = 
        fontMetrics.underlineThickness * mFUnitsConvFactor;
    mMetrics->strikeoutOffset =
        fontMetrics.strikethroughPosition * mFUnitsConvFactor;
    mMetrics->strikeoutSize =
        fontMetrics.strikethroughThickness * mFUnitsConvFactor;
    mMetrics->superscriptOffset = 0;
    mMetrics->subscriptOffset = 0;

    SanitizeMetrics(mMetrics, GetFontEntry()->mIsBadUnderlineFont);

#if 0
    printf("Font: %p (%s) size: %f\n", this,
           NS_ConvertUTF16toUTF8(GetName()).get(), mStyle.size);
    printf("    emHeight: %f emAscent: %f emDescent: %f\n", mMetrics->emHeight, mMetrics->emAscent, mMetrics->emDescent);
    printf("    maxAscent: %f maxDescent: %f maxAdvance: %f\n", mMetrics->maxAscent, mMetrics->maxDescent, mMetrics->maxAdvance);
    printf("    internalLeading: %f externalLeading: %f\n", mMetrics->internalLeading, mMetrics->externalLeading);
    printf("    spaceWidth: %f aveCharWidth: %f zeroOrAve: %f xHeight: %f\n",
           mMetrics->spaceWidth, mMetrics->aveCharWidth, mMetrics->zeroOrAveCharWidth, mMetrics->xHeight);
    printf("    uOff: %f uSize: %f stOff: %f stSize: %f supOff: %f subOff: %f\n",
           mMetrics->underlineOffset, mMetrics->underlineSize, mMetrics->strikeoutOffset, mMetrics->strikeoutSize,
           mMetrics->superscriptOffset, mMetrics->subscriptOffset);
#endif
}

using namespace mozilla; 

struct EBLCHeader {
    AutoSwap_PRUint32 version;
    AutoSwap_PRUint32 numSizes;
};

struct SbitLineMetrics {
    PRInt8  ascender;
    PRInt8  descender;
    PRUint8 widthMax;
    PRInt8  caretSlopeNumerator;
    PRInt8  caretSlopeDenominator;
    PRInt8  caretOffset;
    PRInt8  minOriginSB;
    PRInt8  minAdvanceSB;
    PRInt8  maxBeforeBL;
    PRInt8  minAfterBL;
    PRInt8  pad1;
    PRInt8  pad2;
};

struct BitmapSizeTable {
    AutoSwap_PRUint32 indexSubTableArrayOffset;
    AutoSwap_PRUint32 indexTablesSize;
    AutoSwap_PRUint32 numberOfIndexSubTables;
    AutoSwap_PRUint32 colorRef;
    SbitLineMetrics   hori;
    SbitLineMetrics   vert;
    AutoSwap_PRUint16 startGlyphIndex;
    AutoSwap_PRUint16 endGlyphIndex;
    PRUint8           ppemX;
    PRUint8           ppemY;
    PRUint8           bitDepth;
    PRUint8           flags;
};

typedef EBLCHeader EBSCHeader;

struct BitmapScaleTable {
    SbitLineMetrics   hori;
    SbitLineMetrics   vert;
    PRUint8           ppemX;
    PRUint8           ppemY;
    PRUint8           substitutePpemX;
    PRUint8           substitutePpemY;
};

PRBool
gfxDWriteFont::HasBitmapStrikeForSize(PRUint32 aSize)
{
    PRUint8 *tableData;
    PRUint32 len;
    void *tableContext;
    BOOL exists;
    HRESULT hr =
        mFontFace->TryGetFontTable(DWRITE_MAKE_OPENTYPE_TAG('E', 'B', 'L', 'C'),
                                   (const void**)&tableData, &len,
                                   &tableContext, &exists);
    if (FAILED(hr)) {
        return PR_FALSE;
    }

    PRBool hasStrike = PR_FALSE;
    
    
    
    while (exists) {
        if (len < sizeof(EBLCHeader)) {
            break;
        }
        const EBLCHeader *hdr = reinterpret_cast<const EBLCHeader*>(tableData);
        if (hdr->version != 0x00020000) {
            break;
        }
        PRUint32 numSizes = hdr->numSizes;
        if (numSizes > 0xffff) { 
            break;
        }
        if (len < sizeof(EBLCHeader) + numSizes * sizeof(BitmapSizeTable)) {
            break;
        }
        const BitmapSizeTable *sizeTable =
            reinterpret_cast<const BitmapSizeTable*>(hdr + 1);
        for (PRUint32 i = 0; i < numSizes; ++i, ++sizeTable) {
            if (sizeTable->ppemX == aSize && sizeTable->ppemY == aSize) {
                
                
                
                
                hasStrike = (PRUint16(sizeTable->endGlyphIndex) >=
                             PRUint16(sizeTable->startGlyphIndex) + 3);
                break;
            }
        }
        
        
        break;
    }
    mFontFace->ReleaseFontTable(tableContext);

    if (hasStrike) {
        return PR_TRUE;
    }

    
    
    hr = mFontFace->TryGetFontTable(DWRITE_MAKE_OPENTYPE_TAG('E', 'B', 'S', 'C'),
                                    (const void**)&tableData, &len,
                                    &tableContext, &exists);
    if (FAILED(hr)) {
        return PR_FALSE;
    }

    while (exists) {
        if (len < sizeof(EBSCHeader)) {
            break;
        }
        const EBSCHeader *hdr = reinterpret_cast<const EBSCHeader*>(tableData);
        if (hdr->version != 0x00020000) {
            break;
        }
        PRUint32 numSizes = hdr->numSizes;
        if (numSizes > 0xffff) {
            break;
        }
        if (len < sizeof(EBSCHeader) + numSizes * sizeof(BitmapScaleTable)) {
            break;
        }
        const BitmapScaleTable *scaleTable =
            reinterpret_cast<const BitmapScaleTable*>(hdr + 1);
        for (PRUint32 i = 0; i < numSizes; ++i, ++scaleTable) {
            if (scaleTable->ppemX == aSize && scaleTable->ppemY == aSize) {
                hasStrike = PR_TRUE;
                break;
            }
        }
        break;
    }
    mFontFace->ReleaseFontTable(tableContext);

    return hasStrike;
}

PRUint32
gfxDWriteFont::GetSpaceGlyph()
{
    UINT32 ucs = L' ';
    UINT16 glyph;
    HRESULT hr;
    hr = mFontFace->GetGlyphIndicesA(&ucs, 1, &glyph);
    if (FAILED(hr)) {
        return 0;
    }
    return glyph;
}

PRBool
gfxDWriteFont::SetupCairoFont(gfxContext *aContext)
{
    cairo_scaled_font_t *scaledFont = CairoScaledFont();
    if (cairo_scaled_font_status(scaledFont) != CAIRO_STATUS_SUCCESS) {
        
        
        return PR_FALSE;
    }
    cairo_set_scaled_font(aContext->GetCairo(), scaledFont);
    return PR_TRUE;
}

PRBool
gfxDWriteFont::IsValid()
{
    return mFontFace != NULL;
}

IDWriteFontFace*
gfxDWriteFont::GetFontFace()
{
    return  mFontFace.get();
}

cairo_font_face_t *
gfxDWriteFont::CairoFontFace()
{
    if (!mCairoFontFace) {
#ifdef CAIRO_HAS_DWRITE_FONT
        mCairoFontFace = 
            cairo_dwrite_font_face_create_for_dwrite_fontface(
            ((gfxDWriteFontEntry*)mFontEntry.get())->mFont, mFontFace);
#endif
    }
    return mCairoFontFace;
}


cairo_scaled_font_t *
gfxDWriteFont::CairoScaledFont()
{
    if (!mCairoScaledFont) {
        cairo_matrix_t sizeMatrix;
        cairo_matrix_t identityMatrix;

        cairo_matrix_init_scale(&sizeMatrix, mAdjustedSize, mAdjustedSize);
        cairo_matrix_init_identity(&identityMatrix);

        cairo_font_options_t *fontOptions = cairo_font_options_create();
        if (mNeedsOblique) {
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

        if (mAntialiasOption != kAntialiasDefault) {
            cairo_font_options_set_antialias(fontOptions,
                GetCairoAntialiasOption(mAntialiasOption));
        }

        mCairoScaledFont = cairo_scaled_font_create(CairoFontFace(),
                                                    &sizeMatrix,
                                                    &identityMatrix,
                                                    fontOptions);
        cairo_font_options_destroy(fontOptions);

        cairo_dwrite_scaled_font_allow_manual_show_glyphs(mCairoScaledFont,
                                                          mAllowManualShowGlyphs);

        gfxDWriteFontEntry *fe =
            static_cast<gfxDWriteFontEntry*>(mFontEntry.get());
        cairo_dwrite_scaled_font_set_force_GDI_classic(mCairoScaledFont,
                                                       fe->GetForceGDIClassic());
    }

    NS_ASSERTION(mAdjustedSize == 0.0 ||
                 cairo_scaled_font_status(mCairoScaledFont) 
                   == CAIRO_STATUS_SUCCESS,
                 "Failed to make scaled font");

    return mCairoScaledFont;
}





class FontTableRec {
public:
    FontTableRec(IDWriteFontFace *aFontFace, void *aContext)
        : mFontFace(aFontFace), mContext(aContext)
    { }

    ~FontTableRec() {
        mFontFace->ReleaseFontTable(mContext);
    }

private:
    IDWriteFontFace *mFontFace;
    void            *mContext;
};

 void
gfxDWriteFont::DestroyBlobFunc(void* aUserData)
{
    FontTableRec *ftr = static_cast<FontTableRec*>(aUserData);
    delete ftr;
}

hb_blob_t *
gfxDWriteFont::GetFontTable(PRUint32 aTag)
{
    const void *data;
    UINT32      size;
    void       *context;
    BOOL        exists;
    HRESULT hr = mFontFace->TryGetFontTable(NS_SWAP32(aTag),
                                            &data, &size, &context, &exists);
    if (SUCCEEDED(hr) && exists) {
        FontTableRec *ftr = new FontTableRec(mFontFace, context);
        return hb_blob_create(static_cast<const char*>(data), size,
                              HB_MEMORY_MODE_READONLY,
                              DestroyBlobFunc, ftr);
    }

    if (mFontEntry->IsUserFont() && !mFontEntry->IsLocalUserFont()) {
        
        
        hb_blob_t *blob;
        if (mFontEntry->GetExistingFontTable(aTag, &blob)) {
            return blob;
        }
    }

    return nsnull;
}

PRBool
gfxDWriteFont::ProvidesGlyphWidths()
{
    return !mUseSubpixelPositions ||
           (mFontFace->GetSimulations() & DWRITE_FONT_SIMULATIONS_BOLD);
}

PRInt32
gfxDWriteFont::GetGlyphWidth(gfxContext *aCtx, PRUint16 aGID)
{
    if (!mGlyphWidths.IsInitialized()) {
        mGlyphWidths.Init(200);
    }

    PRInt32 width = -1;
    if (mGlyphWidths.Get(aGID, &width)) {
        return width;
    }

    width = NS_lround(MeasureGlyphWidth(aGID) * 65536.0);
    mGlyphWidths.Put(aGID, width);
    return width;
}

DWRITE_MEASURING_MODE
gfxDWriteFont::GetMeasuringMode()
{
    return static_cast<gfxDWriteFontEntry*>(mFontEntry.get())->GetForceGDIClassic()
        ? DWRITE_MEASURING_MODE_GDI_CLASSIC
        : gfxWindowsPlatform::GetPlatform()->DWriteMeasuringMode();
}

gfxFloat
gfxDWriteFont::MeasureGlyphWidth(PRUint16 aGlyph)
{
    DWRITE_GLYPH_METRICS metrics;
    HRESULT hr;
    if (mUseSubpixelPositions) {
        hr = mFontFace->GetDesignGlyphMetrics(&aGlyph, 1, &metrics, FALSE);
        if (SUCCEEDED(hr)) {
            return metrics.advanceWidth * mFUnitsConvFactor;
        }
    } else {
        hr = mFontFace->GetGdiCompatibleGlyphMetrics(
                  FLOAT(mAdjustedSize), 1.0f, nsnull,
                  GetMeasuringMode() == DWRITE_MEASURING_MODE_GDI_NATURAL,
                  &aGlyph, 1, &metrics, FALSE);
        if (SUCCEEDED(hr)) {
            return NS_lround(metrics.advanceWidth * mFUnitsConvFactor);
        }
    }
    return 0;
}
