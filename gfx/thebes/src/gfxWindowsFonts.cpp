




































#ifdef DEBUG_smontagu
#define DEBUG_pavlov
#endif


#define FORCE_PR_LOG

#include "prtypes.h"
#include "gfxTypes.h"

#include "gfxContext.h"
#include "gfxWindowsFonts.h"
#include "gfxWindowsSurface.h"
#include "gfxWindowsPlatform.h"

#ifdef MOZ_ENABLE_GLITZ
#include "gfxGlitzSurface.h"
#endif

#include "gfxFontTest.h"

#include "cairo.h"
#include "cairo-win32.h"

#include <windows.h>

#include "nsUnicodeRange.h"
#include "nsUnicharUtils.h"

#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsServiceManagerUtils.h"

#include "nsCRT.h"

#include <math.h>

#include "prlog.h"
static PRLogModuleInfo *gFontLog = PR_NewLogModule("winfonts");

#define ROUND(x) floor((x) + 0.5)

inline HDC GetDCFromSurface(gfxASurface *aSurface) {
    if (aSurface->GetType() != gfxASurface::SurfaceTypeWin32) {
        NS_ERROR("GetDCFromSurface: Context target is not win32!");
        return nsnull;
    }
    return NS_STATIC_CAST(gfxWindowsSurface*, aSurface)->GetDC();
}







gfxWindowsFont::gfxWindowsFont(const nsAString& aName, const gfxFontStyle *aFontStyle)
    : gfxFont(aName, aFontStyle),
      mFont(nsnull), mAdjustedSize(0), mScriptCache(nsnull),
      mFontFace(nsnull), mScaledFont(nsnull),
      mMetrics(nsnull)
{
}

gfxWindowsFont::~gfxWindowsFont()
{
    Destroy();
}

void
gfxWindowsFont::Destroy()
{
    if (mFontFace)
        cairo_font_face_destroy(mFontFace);

    if (mScaledFont)
        cairo_scaled_font_destroy(mScaledFont);

    if (mFont)
        DeleteObject(mFont);

    ScriptFreeCache(&mScriptCache);

    delete mMetrics;

    mFont = nsnull;
    mScriptCache = nsnull;
    mFontFace = nsnull;
    mScaledFont = nsnull;
    mMetrics = nsnull;
}

const gfxFont::Metrics&
gfxWindowsFont::GetMetrics()
{
    if (!mMetrics)
        ComputeMetrics();

    return *mMetrics;
}

HFONT
gfxWindowsFont::GetHFONT()
{
    if (!mFont)
        mFont = MakeHFONT();

    NS_ASSERTION(mFont, "Failed to make HFONT");

    return mFont;
}

cairo_font_face_t *
gfxWindowsFont::CairoFontFace()
{
    if (!mFontFace)
        mFontFace = MakeCairoFontFace();

    NS_ASSERTION(mFontFace, "Failed to make font face");

    return mFontFace;
}

cairo_scaled_font_t *
gfxWindowsFont::CairoScaledFont()
{
    if (!mScaledFont)
        mScaledFont = MakeCairoScaledFont();

    NS_ASSERTION(mScaledFont, "Failed to make scaled font");

    return mScaledFont;
}

void
gfxWindowsFont::UpdateCTM(const gfxMatrix& aMatrix)
{
    if (aMatrix.yy == mCTM.yy &&
        aMatrix.xx == mCTM.xx &&
        aMatrix.xy == mCTM.xy &&
        aMatrix.yx == mCTM.yx)
        return;

    Destroy();

    mCTM = aMatrix;
}

HFONT
gfxWindowsFont::MakeHFONT()
{
    if (mFont)
        return mFont;

    if (!mWeightTable) {
        nsString name(mName);
        ToLowerCase(name);

        gfxWindowsPlatform *platform = gfxWindowsPlatform::GetPlatform();

        mWeightTable = platform->GetFontWeightTable(name);
        if (!mWeightTable) {
            mWeightTable = new WeightTable();
            platform->PutFontWeightTable(name, mWeightTable);
        }
    }

    PRInt8 baseWeight, weightDistance;
    GetStyle()->ComputeWeightAndOffset(&baseWeight, &weightDistance);

    HDC dc = nsnull;

    PRUint32 chosenWeight = 0;

    if (weightDistance >= 0) {

        for (PRUint8 i = baseWeight, k = 0; i < 10; i++) {
            if (mWeightTable->HasWeight(i)) {
                k++;
                chosenWeight = i * 100;
            } else if (mWeightTable->TriedWeight(i)) {
                continue;
            } else {
                const PRUint32 tryWeight = i * 100;

                if (!dc)
                    dc = GetDC((HWND)nsnull);

                FillLogFont(GetStyle()->size, tryWeight);
                mFont = CreateFontIndirectW(&mLogFont);
                HGDIOBJ oldFont = SelectObject(dc, mFont);
                TEXTMETRIC metrics;
                GetTextMetrics(dc, &metrics);

                PRBool hasWeight = (metrics.tmWeight == tryWeight);
                mWeightTable->SetWeight(i, hasWeight);
                if (hasWeight) {
                    chosenWeight = i * 100;
                    k++;
                }

                SelectObject(dc, oldFont);
                if (k <= weightDistance) {
                    DeleteObject(mFont);
                    mFont = nsnull;
                }
            }

            if (k > weightDistance) {
                chosenWeight = i * 100;
                break;
            }
        }

    } else if (weightDistance < 0) {
#ifdef DEBUG_pavlov
        printf("dont' support light/lighter yet\n");
#endif
        chosenWeight = baseWeight * 100;
    }

    if (chosenWeight == 0)
        chosenWeight = baseWeight * 100;

    mAdjustedSize = GetStyle()->size;
    if (GetStyle()->sizeAdjust > 0) {
        if (!mFont) {
            FillLogFont(mAdjustedSize, chosenWeight);
            mFont = CreateFontIndirectW(&mLogFont);
        }

        Metrics *oldMetrics = mMetrics;
        ComputeMetrics();
        gfxFloat aspect = mMetrics->xHeight / mMetrics->emHeight;
        mAdjustedSize =
            PR_MAX(ROUND(GetStyle()->size * (GetStyle()->sizeAdjust / aspect)), 1.0f);

        if (mMetrics != oldMetrics) {
            delete mMetrics;
            mMetrics = oldMetrics;
        }
        DeleteObject(mFont);
        mFont = nsnull;
    }

    if (!mFont) {
        FillLogFont(mAdjustedSize, chosenWeight);
        mFont = CreateFontIndirectW(&mLogFont);
    }

    if (dc)
        ReleaseDC((HWND)nsnull, dc);

    return mFont;
}

cairo_font_face_t *
gfxWindowsFont::MakeCairoFontFace()
{
    
    MakeHFONT();

    return cairo_win32_font_face_create_for_hfont(mFont);
}

cairo_scaled_font_t *
gfxWindowsFont::MakeCairoScaledFont()
{
    cairo_scaled_font_t *font = nsnull;

    cairo_matrix_t sizeMatrix;

    MakeHFONT(); 
    cairo_matrix_init_scale(&sizeMatrix, mAdjustedSize, mAdjustedSize);

    cairo_font_options_t *fontOptions = cairo_font_options_create();
    font = cairo_scaled_font_create(CairoFontFace(), &sizeMatrix,
                                    reinterpret_cast<cairo_matrix_t*>(&mCTM),
                                    fontOptions);
    cairo_font_options_destroy(fontOptions);

    return font;
}

void
gfxWindowsFont::ComputeMetrics()
{
    if (!mMetrics)
        mMetrics = new gfxFont::Metrics;

    HDC dc = GetDC((HWND)nsnull);

    HFONT font = GetHFONT();

    HGDIOBJ oldFont = SelectObject(dc, font);

    
    OUTLINETEXTMETRIC oMetrics;
    TEXTMETRIC& metrics = oMetrics.otmTextMetrics;

    if (0 < GetOutlineTextMetrics(dc, sizeof(oMetrics), &oMetrics)) {
        mMetrics->superscriptOffset = (double)oMetrics.otmptSuperscriptOffset.y;
        mMetrics->subscriptOffset = (double)oMetrics.otmptSubscriptOffset.y;
        mMetrics->strikeoutSize = PR_MAX(1, (double)oMetrics.otmsStrikeoutSize);
        mMetrics->strikeoutOffset = (double)oMetrics.otmsStrikeoutPosition;
        mMetrics->underlineSize = PR_MAX(1, (double)oMetrics.otmsUnderscoreSize);
        mMetrics->underlineOffset = (double)oMetrics.otmsUnderscorePosition;
        
        const MAT2 kIdentityMatrix = { {0, 1}, {0, 0}, {0, 0}, {0, 1} };
        GLYPHMETRICS gm;
        DWORD len = GetGlyphOutlineW(dc, PRUnichar('x'), GGO_METRICS, &gm, 0, nsnull, &kIdentityMatrix);
        if (len == GDI_ERROR || gm.gmptGlyphOrigin.y <= 0) {
            
            mMetrics->xHeight = ROUND((double)metrics.tmAscent * 0.56);
        } else {
            mMetrics->xHeight = gm.gmptGlyphOrigin.y;
        }
        
        
        
        
        if (mMetrics->superscriptOffset == 0 ||
            mMetrics->superscriptOffset >= metrics.tmAscent) {
            mMetrics->superscriptOffset = mMetrics->xHeight;
        }
        
        
        if (mMetrics->subscriptOffset == 0 ||
            mMetrics->subscriptOffset >= metrics.tmAscent) {
            mMetrics->subscriptOffset = mMetrics->xHeight;
        }
    } else {
        
        
        GetTextMetrics(dc, &metrics);

        mMetrics->xHeight = ROUND((float)metrics.tmAscent * 0.56f); 
        mMetrics->superscriptOffset = mMetrics->xHeight;
        mMetrics->subscriptOffset = mMetrics->xHeight;
        mMetrics->strikeoutSize = 1;
        mMetrics->strikeoutOffset = ROUND(mMetrics->xHeight / 2.0f); 
        mMetrics->underlineSize = 1;
        mMetrics->underlineOffset = -ROUND((float)metrics.tmDescent * 0.30f); 
    }

    mMetrics->internalLeading = metrics.tmInternalLeading;
    mMetrics->externalLeading = metrics.tmExternalLeading;
    mMetrics->emHeight = (metrics.tmHeight - metrics.tmInternalLeading);
    mMetrics->emAscent = (metrics.tmAscent - metrics.tmInternalLeading);
    mMetrics->emDescent = metrics.tmDescent;
    mMetrics->maxHeight = metrics.tmHeight;
    mMetrics->maxAscent = metrics.tmAscent;
    mMetrics->maxDescent = metrics.tmDescent;
    mMetrics->maxAdvance = metrics.tmMaxCharWidth;
    mMetrics->aveCharWidth = PR_MAX(1, metrics.tmAveCharWidth);

    
    SIZE size;
    GetTextExtentPoint32(dc, " ", 1, &size);
    mMetrics->spaceWidth = ROUND(size.cx);

    SelectObject(dc, oldFont);

    ReleaseDC((HWND)nsnull, dc);
}

void
gfxWindowsFont::FillLogFont(gfxFloat aSize, PRInt16 aWeight)
{
#define CLIP_TURNOFF_FONTASSOCIATION 0x40
    
    const double yScale = mCTM.yy;

    mLogFont.lfHeight = (LONG)-ROUND(aSize * yScale);

    if (mLogFont.lfHeight == 0)
        mLogFont.lfHeight = -1;

    
    mLogFont.lfWidth          = 0; 
    mLogFont.lfEscapement     = 0;
    mLogFont.lfOrientation    = 0;
    mLogFont.lfUnderline      = (GetStyle()->decorations & FONT_DECORATION_UNDERLINE) ? TRUE : FALSE;
    mLogFont.lfStrikeOut      = (GetStyle()->decorations & FONT_DECORATION_STRIKEOUT) ? TRUE : FALSE;
    mLogFont.lfCharSet        = DEFAULT_CHARSET;
#ifndef WINCE
    mLogFont.lfOutPrecision   = OUT_TT_PRECIS;
#else
    mLogFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
#endif
    mLogFont.lfClipPrecision  = CLIP_TURNOFF_FONTASSOCIATION;
    mLogFont.lfQuality        = DEFAULT_QUALITY;
    mLogFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    mLogFont.lfItalic         = (GetStyle()->style & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE)) ? TRUE : FALSE;
    mLogFont.lfWeight         = aWeight;

    int len = PR_MIN(mName.Length(), LF_FACESIZE - 1);
    memcpy(mLogFont.lfFaceName, nsPromiseFlatString(mName).get(), len * 2);
    mLogFont.lfFaceName[len] = '\0';
}


nsString
gfxWindowsFont::GetUniqueName()
{
    nsString uniqueName;

    
    MakeHFONT();

    
    uniqueName.Assign(mName);

    
    if (mLogFont.lfWeight != 400) {
        uniqueName.AppendLiteral(":");
        uniqueName.AppendInt((PRInt32)mLogFont.lfWeight);
    }

    
    if (mLogFont.lfItalic)
        uniqueName.AppendLiteral(":Italic");

    if (mLogFont.lfUnderline)
        uniqueName.AppendLiteral(":Underline");

    if (mLogFont.lfStrikeOut)
        uniqueName.AppendLiteral(":StrikeOut");

    return uniqueName;
}

void
gfxWindowsFont::Draw(gfxTextRun *aTextRun, PRUint32 aStart, PRUint32 aEnd,
                     gfxContext *aContext, PRBool aDrawToPath, gfxPoint *aBaselineOrigin,
                     Spacing *aSpacing)
{
    
    gfxFont::Draw(aTextRun, aStart, aEnd, aContext, aDrawToPath, aBaselineOrigin,
                  aSpacing);
}

void
gfxWindowsFont::SetupCairoFont(cairo_t *aCR)
{
    cairo_set_scaled_font(aCR, CairoScaledFont());
}












static already_AddRefed<gfxWindowsFont>
GetOrMakeFont(const nsAString& aName, const gfxFontStyle *aStyle)
{
    nsRefPtr<gfxFont> font = gfxFontCache::GetCache()->Lookup(aName, aStyle);
    if (!font) {
        font = new gfxWindowsFont(aName, aStyle);
        if (!font)
            return nsnull;
        gfxFontCache::GetCache()->AddNew(font);
    }
    gfxFont *f = nsnull;
    font.swap(f);
    return static_cast<gfxWindowsFont *>(f);
}

PRBool
gfxWindowsFontGroup::MakeFont(const nsAString& aName,
                              const nsACString& aGenericName,
                              void *closure)
{
    if (!aName.IsEmpty()) {
        gfxWindowsFontGroup *fg = NS_STATIC_CAST(gfxWindowsFontGroup*, closure);

        if (fg->HasFontNamed(aName))
            return PR_TRUE;

        nsRefPtr<gfxWindowsFont> font = GetOrMakeFont(aName, fg->GetStyle());
        if (font) {
            fg->AppendFont(font);
        }

        if (!aGenericName.IsEmpty() && fg->GetGenericFamily().IsEmpty())
            fg->mGenericFamily = aGenericName;
    }

    return PR_TRUE;
}


gfxWindowsFontGroup::gfxWindowsFontGroup(const nsAString& aFamilies, const gfxFontStyle *aStyle)
    : gfxFontGroup(aFamilies, aStyle)
{
    ForEachFont(MakeFont, this);

    if (mGenericFamily.IsEmpty())
        FindGenericFontFromStyle(MakeFont, this);

    if (mFonts.Length() == 0) {
        
        HGDIOBJ hGDI = ::GetStockObject(DEFAULT_GUI_FONT);
        LOGFONTW logFont;
        if (!hGDI ||
            !::GetObjectW(hGDI, sizeof(logFont), &logFont)) {
            NS_ERROR("Failed to create font group");
            return;
        }
        nsAutoString defaultFont(logFont.lfFaceName);
        MakeFont(defaultFont, mGenericFamily, this);
    }
}

gfxWindowsFontGroup::~gfxWindowsFontGroup()
{
}

gfxFontGroup *
gfxWindowsFontGroup::Copy(const gfxFontStyle *aStyle)
{
    return new gfxWindowsFontGroup(mFamilies, aStyle);
}

gfxTextRun *
gfxWindowsFontGroup::MakeTextRun(const PRUnichar *aString, PRUint32 aLength,
                                 Parameters *aParams)
{
    NS_ASSERTION(!(aParams->mFlags & TEXT_NEED_BOUNDING_BOX),
                 "Glyph extents not yet supported");

    gfxTextRun *textRun = new gfxTextRun(aParams, aLength);
    if (!textRun)
        return nsnull;

    textRun->RecordSurrogates(aString);
    
#ifdef FORCE_UNISCRIBE
    const PRBool isComplex = PR_TRUE;
#else
    const PRBool isComplex = ScriptIsComplex(aString, aLength, SIC_COMPLEX) == S_OK ||
                             textRun->IsRightToLeft();
#endif
    if (isComplex)
        InitTextRunUniscribe(aParams->mContext, textRun, aString, aLength);
    else
        InitTextRunGDI(aParams->mContext, textRun, aString, aLength);

    return textRun;
}

gfxTextRun *
gfxWindowsFontGroup::MakeTextRun(const PRUint8 *aString, PRUint32 aLength,
                                 Parameters *aParams)
{
    NS_ASSERTION((aParams->mFlags & TEXT_IS_ASCII || aParams->mFlags & TEXT_IS_8BIT),
                 "unknown text type");
    gfxTextRun *textRun = new gfxTextRun(aParams, aLength);
    if (!textRun)
        return nsnull;

#ifdef FORCE_UNISCRIBE
    const PRBool isComplex = PR_TRUE;
#else
    const PRBool isComplex = textRun->IsRightToLeft();
#endif

    if (isComplex) {
        nsDependentCSubstring cString(reinterpret_cast<const char*>(aString),
                                  reinterpret_cast<const char*>(aString + aLength));
        nsAutoString utf16;
        AppendASCIItoUTF16(cString, utf16);
        InitTextRunUniscribe(aParams->mContext, textRun, utf16.get(), aLength);
    } else {
        InitTextRunGDI(aParams->mContext, textRun,
                       reinterpret_cast<const char*>(aString), aLength);
    }

    return textRun;
}







static HDC
SetupContextFont(gfxContext *aContext, gfxWindowsFont *aFont)
{
    nsRefPtr<gfxASurface> surf = aContext->CurrentSurface();
    HDC dc = GetDCFromSurface(surf);
    if (!dc)
        return 0;

    
    aFont->UpdateCTM(gfxMatrix());
    HFONT hfont = aFont->GetHFONT();
    if (!hfont)
        return 0;
    SelectObject(dc, hfont);

    

    TEXTMETRIC metrics;
    GetTextMetrics(dc, &metrics);
    if ((metrics.tmPitchAndFamily & (TMPF_TRUETYPE)) == 0)
        return 0;

    return dc;
}

static PRBool
IsAnyGlyphMissing(WCHAR *aGlyphs, PRUint32 aLength)
{
    PRUint32 i;
    for (i = 0; i < aLength; ++i) {
        if (aGlyphs[i] == 0xFFFF)
            return PR_TRUE;
    }
    return PR_FALSE;
}

static PRBool
SetupTextRunFromGlyphs(gfxTextRun *aRun, WCHAR *aGlyphs, HDC aDC,
                       gfxWindowsFont *aFont)
{
    PRUint32 length = aRun->GetLength();
    if (IsAnyGlyphMissing(aGlyphs, length))
        return PR_FALSE;
       
    SIZE size;
    nsAutoTArray<int,500> partialWidthArray;
    if (!partialWidthArray.AppendElements(length))
        return PR_FALSE;
    BOOL success = GetTextExtentExPointI(aDC,
                                         (WORD*) aGlyphs,
                                         length,
                                         INT_MAX,
                                         NULL,
                                         partialWidthArray.Elements(),
                                         &size);
    if (!success)
        return PR_FALSE;

    aRun->AddGlyphRun(aFont, 0);

    gfxTextRun::CompressedGlyph g;
    PRUint32 i;
    PRInt32 lastWidth = 0;
    PRUint32 appUnitsPerDevPixel = aRun->GetAppUnitsPerDevUnit();
    for (i = 0; i < length; ++i) {
        PRInt32 advancePixels = partialWidthArray[i] - lastWidth;
        lastWidth = partialWidthArray[i];
        PRInt32 advanceAppUnits = advancePixels*appUnitsPerDevPixel;
        WCHAR glyph = aGlyphs[i];
        if (advanceAppUnits >= 0 &&
            gfxTextRun::CompressedGlyph::IsSimpleAdvance(advanceAppUnits) &&
            gfxTextRun::CompressedGlyph::IsSimpleGlyphID(glyph)) {
            aRun->SetCharacterGlyph(i, g.SetSimpleGlyph(advanceAppUnits, glyph));
        } else {
            gfxTextRun::DetailedGlyph details;
            details.mIsLastGlyph = PR_TRUE;
            details.mGlyphID = glyph;
            details.mAdvance = advanceAppUnits;
            details.mXOffset = 0;
            details.mYOffset = 0;
            aRun->SetDetailedGlyphs(i, &details, 1);
        }
    }
    return PR_TRUE;
}

void
gfxWindowsFontGroup::InitTextRunGDI(gfxContext *aContext, gfxTextRun *aRun,
                                    const char *aString, PRUint32 aLength)
{
    gfxWindowsFont *font = GetFontAt(0);
    HDC dc = SetupContextFont(aContext, font);
    if (dc) {
        nsAutoTArray<WCHAR,500> glyphArray;
        if (!glyphArray.AppendElements(aLength))
            return;

        DWORD ret = GetGlyphIndicesA(dc, aString, aLength, (WORD*) glyphArray.Elements(),
                                     GGI_MARK_NONEXISTING_GLYPHS);
        if (ret != GDI_ERROR &&
            SetupTextRunFromGlyphs(aRun, glyphArray.Elements(), dc, font))
            return;
    }

    nsDependentCSubstring cString(aString, aString + aLength);
    nsAutoString utf16;
    AppendASCIItoUTF16(cString, utf16);
    InitTextRunUniscribe(aContext, aRun, utf16.get(), aLength);
}

void
gfxWindowsFontGroup::InitTextRunGDI(gfxContext *aContext, gfxTextRun *aRun,
                                    const PRUnichar *aString, PRUint32 aLength)
{
    gfxWindowsFont *font = GetFontAt(0);
    HDC dc = SetupContextFont(aContext, font);
    if (dc) {
        nsAutoTArray<WCHAR,500> glyphArray;
        if (!glyphArray.AppendElements(aLength))
            return;

        DWORD ret = GetGlyphIndicesW(dc, aString, aLength, (WORD*) glyphArray.Elements(),
                                     GGI_MARK_NONEXISTING_GLYPHS);
        if (ret != GDI_ERROR &&
            SetupTextRunFromGlyphs(aRun, glyphArray.Elements(), dc, font))
            return;
    }

    InitTextRunUniscribe(aContext, aRun, aString, aLength);
}






struct ScriptPropertyEntry {
    const char *value;
    const char *langCode;
};

static const struct ScriptPropertyEntry gScriptToText[] =
{
    { nsnull, nsnull },
    { "LANG_ARABIC",     "ara" },
    { "LANG_BULGARIAN",  "bul" },
    { "LANG_CATALAN",    "cat" },
    { "LANG_CHINESE",    "zh-CN" }, 
    { "LANG_CZECH",      "cze" }, 
    { "LANG_DANISH",     "dan" },
    { "LANG_GERMAN",     "ger" }, 
    { "LANG_GREEK",      "el" }, 
    { "LANG_ENGLISH",    "x-western" },
    { "LANG_SPANISH",    "spa" },
    { "LANG_FINNISH",    "fin" },
    { "LANG_FRENCH",     "fre" }, 
    { "LANG_HEBREW",     "he" }, 
    { "LANG_HUNGARIAN",  "hun" },
    { "LANG_ICELANDIC",  "ice" }, 
    { "LANG_ITALIAN",    "ita" },
    { "LANG_JAPANESE",   "ja" }, 
    { "LANG_KOREAN",     "ko" }, 
    { "LANG_DUTCH",      "dut" }, 
    { "LANG_NORWEGIAN",  "nor" },
    { "LANG_POLISH",     "pol" },
    { "LANG_PORTUGUESE", "por" },
    { nsnull, nsnull },
    { "LANG_ROMANIAN",   "rum" }, 
    { "LANG_RUSSIAN",    "rus" },
    { "LANG_SERBIAN",    "scc" }, 
    { "LANG_SLOVAK",     "slo" }, 
    { "LANG_ALBANIAN",   "alb" }, 
    { "LANG_SWEDISH",    "swe" },
    { "LANG_THAI",       "th" }, 
    { "LANG_TURKISH",    "tr" }, 
    { "LANG_URDU",       "urd" },
    { "LANG_INDONESIAN", "ind" },
    { "LANG_UKRAINIAN",  "ukr" },
    { "LANG_BELARUSIAN", "bel" },
    { "LANG_SLOVENIAN",  "slv" },
    { "LANG_ESTONIAN",   "est" },
    { "LANG_LATVIAN",    "lav" },
    { "LANG_LITHUANIAN", "lit" },
    { nsnull, nsnull },
    { "LANG_FARSI",      "per" }, 
    { "LANG_VIETNAMESE", "vie" },
    { "LANG_ARMENIAN",   "x-armn" }, 
    { "LANG_AZERI",      "aze" },
    { "LANG_BASQUE",     "baq" }, 
    { nsnull, nsnull },
    { "LANG_MACEDONIAN", "mac" }, 
    { nsnull, nsnull },
    { nsnull, nsnull },
    { nsnull, nsnull },
    { nsnull, nsnull },
    { nsnull, nsnull },
    { nsnull, nsnull },
    { "LANG_AFRIKAANS",  "afr" },
    { "LANG_GEORGIAN",   "x-geor" }, 
    { "LANG_FAEROESE",   "fao" },
    { "LANG_HINDI",      "x-devanagari" }, 
    { nsnull, nsnull },
    { nsnull, nsnull },
    { nsnull, nsnull },
    { nsnull, nsnull },
    { "LANG_MALAY",      "may" }, 
    { "LANG_KAZAK",      "kaz" }, 
    { "LANG_KYRGYZ",     "kis" },
    { "LANG_SWAHILI",    "swa" },
    { nsnull, nsnull },
    { "LANG_UZBEK",      "uzb" },
    { "LANG_TATAR",      "tat" },
    { "LANG_BENGALI",    "x-beng" }, 
    { "LANG_PUNJABI",    "x-guru" }, 
    { "LANG_GUJARATI",   "x-gujr" }, 
    { "LANG_ORIYA",      "ori" },
    { "LANG_TAMIL",      "x-tamil" }, 
    { "LANG_TELUGU",     "tel" },
    { "LANG_KANNADA",    "kan" },
    { "LANG_MALAYALAM",  "x-mlym" }, 
    { "LANG_ASSAMESE",   "asm" },
    { "LANG_MARATHI",    "mar" },
    { "LANG_SANSKRIT",   "san" },
    { "LANG_MONGOLIAN",  "mon" },
    { "TIBETAN",         "tib" }, 
    { nsnull, nsnull },
    { "KHMER",           "x-khmr" }, 
    { "LAO",             "lao" },
    { "MYANMAR",         "bur" }, 
    { "LANG_GALICIAN",   "glg" },
    { "LANG_KONKANI",    "kok" },
    { "LANG_MANIPURI",   "mni" },
    { "LANG_SINDHI",     "x-devanagari" }, 
    { "LANG_SYRIAC",     "syr" },
    { "SINHALESE",       "sin" },
    { "CHEROKEE",        "chr" },
    { "INUKTITUT",       "x-cans" }, 
    { "ETHIOPIC",        "x-ethi" }, 
    { nsnull, nsnull },
    { "LANG_KASHMIRI",   "x-devanagari" }, 
    { "LANG_NEPALI",     "x-devanagari" }, 
    { nsnull, nsnull },
    { nsnull, nsnull },
    { nsnull, nsnull },
    { "LANG_DIVEHI",     "div" }
};

static const char *sCJKLangGroup[] = {
    "ja",
    "ko",
    "zh-CN",
    "zh-HK",
    "zh-TW"
};

#define COUNT_OF_CJK_LANG_GROUP 5
#define CJK_LANG_JA    sCJKLangGroup[0]
#define CJK_LANG_KO    sCJKLangGroup[1]
#define CJK_LANG_ZH_CN sCJKLangGroup[2]
#define CJK_LANG_ZH_HK sCJKLangGroup[3]
#define CJK_LANG_ZH_TW sCJKLangGroup[4]

#define STATIC_STRING_LENGTH 100





class UniscribeItem
{
public:
    UniscribeItem(gfxContext *aContext, HDC aDC,
                  const PRUnichar *aString, PRUint32 aLength,
                  SCRIPT_ITEM *aItem,
                  gfxWindowsFontGroup *aGroup) :
        mContext(aContext), mDC(aDC), mString(aString),
        mLength(aLength), mAlternativeString(nsnull), mScriptItem(aItem),
        mScript(aItem->a.eScript), mGroup(aGroup),
        mGlyphs(nsnull), mClusters(nsnull), mAttr(nsnull),
        mNumGlyphs(0), mMaxGlyphs((int)(1.5 * aLength) + 16),
        mOffsets(nsnull), mAdvances(nsnull),
        mFontIndex(0), mTriedPrefFonts(PR_FALSE),
        mTriedOtherFonts(PR_FALSE), mAppendedCJKFonts(PR_FALSE),
        mFontSelected(PR_FALSE)
    {
        mGlyphs = (WORD *)malloc(mMaxGlyphs * sizeof(WORD));
        mClusters = (WORD *)malloc((mLength + 1) * sizeof(WORD));
        mAttr = (SCRIPT_VISATTR *)malloc(mMaxGlyphs * sizeof(SCRIPT_VISATTR));

        
        for (PRUint32 i = 0; i < mGroup->FontListLength(); ++i)
            mFonts.AppendElement(mGroup->GetFontAt(i));
    }

    ~UniscribeItem() {
        free(mGlyphs);
        free(mClusters);
        free(mAttr);
        free(mOffsets);
        free(mAdvances);
        free(mAlternativeString);
    }

    const PRUnichar *GetString() const { return mString; }
    const PRUint32 GetStringLength() const { return mLength; }



    static PRBool AddFontCallback(const nsAString& aName,
                                  const nsACString& aGenericName,
                                  void *closure) {
        if (aName.IsEmpty())
            return PR_TRUE;

        UniscribeItem *item = NS_STATIC_CAST(UniscribeItem*, closure);

        
        PRUint32 len = item->mFonts.Length();
        for (PRUint32 i = 0; i < len; ++i)
            if (aName.Equals(item->mFonts[i]->GetName()))
                return PR_TRUE;

        nsRefPtr<gfxWindowsFont> font =
            GetOrMakeFont(aName, item->mGroup->GetStyle());
        if (font) {
            item->mFonts.AppendElement(font);
        }
        return PR_TRUE;
    }

#ifdef DEBUG_pavlov
    HRESULT Break() {
        HRESULT rv;

        SCRIPT_LOGATTR *logAttrs = (SCRIPT_LOGATTR*)malloc(sizeof(SCRIPT_LOGATTR) * mLength);

        rv = ScriptBreak(mString, mLength, &mScriptItem->a, logAttrs);

        for (PRUint32 i = 0; i < mLength; ++i) {
            PR_LOG(gFontLog, PR_LOG_DEBUG, ("0x%04x - %d %d %d %d %d",
                                            mString[i],
                                            logAttrs[i].fSoftBreak,
                                            logAttrs[i].fWhiteSpace,
                                            logAttrs[i].fCharStop,
                                            logAttrs[i].fWordStop,
                                            logAttrs[i].fInvalid));
        }

        free(logAttrs);
        return rv;
    }
#endif

    





    HRESULT Shape() {
        HRESULT rv;

        HDC shapeDC = nsnull;

        while (PR_TRUE) {
            const PRUnichar *str =
                mAlternativeString ? mAlternativeString : mString;
            mScriptItem->a.fLogicalOrder = PR_TRUE;
            mScriptItem->a.s.fDisplayZWG = PR_TRUE;

            rv = ScriptShape(shapeDC, mCurrentFont->ScriptCache(),
                             str, mLength,
                             mMaxGlyphs, &mScriptItem->a,
                             mGlyphs, mClusters,
                             mAttr, &mNumGlyphs);

            if (rv == E_OUTOFMEMORY) {
                mMaxGlyphs *= 2;
                mGlyphs = (WORD *)realloc(mGlyphs, mMaxGlyphs * sizeof(WORD));
                mAttr = (SCRIPT_VISATTR *)realloc(mAttr, mMaxGlyphs * sizeof(SCRIPT_VISATTR));
                continue;
            }

            if (rv == E_PENDING) {
                SelectFont();

                shapeDC = mDC;
                continue;
            }

#if 0 
            if (rv != USP_E_SCRIPT_NOT_IN_FONT && !shapeDC)
                printf("skipped select-shape %d\n", rv);
#endif
            return rv;
        }
    }

    PRBool ShapingEnabled() {
        return (mScriptItem->a.eScript != SCRIPT_UNDEFINED);
    }
    void DisableShaping() {
        mScriptItem->a.eScript = SCRIPT_UNDEFINED;
        
        
        
        
        GenerateAlternativeString();
    }

    

    PRBool IsMissingGlyphsCMap() {
        HRESULT rv;
        HDC cmapDC = nsnull;

        while (PR_TRUE) {
            rv = ScriptGetCMap(cmapDC, mCurrentFont->ScriptCache(),
                               mString, mLength, 0, mGlyphs);

            if (rv == E_PENDING) {
                SelectFont();
                cmapDC = mDC;
                continue;
            }

            if (rv == S_OK)
                return PR_FALSE;

            PR_LOG(gFontLog, PR_LOG_WARNING, ("cmap is missing a glyph"));
            for (PRUint32 i = 0; i < mLength; i++)
                PR_LOG(gFontLog, PR_LOG_WARNING, (" - %d", mGlyphs[i]));
            return PR_TRUE;
        }
    }

    PRBool IsGlyphMissing(SCRIPT_FONTPROPERTIES *aSFP, PRUint32 aGlyphIndex) {
        if (mGlyphs[aGlyphIndex] == aSFP->wgDefault)
            return PR_TRUE;
        return PR_FALSE;
    }

    PRBool IsMissingGlyphs() {
        SCRIPT_FONTPROPERTIES sfp;
        ScriptFontProperties(&sfp);
        PRUint32 charIndex = 0;
        for (int i = 0; i < mNumGlyphs; ++i) {
            if (IsGlyphMissing(&sfp, i))
                return PR_TRUE;
#ifdef DEBUG_pavlov 
            PR_LOG(gFontLog, PR_LOG_DEBUG, ("%04x %04x %04x", sfp.wgBlank, sfp.wgDefault, sfp.wgInvalid));
            PR_LOG(gFontLog, PR_LOG_DEBUG, ("glyph%d - 0x%04x", i, mGlyphs[i]));
            PR_LOG(gFontLog, PR_LOG_DEBUG, ("%04x  --  %04x -- %04x", ScriptProperties()->fInvalidGlyph, mScriptItem->a.fNoGlyphIndex, mAttr[i].fZeroWidth));
#endif
        }
        return PR_FALSE;
    }

    HRESULT Place() {
        HRESULT rv;

        mOffsets = (GOFFSET *)malloc(mNumGlyphs * sizeof(GOFFSET));
        mAdvances = (int *)malloc(mNumGlyphs * sizeof(int));

        HDC placeDC = nsnull;

        while (PR_TRUE) {
            rv = ScriptPlace(placeDC, mCurrentFont->ScriptCache(),
                             mGlyphs, mNumGlyphs,
                             mAttr, &mScriptItem->a,
                             mAdvances, mOffsets, NULL);

            if (rv == E_PENDING) {
                SelectFont();
                placeDC = mDC;
                continue;
            }

            break;
        }

        return rv;
    }

    const SCRIPT_PROPERTIES *ScriptProperties() {
        
        static const SCRIPT_PROPERTIES **gScriptProperties;
        static int gMaxScript = -1;

        if (gMaxScript == -1) {
            ScriptGetProperties(&gScriptProperties, &gMaxScript);
        }
        return gScriptProperties[mScript];
    }

    void ScriptFontProperties(SCRIPT_FONTPROPERTIES *sfp) {
        HRESULT rv;

        memset(sfp, 0, sizeof(SCRIPT_FONTPROPERTIES));
        sfp->cBytes = sizeof(SCRIPT_FONTPROPERTIES);
        rv = ScriptGetFontProperties(NULL, mCurrentFont->ScriptCache(),
                                     sfp);
        if (rv == E_PENDING) {
            SelectFont();
            rv = ScriptGetFontProperties(mDC, mCurrentFont->ScriptCache(),
                                         sfp);
        }
    }

    void SetupClusterBoundaries(gfxTextRun *aRun, PRUint32 aOffsetInRun) {
        if (aRun->GetFlags() & gfxTextRunFactory::TEXT_IS_8BIT)
            return;

        nsAutoTArray<SCRIPT_LOGATTR,STATIC_STRING_LENGTH> logAttr;
        if (!logAttr.AppendElements(mLength))
            return;
        HRESULT rv = ScriptBreak(mString, mLength, &mScriptItem->a, logAttr.Elements());
        if (FAILED(rv))
            return;
        gfxTextRun::CompressedGlyph g;
        
        
        
        for (PRUint32 i = 1; i < mLength; ++i) {
            if (!logAttr[i].fCharStop) {
                aRun->SetCharacterGlyph(i + aOffsetInRun, g.SetClusterContinuation());
            }
        }
    }

    void SaveGlyphs(gfxTextRun *aRun) {
        PRUint32 offsetInRun = mScriptItem->iCharPos;
        SetupClusterBoundaries(aRun, offsetInRun);

        aRun->AddGlyphRun(GetCurrentFont(), offsetInRun);

        
        SCRIPT_FONTPROPERTIES sfp;
        ScriptFontProperties(&sfp);

        PRUint32 offset = 0;
        nsAutoTArray<gfxTextRun::DetailedGlyph,1> detailedGlyphs;
        gfxTextRun::CompressedGlyph g;
        const PRUint32 appUnitsPerDevUnit = aRun->GetAppUnitsPerDevUnit();
        while (offset < mLength) {
            PRUint32 runOffset = offsetInRun + offset;
            if (offset > 0 && mClusters[offset] == mClusters[offset - 1]) {
                if (!aRun->GetCharacterGlyphs()[runOffset].IsClusterContinuation()) {
                    
                    aRun->SetCharacterGlyph(runOffset, g.SetLigatureContinuation());
                }
            } else {
                
                PRUint32 k = mClusters[offset];
                PRUint32 glyphCount = mNumGlyphs - k;
                PRUint32 nextClusterOffset;
                PRBool missing = IsGlyphMissing(&sfp, k);
                for (nextClusterOffset = offset + 1; nextClusterOffset < mLength; ++nextClusterOffset) {
                    if (mClusters[nextClusterOffset] > k) {
                        glyphCount = mClusters[nextClusterOffset] - k;
                        break;
                    }
                }
                PRUint32 j;
                for (j = 1; j < glyphCount; ++j) {
                    if (IsGlyphMissing(&sfp, k + j)) {
                        missing = PR_TRUE;
                    }
                }
                PRInt32 advance = mAdvances[k]*appUnitsPerDevUnit;
                WORD glyph = mGlyphs[k];
                if (missing) {
                    aRun->SetMissingGlyph(runOffset, mString[offset]);
                } else if (glyphCount == 1 && advance >= 0 &&
                    mOffsets[k].dv == 0 && mOffsets[k].du == 0 &&
                    gfxTextRun::CompressedGlyph::IsSimpleAdvance(advance) &&
                    gfxTextRun::CompressedGlyph::IsSimpleGlyphID(glyph)) {
                    aRun->SetCharacterGlyph(runOffset, g.SetSimpleGlyph(advance, glyph));
                } else {
                    if (detailedGlyphs.Length() < glyphCount) {
                        if (!detailedGlyphs.AppendElements(glyphCount - detailedGlyphs.Length()))
                            return;
                    }
                    PRUint32 i;
                    for (i = 0; i < glyphCount; ++i) {
                        gfxTextRun::DetailedGlyph *details = &detailedGlyphs[i];
                        details->mIsLastGlyph = i == glyphCount - 1;
                        details->mGlyphID = mGlyphs[k + i];
                        details->mAdvance = mAdvances[k + i]*appUnitsPerDevUnit;
                        details->mXOffset = float(mOffsets[k + i].du)*appUnitsPerDevUnit;
                        details->mYOffset = float(mOffsets[k + i].dv)*appUnitsPerDevUnit;
                    }
                    aRun->SetDetailedGlyphs(runOffset, detailedGlyphs.Elements(), glyphCount);
                }
            }
            ++offset;
        }
    }

    gfxWindowsFont *GetNextFont() {
        
TRY_AGAIN_HOPE_FOR_THE_BEST_2:
        if (mFontIndex < mFonts.Length()) {
            nsRefPtr<gfxWindowsFont> font = mFonts[mFontIndex];
            mFontIndex++;
            return font;
        } else if (!mTriedPrefFonts) {
            mTriedPrefFonts = PR_TRUE;

            
            const SCRIPT_PROPERTIES *sp = ScriptProperties();
            if (!sp->fAmbiguousCharSet) {
                WORD primaryId = PRIMARYLANGID(sp->langid);
                const char *langGroup = gScriptToText[primaryId].langCode;
                if (langGroup) {
                    if (PR_LOG_TEST(gFontLog, PR_LOG_DEBUG))
                        PR_LOG(gFontLog, PR_LOG_DEBUG, ("Trying to find fonts for: %s (%s)", langGroup, gScriptToText[primaryId].value));
                    AppendPrefFonts(langGroup);
                } else if (primaryId != 0) {
#ifdef DEBUG_pavlov
                    printf("Couldn't find anything about %d\n", primaryId);
#endif
                }
            } else {
                for (PRUint32 i = 0; i < mLength; ++i) {
                    const PRUnichar ch = mString[i];
                    PRUint32 unicodeRange = FindCharUnicodeRange(ch);

                    
                    if (unicodeRange == kRangeSetCJK) {
                        if (!mAppendedCJKFonts) {
                            mAppendedCJKFonts = PR_TRUE;

                            if (PR_LOG_TEST(gFontLog, PR_LOG_DEBUG))
                                PR_LOG(gFontLog, PR_LOG_DEBUG, ("Trying to find fonts for: CJK"));

                            AppendCJKPrefFonts();
                        }
                    } else {
                        const char *langGroup = LangGroupFromUnicodeRange(unicodeRange);
                        if (langGroup) {
                            if (PR_LOG_TEST(gFontLog, PR_LOG_DEBUG))
                                PR_LOG(gFontLog, PR_LOG_DEBUG, ("Trying to find fonts for: %s", langGroup));
                            AppendPrefFonts(langGroup);
                        }
                    }
                }
            }
            goto TRY_AGAIN_HOPE_FOR_THE_BEST_2;
        } else if (!mTriedOtherFonts) {
            mTriedOtherFonts = PR_TRUE;
            nsString fonts;
            gfxWindowsPlatform *platform = gfxWindowsPlatform::GetPlatform();

            if (PR_LOG_TEST(gFontLog, PR_LOG_DEBUG)) {
                PR_LOG(gFontLog, PR_LOG_DEBUG, ("Looking for other fonts to support the string:"));
                for (PRUint32 la = 0; la < mLength; la++) {
                    PRUint32 ch = mString[la];

                    if ((la+1 < mLength) && NS_IS_HIGH_SURROGATE(ch) && NS_IS_LOW_SURROGATE(mString[la+1])) {
                        la++;
                        ch = SURROGATE_TO_UCS4(ch, mString[la]);
                    }

                    PR_LOG(gFontLog, PR_LOG_DEBUG, (" - 0x%04x", ch));
                }
            }
            
            platform->FindOtherFonts(mString, mLength,
                                     nsPromiseFlatCString(mGroup->GetStyle()->langGroup).get(),
                                     nsPromiseFlatCString(mGroup->GetGenericFamily()).get(),
                                     fonts);
            if (!fonts.IsEmpty()) {
                if (PR_LOG_TEST(gFontLog, PR_LOG_DEBUG))
                    PR_LOG(gFontLog, PR_LOG_DEBUG, ("Got back: %s", NS_LossyConvertUTF16toASCII(fonts).get()));
                gfxFontGroup::ForEachFont(fonts, EmptyCString(), UniscribeItem::AddFontCallback, this);
            }
            goto TRY_AGAIN_HOPE_FOR_THE_BEST_2;
        } else {
            
            
            
            
        }

        return nsnull;
    }

    void ResetFontIndex() {
        mFontIndex = 0;
    }

    void SetCurrentFont(gfxWindowsFont *aFont) {
        if (mCurrentFont != aFont) {
            mCurrentFont = aFont;
            cairo_win32_scaled_font_done_font(mCurrentFont->CairoScaledFont());
            mFontSelected = PR_FALSE;
        }
    }

    gfxWindowsFont *GetCurrentFont() {
        return mCurrentFont;
    }

    void SelectFont() {
        if (mFontSelected)
            return;

        cairo_t *cr = mContext->GetCairo();

        cairo_set_font_face(cr, mCurrentFont->CairoFontFace());
        cairo_set_font_size(cr, mCurrentFont->GetAdjustedSize());

        cairo_win32_scaled_font_select_font(mCurrentFont->CairoScaledFont(), mDC);

        mFontSelected = PR_TRUE;
    }


private:
    static PRInt32 GetCJKLangGroupIndex(const char *aLangGroup) {
        PRInt32 i;
        for (i = 0; i < COUNT_OF_CJK_LANG_GROUP; i++) {
            if (!PL_strcasecmp(aLangGroup, sCJKLangGroup[i]))
                return i;
        }
        return -1;
    }

    void AppendPrefFonts(const char *aLangGroup) {
        NS_ASSERTION(aLangGroup, "aLangGroup is null");
        gfxPlatform *platform = gfxPlatform::GetPlatform();
        nsString fonts;
        platform->GetPrefFonts(aLangGroup, fonts);
        if (fonts.IsEmpty())
            return;
        gfxFontGroup::ForEachFont(fonts, nsDependentCString(aLangGroup),
                                  UniscribeItem::AddFontCallback, this);
        return;
   }

   void AppendCJKPrefFonts() {
       nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
       if (!prefs)
           return;

       nsCOMPtr<nsIPrefBranch> prefBranch;
       prefs->GetBranch(0, getter_AddRefs(prefBranch));
       if (!prefBranch)
           return;

       
       nsXPIDLCString list;
       nsresult rv = prefBranch->GetCharPref("intl.accept_languages", getter_Copies(list));
       if (NS_SUCCEEDED(rv) && !list.IsEmpty()) {
           const char kComma = ',';
           const char *p, *p_end;
           list.BeginReading(p);
           list.EndReading(p_end);
           while (p < p_end) {
               while (nsCRT::IsAsciiSpace(*p)) {
                   if (++p == p_end)
                       break;
               }
               if (p == p_end)
                   break;
               const char *start = p;
               while (++p != p_end && *p != kComma)
                    ;
               nsCAutoString lang(Substring(start, p));
               lang.CompressWhitespace(PR_FALSE, PR_TRUE);
               PRInt32 index = GetCJKLangGroupIndex(lang.get());
               if (index >= 0)
                   AppendPrefFonts(sCJKLangGroup[index]);
               p++;
           }
       }

       
       switch (::GetACP()) {
           case 932: AppendPrefFonts(CJK_LANG_JA);    break;
           case 936: AppendPrefFonts(CJK_LANG_ZH_CN); break;
           case 949: AppendPrefFonts(CJK_LANG_KO);    break;
           
           case 950: AppendPrefFonts(CJK_LANG_ZH_TW); break;
       }

       
       AppendPrefFonts(CJK_LANG_JA);
       AppendPrefFonts(CJK_LANG_KO);
       AppendPrefFonts(CJK_LANG_ZH_CN);
       AppendPrefFonts(CJK_LANG_ZH_HK);
       AppendPrefFonts(CJK_LANG_ZH_TW);
    }

    void GenerateAlternativeString() {
        if (mAlternativeString)
            free(mAlternativeString);
        mAlternativeString = (PRUnichar *)malloc(mLength * sizeof(PRUnichar));
        memcpy((void *)mAlternativeString, (const void *)mString,
               mLength * sizeof(PRUnichar));
        for (PRUint32 i = 0; i < mLength; i++) {
            if (NS_IS_HIGH_SURROGATE(mString[i]) || NS_IS_LOW_SURROGATE(mString[i]))
                mAlternativeString[i] = PRUnichar(0xFFFD);
        }
    }
private:
    nsRefPtr<gfxContext> mContext;
    HDC mDC;

    SCRIPT_ITEM *mScriptItem;
    WORD mScript;

    const PRUnichar *mString;
    const PRUint32 mLength;

    PRUnichar *mAlternativeString;

    gfxWindowsFontGroup *mGroup;

    WORD *mGlyphs;
    WORD *mClusters;
    SCRIPT_VISATTR *mAttr;

    int mMaxGlyphs;
    int mNumGlyphs;

    GOFFSET *mOffsets;
    int *mAdvances;

    nsTArray< nsRefPtr<gfxWindowsFont> > mFonts;

    nsRefPtr<gfxWindowsFont> mCurrentFont;

    PRUint32 mFontIndex;
    PRPackedBool mTriedPrefFonts;
    PRPackedBool mTriedOtherFonts;
    PRPackedBool mAppendedCJKFonts;
    PRPackedBool mFontSelected;
};

class Uniscribe
{
public:
    Uniscribe(gfxContext *aContext, HDC aDC, const PRUnichar *aString, PRUint32 aLength, PRBool aIsRTL) :
        mContext(aContext), mDC(aDC), mString(aString), mLength(aLength), mIsRTL(aIsRTL),
        mIsComplex(ScriptIsComplex(aString, aLength, SIC_COMPLEX) == S_OK),
        mItems(nsnull) {
    }
    ~Uniscribe() {
        if (mItems)
            free(mItems);
    }

    void Init() {
        memset(&mControl, 0, sizeof(SCRIPT_CONTROL));
        memset(&mState, 0, sizeof(SCRIPT_STATE));
        
        
        mState.uBidiLevel = mIsRTL;
        mState.fOverrideDirection = PR_TRUE;
    }

    int Itemize() {
        HRESULT rv;

        int maxItems = 5;

        Init();
        mItems = (SCRIPT_ITEM *)malloc(maxItems * sizeof(SCRIPT_ITEM));
        while ((rv = ScriptItemize(mString, mLength, maxItems, &mControl, &mState,
                                   mItems, &mNumItems)) == E_OUTOFMEMORY) {
            maxItems *= 2;
            mItems = (SCRIPT_ITEM *)realloc(mItems, maxItems * sizeof(SCRIPT_ITEM));
            Init();
        }

        return mNumItems;
    }

    PRUint32 ItemsLength() {
        return mNumItems;
    }

    
    
    UniscribeItem *GetItem(PRUint32 i, gfxWindowsFontGroup *aGroup) {
        NS_ASSERTION(i < (PRUint32)mNumItems, "Trying to get out of bounds item");

        UniscribeItem *item = new UniscribeItem(mContext, mDC,
                                                mString + mItems[i].iCharPos,
                                                mItems[i+1].iCharPos - mItems[i].iCharPos,
                                                &mItems[i],
                                                aGroup);

        return item;
    }

private:
    nsRefPtr<gfxContext> mContext;
    HDC mDC;
    const PRUnichar *mString;
    const PRUint32 mLength;
    const PRBool mIsRTL;
    
    const PRBool mIsComplex;

    SCRIPT_CONTROL mControl;
    SCRIPT_STATE   mState;
    SCRIPT_ITEM   *mItems;
    int mNumItems;
};


void
gfxWindowsFontGroup::InitTextRunUniscribe(gfxContext *aContext, gfxTextRun *aRun, const PRUnichar *aString,
                                         PRUint32 aLength)
{
    nsRefPtr<gfxASurface> surf = aContext->CurrentSurface();
    HDC aDC = GetDCFromSurface(surf);
    NS_ASSERTION(aDC, "No DC");
 
    const PRBool isRTL = aRun->IsRightToLeft();

    HRESULT rv;

    Uniscribe us(aContext, aDC, aString, aLength, isRTL);

    
    int numItems = us.Itemize();

    for (int i=0; i < numItems; ++i) {
        PRUint32 fontIndex = 0;

        SaveDC(aDC);

        UniscribeItem *item = us.GetItem(i, this);

        int giveUp = PR_FALSE;

        
        while (PR_TRUE) {
            nsRefPtr<gfxWindowsFont> font = item->GetNextFont();

            if (font) {
                
                font->UpdateCTM(aContext->CurrentMatrix());

                
                item->SetCurrentFont(font);

                if (PR_LOG_TEST(gFontLog, PR_LOG_DEBUG))
                    PR_LOG(gFontLog, PR_LOG_DEBUG, ("trying: %s", NS_LossyConvertUTF16toASCII(font->GetName()).get()));

                PRBool cmapHasGlyphs = PR_FALSE; 

                if (!giveUp && !(aRun->GetFlags() & TEXT_HAS_SURROGATES)) {
                    if (item->IsMissingGlyphsCMap())
                        continue;
                    else
                        cmapHasGlyphs = PR_TRUE;
                }

                rv = item->Shape();

                if (giveUp) {
                    if (PR_LOG_TEST(gFontLog, PR_LOG_DEBUG))
                        PR_LOG(gFontLog, PR_LOG_DEBUG, ("%s - gave up", NS_LossyConvertUTF16toASCII(font->GetName()).get()));
                    goto SCRIPT_PLACE;
                }

                if (FAILED(rv))
                    continue;

                if (!cmapHasGlyphs && item->IsMissingGlyphs())
                    continue;

            } else {
#if 0
                

                if (item->ShapingEnabled()) {
                    item->DisableShaping();
                    item->ResetFontIndex();
                    continue;
                }
#endif
                giveUp = PR_TRUE;
                item->DisableShaping();
                item->ResetFontIndex();
                continue;
            }

            if (PR_LOG_TEST(gFontLog, PR_LOG_DEBUG))
                PR_LOG(gFontLog, PR_LOG_DEBUG, ("%s - worked", NS_LossyConvertUTF16toASCII(font->GetName()).get()));

SCRIPT_PLACE:
            NS_ASSERTION(SUCCEEDED(rv), "Failed to shape -- we should never hit this");

#ifdef DEBUG_pavlov
            item->Break();
#endif

            rv = item->Place();
            if (FAILED(rv)) {
                if (PR_LOG_TEST(gFontLog, PR_LOG_WARNING))
                    PR_LOG(gFontLog, PR_LOG_WARNING, ("Failed to place with %s", NS_LossyConvertUTF16toASCII(font->GetName()).get()));
                continue;
            }

            break;
        }

        item->SaveGlyphs(aRun);
        delete item;

        RestoreDC(aDC, -1);
    }
}
