




#include "gfxMacFont.h"
#include "gfxCoreTextShaper.h"
#include "gfxHarfBuzzShaper.h"
#include <algorithm>
#include "gfxGraphiteShaper.h"
#include "gfxPlatformMac.h"
#include "gfxContext.h"
#include "gfxFontUtils.h"

#include "cairo-quartz.h"

using namespace mozilla;
using namespace mozilla::gfx;

gfxMacFont::gfxMacFont(MacOSFontEntry *aFontEntry, const gfxFontStyle *aFontStyle,
                       bool aNeedsBold)
    : gfxFont(aFontEntry, aFontStyle),
      mCGFont(nullptr),
      mFontFace(nullptr)
{
    mApplySyntheticBold = aNeedsBold;

    mCGFont = aFontEntry->GetFontRef();
    if (!mCGFont) {
        mIsValid = false;
        return;
    }

    
    InitMetrics();
    if (!mIsValid) {
        return;
    }

    mFontFace = cairo_quartz_font_face_create_for_cgfont(mCGFont);

    cairo_status_t cairoerr = cairo_font_face_status(mFontFace);
    if (cairoerr != CAIRO_STATUS_SUCCESS) {
        mIsValid = false;
#ifdef DEBUG
        char warnBuf[1024];
        sprintf(warnBuf, "Failed to create Cairo font face: %s status: %d",
                NS_ConvertUTF16toUTF8(GetName()).get(), cairoerr);
        NS_WARNING(warnBuf);
#endif
        return;
    }

    cairo_matrix_t sizeMatrix, ctm;
    cairo_matrix_init_identity(&ctm);
    cairo_matrix_init_scale(&sizeMatrix, mAdjustedSize, mAdjustedSize);

    
    bool needsOblique =
        (mFontEntry != NULL) &&
        (!mFontEntry->IsItalic() &&
         (mStyle.style & (NS_FONT_STYLE_ITALIC | NS_FONT_STYLE_OBLIQUE)));

    if (needsOblique) {
        double skewfactor = (needsOblique ? Fix2X(kATSItalicQDSkew) : 0);

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

    
    if (mAdjustedSize <=
        (gfxFloat)gfxPlatformMac::GetPlatform()->GetAntiAliasingThreshold()) {
        cairo_font_options_set_antialias(fontOptions, CAIRO_ANTIALIAS_NONE);
    }

    mScaledFont = cairo_scaled_font_create(mFontFace, &sizeMatrix, &ctm,
                                           fontOptions);
    cairo_font_options_destroy(fontOptions);

    cairoerr = cairo_scaled_font_status(mScaledFont);
    if (cairoerr != CAIRO_STATUS_SUCCESS) {
        mIsValid = false;
#ifdef DEBUG
        char warnBuf[1024];
        sprintf(warnBuf, "Failed to create scaled font: %s status: %d",
                NS_ConvertUTF16toUTF8(GetName()).get(), cairoerr);
        NS_WARNING(warnBuf);
#endif
    }

    if (FontCanSupportGraphite()) {
        mGraphiteShaper = new gfxGraphiteShaper(this);
    }
    if (FontCanSupportHarfBuzz()) {
        mHarfBuzzShaper = new gfxHarfBuzzShaper(this);
    }
}

gfxMacFont::~gfxMacFont()
{
    if (mScaledFont) {
        cairo_scaled_font_destroy(mScaledFont);
    }
    if (mFontFace) {
        cairo_font_face_destroy(mFontFace);
    }
}

bool
gfxMacFont::ShapeText(gfxContext      *aContext,
                      const PRUnichar *aText,
                      uint32_t         aOffset,
                      uint32_t         aLength,
                      int32_t          aScript,
                      gfxShapedText   *aShapedText,
                      bool             aPreferPlatformShaping)
{
    if (!mIsValid) {
        NS_WARNING("invalid font! expect incorrect text rendering");
        return false;
    }

    bool requiresAAT =
        static_cast<MacOSFontEntry*>(GetFontEntry())->RequiresAATLayout();
    return gfxFont::ShapeText(aContext, aText, aOffset, aLength,
                              aScript, aShapedText, requiresAAT);
}

void
gfxMacFont::CreatePlatformShaper()
{
    mPlatformShaper = new gfxCoreTextShaper(this);
}

bool
gfxMacFont::SetupCairoFont(gfxContext *aContext)
{
    if (cairo_scaled_font_status(mScaledFont) != CAIRO_STATUS_SUCCESS) {
        
        
        return false;
    }
    cairo_set_scaled_font(aContext->GetCairo(), mScaledFont);
    return true;
}

gfxFont::RunMetrics
gfxMacFont::Measure(gfxTextRun *aTextRun,
                    uint32_t aStart, uint32_t aEnd,
                    BoundingBoxType aBoundingBoxType,
                    gfxContext *aRefContext,
                    Spacing *aSpacing)
{
    gfxFont::RunMetrics metrics =
        gfxFont::Measure(aTextRun, aStart, aEnd,
                         aBoundingBoxType, aRefContext, aSpacing);

    
    
    if (aBoundingBoxType != TIGHT_HINTED_OUTLINE_EXTENTS &&
        metrics.mBoundingBox.width > 0) {
        metrics.mBoundingBox.x -= aTextRun->GetAppUnitsPerDevUnit();
        metrics.mBoundingBox.width += aTextRun->GetAppUnitsPerDevUnit() * 2;
    }

    return metrics;
}

void
gfxMacFont::InitMetrics()
{
    mIsValid = false;
    ::memset(&mMetrics, 0, sizeof(mMetrics));

    uint32_t upem = 0;

    
    
    
    
    
    const uint32_t kHeadTableTag = TRUETYPE_TAG('h','e','a','d');
    AutoFallibleTArray<uint8_t,sizeof(HeadTable)> headData;
    if (NS_SUCCEEDED(mFontEntry->GetFontTable(kHeadTableTag, headData)) &&
        headData.Length() >= sizeof(HeadTable)) {
        HeadTable *head = reinterpret_cast<HeadTable*>(headData.Elements());
        upem = head->unitsPerEm;
    } else {
        upem = ::CGFontGetUnitsPerEm(mCGFont);
    }

    if (upem < 16 || upem > 16384) {
        
#ifdef DEBUG
        char warnBuf[1024];
        sprintf(warnBuf, "Bad font metrics for: %s (invalid unitsPerEm value)",
                NS_ConvertUTF16toUTF8(mFontEntry->Name()).get());
        NS_WARNING(warnBuf);
#endif
        return;
    }

    mAdjustedSize = std::max(mStyle.size, 1.0);
    mFUnitsConvFactor = mAdjustedSize / upem;

    
    
    
    gfxFloat cgConvFactor;
    if (static_cast<MacOSFontEntry*>(mFontEntry.get())->IsCFF()) {
        cgConvFactor = mAdjustedSize / ::CGFontGetUnitsPerEm(mCGFont);
    } else {
        cgConvFactor = mFUnitsConvFactor;
    }

    
    
    if (!InitMetricsFromSfntTables(mMetrics) &&
        (!mFontEntry->IsUserFont() || mFontEntry->IsLocalUserFont())) {
        InitMetricsFromPlatform();
    }
    if (!mIsValid) {
        return;
    }

    if (mMetrics.xHeight == 0.0) {
        mMetrics.xHeight = ::CGFontGetXHeight(mCGFont) * cgConvFactor;
    }

    if (mStyle.sizeAdjust != 0.0 && mStyle.size > 0.0 &&
        mMetrics.xHeight > 0.0) {
        
        gfxFloat aspect = mMetrics.xHeight / mStyle.size;
        mAdjustedSize = mStyle.GetAdjustedSize(aspect);
        mFUnitsConvFactor = mAdjustedSize / upem;
        if (static_cast<MacOSFontEntry*>(mFontEntry.get())->IsCFF()) {
            cgConvFactor = mAdjustedSize / ::CGFontGetUnitsPerEm(mCGFont);
        } else {
            cgConvFactor = mFUnitsConvFactor;
        }
        mMetrics.xHeight = 0.0;
        if (!InitMetricsFromSfntTables(mMetrics) &&
            (!mFontEntry->IsUserFont() || mFontEntry->IsLocalUserFont())) {
            InitMetricsFromPlatform();
        }
        if (!mIsValid) {
            
            
            return;
        }
        if (mMetrics.xHeight == 0.0) {
            mMetrics.xHeight = ::CGFontGetXHeight(mCGFont) * cgConvFactor;
        }
    }

    
    
    

    mMetrics.emHeight = mAdjustedSize;

    
    

    CFDataRef cmap =
        ::CGFontCopyTableForTag(mCGFont, TRUETYPE_TAG('c','m','a','p'));

    uint32_t glyphID;
    if (mMetrics.aveCharWidth <= 0) {
        mMetrics.aveCharWidth = GetCharWidth(cmap, 'x', &glyphID,
                                             cgConvFactor);
        if (glyphID == 0) {
            
            mMetrics.aveCharWidth = mMetrics.maxAdvance;
        }
    }
    if (IsSyntheticBold()) {
        mMetrics.aveCharWidth += GetSyntheticBoldOffset();
        mMetrics.maxAdvance += GetSyntheticBoldOffset();
    }

    mMetrics.spaceWidth = GetCharWidth(cmap, ' ', &glyphID, cgConvFactor);
    if (glyphID == 0) {
        
        mMetrics.spaceWidth = mMetrics.aveCharWidth;
    }
    mSpaceGlyph = glyphID;

    mMetrics.zeroOrAveCharWidth = GetCharWidth(cmap, '0', &glyphID,
                                               cgConvFactor);
    if (glyphID == 0) {
        mMetrics.zeroOrAveCharWidth = mMetrics.aveCharWidth;
    }

    if (cmap) {
        ::CFRelease(cmap);
    }

    CalculateDerivedMetrics(mMetrics);

    SanitizeMetrics(&mMetrics, mFontEntry->mIsBadUnderlineFont);

#if 0
    fprintf (stderr, "Font: %p (%s) size: %f\n", this,
             NS_ConvertUTF16toUTF8(GetName()).get(), mStyle.size);

    fprintf (stderr, "    emHeight: %f emAscent: %f emDescent: %f\n", mMetrics.emHeight, mMetrics.emAscent, mMetrics.emDescent);
    fprintf (stderr, "    maxAscent: %f maxDescent: %f maxAdvance: %f\n", mMetrics.maxAscent, mMetrics.maxDescent, mMetrics.maxAdvance);
    fprintf (stderr, "    internalLeading: %f externalLeading: %f\n", mMetrics.internalLeading, mMetrics.externalLeading);
    fprintf (stderr, "    spaceWidth: %f aveCharWidth: %f xHeight: %f\n", mMetrics.spaceWidth, mMetrics.aveCharWidth, mMetrics.xHeight);
    fprintf (stderr, "    uOff: %f uSize: %f stOff: %f stSize: %f supOff: %f subOff: %f\n", mMetrics.underlineOffset, mMetrics.underlineSize, mMetrics.strikeoutOffset, mMetrics.strikeoutSize, mMetrics.superscriptOffset, mMetrics.subscriptOffset);
#endif
}

gfxFloat
gfxMacFont::GetCharWidth(CFDataRef aCmap, PRUnichar aUniChar,
                         uint32_t *aGlyphID, gfxFloat aConvFactor)
{
    CGGlyph glyph = 0;
    
    if (aCmap) {
        glyph = gfxFontUtils::MapCharToGlyph(::CFDataGetBytePtr(aCmap),
                                             ::CFDataGetLength(aCmap),
                                             aUniChar);
    }

    if (aGlyphID) {
        *aGlyphID = glyph;
    }

    if (glyph) {
        int advance;
        if (::CGFontGetGlyphAdvances(mCGFont, &glyph, 1, &advance)) {
            return advance * aConvFactor;
        }
    }

    return 0;
}

 void
gfxMacFont::DestroyBlobFunc(void* aUserData)
{
    ::CFRelease((CFDataRef)aUserData);
}

hb_blob_t *
gfxMacFont::GetFontTable(uint32_t aTag)
{
    CFDataRef dataRef = ::CGFontCopyTableForTag(mCGFont, aTag);
    if (dataRef) {
        return hb_blob_create((const char*)::CFDataGetBytePtr(dataRef),
                              ::CFDataGetLength(dataRef),
                              HB_MEMORY_MODE_READONLY,
                              (void*)dataRef, DestroyBlobFunc);
    }

    if (mFontEntry->IsUserFont() && !mFontEntry->IsLocalUserFont()) {
        
        
        hb_blob_t *blob;
        if (mFontEntry->GetExistingFontTable(aTag, &blob)) {
            return blob;
        }
    }

    return nullptr;
}






void
gfxMacFont::InitMetricsFromPlatform()
{
    CTFontRef ctFont = ::CTFontCreateWithGraphicsFont(mCGFont,
                                                      mAdjustedSize,
                                                      NULL, NULL);
    if (!ctFont) {
        return;
    }

    mMetrics.underlineOffset = ::CTFontGetUnderlinePosition(ctFont);
    mMetrics.underlineSize = ::CTFontGetUnderlineThickness(ctFont);

    mMetrics.externalLeading = ::CTFontGetLeading(ctFont);

    mMetrics.maxAscent = ::CTFontGetAscent(ctFont);
    mMetrics.maxDescent = ::CTFontGetDescent(ctFont);

    
    
    
    CGRect r = ::CTFontGetBoundingBox(ctFont);
    mMetrics.maxAdvance = r.size.width;

    
    
    
    
    
    
    mMetrics.aveCharWidth = 0;

    mMetrics.xHeight = ::CTFontGetXHeight(ctFont);

    ::CFRelease(ctFont);

    mIsValid = true;
}

TemporaryRef<ScaledFont>
gfxMacFont::GetScaledFont(DrawTarget *aTarget)
{
  if (!mAzureScaledFont) {
    NativeFont nativeFont;
    nativeFont.mType = NATIVE_FONT_MAC_FONT_FACE;
    nativeFont.mFont = GetCGFontRef();
    mAzureScaledFont = mozilla::gfx::Factory::CreateScaledFontWithCairo(nativeFont, GetAdjustedSize(), mScaledFont);
  }

  return mAzureScaledFont;
}

void
gfxMacFont::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                FontCacheSizes*   aSizes) const
{
    gfxFont::SizeOfExcludingThis(aMallocSizeOf, aSizes);
    
    
}

void
gfxMacFont::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                FontCacheSizes*   aSizes) const
{
    aSizes->mFontInstances += aMallocSizeOf(this);
    SizeOfExcludingThis(aMallocSizeOf, aSizes);
}
