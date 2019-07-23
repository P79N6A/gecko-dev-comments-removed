







































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

#include "nsTArray.h"
#include "nsUnicodeRange.h"
#include "nsUnicharUtils.h"

#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIPrefLocalizedString.h"
#include "nsServiceManagerUtils.h"

#include "nsCRT.h"

#include <math.h>

#include "prlog.h"
#include "prinit.h"
static PRLogModuleInfo *gFontLog = PR_NewLogModule("winfonts");

#define ROUND(x) floor((x) + 0.5)

BYTE 
FontTypeToOutPrecision(PRUint8 fontType)
{
#ifdef WINCE
    return OUT_DEFAULT_PRECIS;
#else
    BYTE ret;
    switch (fontType) {
    case GFX_FONT_TYPE_TT_OPENTYPE:
    case GFX_FONT_TYPE_TRUETYPE:
        ret = OUT_TT_ONLY_PRECIS;
        break;
    case GFX_FONT_TYPE_PS_OPENTYPE:
        ret = OUT_PS_ONLY_PRECIS;
        break;
    case GFX_FONT_TYPE_TYPE1:
        ret = OUT_OUTLINE_PRECIS;
        break;
    case GFX_FONT_TYPE_RASTER:
        ret = OUT_RASTER_PRECIS;
        break;
    case GFX_FONT_TYPE_DEVICE:
        ret = OUT_DEVICE_PRECIS;
        break;
    default:
        ret = OUT_DEFAULT_PRECIS;
    }
    return ret;
#endif
}

struct DCFromContext {
    DCFromContext(gfxContext *aContext) {
        dc = NULL;
        nsRefPtr<gfxASurface> aSurface = aContext->CurrentSurface();
        NS_ASSERTION(aSurface, "DCFromContext: null surface");
        if (aSurface &&
            (aSurface->GetType() == gfxASurface::SurfaceTypeWin32 ||
             aSurface->GetType() == gfxASurface::SurfaceTypeWin32Printing))
        {
            dc = static_cast<gfxWindowsSurface*>(aSurface.get())->GetDC();
            needsRelease = PR_FALSE;
        }
        if (!dc) {
            dc = GetDC(NULL);
            SetGraphicsMode(dc, GM_ADVANCED);
            needsRelease = PR_TRUE;
        }
    }

    ~DCFromContext() {
        if (needsRelease)
            ReleaseDC(NULL, dc);
    }

    operator HDC () {
        return dc;
    }

    HDC dc;
    PRBool needsRelease;
};







static nsresult
ReadCMAP(HDC hdc, FontEntry *aFontEntry)
{
    const PRUint32 kCMAP = (('c') | ('m' << 8) | ('a' << 16) | ('p' << 24));

    DWORD len = GetFontData(hdc, kCMAP, 0, nsnull, 0);
    if (len == GDI_ERROR || len == 0) 
        return NS_ERROR_FAILURE;      

    nsAutoTArray<PRUint8,16384> buffer;
    if (!buffer.AppendElements(len))
        return NS_ERROR_OUT_OF_MEMORY;
    PRUint8 *buf = buffer.Elements();

    DWORD newLen = GetFontData(hdc, kCMAP, 0, buf, len);
    NS_ENSURE_TRUE(newLen == len, NS_ERROR_FAILURE);

    
    PRPackedBool unicodeFont = aFontEntry->mUnicodeFont;
    PRPackedBool symbolFont = aFontEntry->mSymbolFont;
    nsresult rv = gfxFontUtils::ReadCMAP(buf, len, aFontEntry->mCharacterMap,
                                         unicodeFont, symbolFont);
    aFontEntry->mUnicodeFont = unicodeFont;
    aFontEntry->mSymbolFont = symbolFont;

    return rv;
}

struct FamilyAddStyleProcData {
    HDC dc;
    FontFamily *ff;
};

int CALLBACK 
FontFamily::FamilyAddStylesProc(const ENUMLOGFONTEXW *lpelfe,
                                const NEWTEXTMETRICEXW *nmetrics,
                                DWORD fontType, LPARAM data)
{
    const NEWTEXTMETRICW& metrics = nmetrics->ntmTm;
    LOGFONTW logFont = lpelfe->elfLogFont;

    FamilyAddStyleProcData *faspd = reinterpret_cast<FamilyAddStyleProcData*>(data);
    FontFamily *ff = faspd->ff;
    HDC hdc = faspd->dc;

    
    logFont.lfWeight = PR_MAX(PR_MIN(logFont.lfWeight, 900), 100);

    gfxWindowsFontType feType;
    if (metrics.ntmFlags & NTM_TYPE1)
        feType = GFX_FONT_TYPE_TYPE1;
    else if (metrics.ntmFlags & (NTM_PS_OPENTYPE))
        feType = GFX_FONT_TYPE_PS_OPENTYPE;
    else if (metrics.ntmFlags & (NTM_TT_OPENTYPE))
        feType = GFX_FONT_TYPE_TT_OPENTYPE;
    else if (fontType == TRUETYPE_FONTTYPE)
        feType = GFX_FONT_TYPE_TRUETYPE;
    else if (fontType == RASTER_FONTTYPE)
        feType = GFX_FONT_TYPE_RASTER;
    else if (fontType == DEVICE_FONTTYPE)
        feType = GFX_FONT_TYPE_DEVICE;
    else
        feType = GFX_FONT_TYPE_UNKNOWN;

    FontEntry *fe = nsnull;
    for (PRUint32 i = 0; i < ff->mVariations.Length(); ++i) {
        fe = ff->mVariations[i];
        if (feType > fe->mFontType) {
            
            ff->mVariations.RemoveElementAt(i);
            --i;
        } else if (feType < fe->mFontType) {
            
            return 1;
        }
    }

    for (PRUint32 i = 0; i < ff->mVariations.Length(); ++i) {
        fe = ff->mVariations[i];
        
        if (fe->mWeight == logFont.lfWeight &&
            fe->mItalic == (logFont.lfItalic == 0xFF)) {
            
            fe->mCharset[metrics.tmCharSet] = 1;
            return 1; 
        }
    }

    fe = new FontEntry(ff->mName);
    ff->mVariations.AppendElement(fe);
    fe->mFontType = feType;

    fe->mItalic = (logFont.lfItalic == 0xFF);
    fe->mWeight = logFont.lfWeight;

    if (fe->IsType1())
        fe->mForceGDI = PR_TRUE;

    
    fe->mCharset[metrics.tmCharSet] = 1;

    fe->mWindowsFamily = logFont.lfPitchAndFamily & 0xF0;
    fe->mWindowsPitch = logFont.lfPitchAndFamily & 0x0F;

    if (nmetrics->ntmFontSig.fsUsb[0] != 0x00000000 &&
        nmetrics->ntmFontSig.fsUsb[1] != 0x00000000 &&
        nmetrics->ntmFontSig.fsUsb[2] != 0x00000000 &&
        nmetrics->ntmFontSig.fsUsb[3] != 0x00000000) {

        
        PRUint32 x = 0;
        for (PRUint32 i = 0; i < 4; ++i) {
            DWORD range = nmetrics->ntmFontSig.fsUsb[i];
            for (PRUint32 k = 0; k < 32; ++k) {
                fe->mUnicodeRanges[x++] = (range & (1 << k)) != 0;
            }
        }
    }

    fe->mIsBadUnderlineFont = ff->mIsBadUnderlineFont;

    
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfOutPrecision = FontTypeToOutPrecision(fe->mFontType);

    HFONT font = CreateFontIndirectW(&logFont);

    NS_ASSERTION(font, "This font creation should never ever ever fail");
    if (font) {
        HFONT oldFont = (HFONT)SelectObject(hdc, font);

        
        if (NS_FAILED(ReadCMAP(hdc, fe))) {
            
            
            if (fe->IsType1())
                fe->mUnicodeFont = PR_TRUE;
            else
                fe->mUnicodeFont = PR_FALSE;

            
            
            fe->mUnknownCMAP = PR_TRUE;

            
        } else {
            
        }

        SelectObject(hdc, oldFont);
        DeleteObject(font);
    }

    return 1;
}


void
FontFamily::FindStyleVariations()
{
    if (mHasStyles)
        return;
    mHasStyles = PR_TRUE;

    HDC hdc = GetDC(nsnull);
    SetGraphicsMode(hdc, GM_ADVANCED);

    LOGFONTW logFont;
    memset(&logFont, 0, sizeof(LOGFONTW));
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfPitchAndFamily = 0;
    PRUint32 l = PR_MIN(mName.Length(), LF_FACESIZE - 1);
    memcpy(logFont.lfFaceName,
           nsPromiseFlatString(mName).get(),
           l * sizeof(PRUnichar));
    logFont.lfFaceName[l] = 0;

    FamilyAddStyleProcData faspd;
    faspd.dc = hdc;
    faspd.ff = this;

    EnumFontFamiliesExW(hdc, &logFont, (FONTENUMPROCW)FontFamily::FamilyAddStylesProc, (LPARAM)&faspd, 0);

    ReleaseDC(nsnull, hdc);

    
    
    FontEntry *darkestItalic = nsnull;
    FontEntry *darkestNonItalic = nsnull;
    PRUint8 highestItalic = 0, highestNonItalic = 0;
    for (PRUint32 i = 0; i < mVariations.Length(); i++) {
        FontEntry *fe = mVariations[i];
        if (fe->mItalic) {
            if (!darkestItalic || fe->mWeight > darkestItalic->mWeight)
                darkestItalic = fe;
        } else {
            if (!darkestNonItalic || fe->mWeight > darkestNonItalic->mWeight)
                darkestNonItalic = fe;
        }
    }

    if (darkestItalic && darkestItalic->mWeight < 600) {
        FontEntry *newEntry = new FontEntry(*darkestItalic);
        newEntry->mWeight = 600;
        mVariations.AppendElement(newEntry);
    }
    if (darkestNonItalic && darkestNonItalic->mWeight < 600) {
        FontEntry *newEntry = new FontEntry(*darkestNonItalic);
        newEntry->mWeight = 600;
        mVariations.AppendElement(newEntry);
    }
}


FontEntry *
FontFamily::FindFontEntry(const gfxFontStyle& aFontStyle)
{
    if (!mHasStyles)
        FindStyleVariations();

    PRUint8 bestMatch = 0;
    PRBool italic = (aFontStyle.style & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE)) != 0;

    FontEntry *weightList[10] = { 0 };
    for (PRUint32 j = 0; j < 2; j++) {
        PRBool matchesSomething = PR_FALSE;
        
        for (PRUint32 i = 0; i < mVariations.Length(); i++) {
            FontEntry *fe = mVariations[i];
            const PRUint8 weight = (fe->mWeight / 100);
            if (fe->mItalic == italic) {
                weightList[weight] = fe;
                matchesSomething = PR_TRUE;
            }
        }
        if (matchesSomething)
            break;
        italic = !italic;
    }

    PRInt8 baseWeight, weightDistance;
    aFontStyle.ComputeWeightAndOffset(&baseWeight, &weightDistance);

    
    
    if (baseWeight == 5 && weightDistance == 0) {
        
        if (weightList[5])
            return weightList[5];

        
        baseWeight = 4;
    }

    PRInt8 matchBaseWeight = 0;
    PRInt8 direction = (baseWeight > 5) ? 1 : -1;
    for (PRInt8 i = baseWeight; ; i += direction) {
        if (weightList[i]) {
            matchBaseWeight = i;
            break;
        }

        
        
        if (i == 1 || i == 9)
            direction = -direction;
    }

    FontEntry *matchFE;
    const PRInt8 absDistance = abs(weightDistance);
    direction = (weightDistance >= 0) ? 1 : -1;
    for (PRInt8 i = matchBaseWeight, k = 0; i < 10 && i > 0; i += direction) {
        if (weightList[i]) {
            matchFE = weightList[i];
            k++;
        }
        if (k > absDistance)
            break;
    }

    if (!matchFE)
        matchFE = weightList[matchBaseWeight];

    NS_ASSERTION(matchFE, "we should always be able to return something here");
    return matchFE;
}








gfxWindowsFont::gfxWindowsFont(const nsAString& aName, const gfxFontStyle *aFontStyle, FontEntry *aFontEntry)
    : gfxFont(aName, aFontStyle),
      mFont(nsnull), mAdjustedSize(0.0), mScriptCache(nsnull),
      mFontFace(nsnull), mScaledFont(nsnull),
      mMetrics(nsnull), mFontEntry(aFontEntry), mIsValid(PR_TRUE)
{
    NS_ASSERTION(mFontEntry, "Unable to find font entry for font.  Something is whack.");

    mFont = MakeHFONT(); 
    NS_ASSERTION(mFont, "Failed to make HFONT");
}

gfxWindowsFont::~gfxWindowsFont()
{
    if (mFontFace)
        cairo_font_face_destroy(mFontFace);

    if (mScaledFont)
        cairo_scaled_font_destroy(mScaledFont);

    if (mFont)
        DeleteObject(mFont);

    ScriptFreeCache(&mScriptCache);

    delete mMetrics;
}

const gfxFont::Metrics&
gfxWindowsFont::GetMetrics()
{
    if (!mMetrics)
        ComputeMetrics();

    return *mMetrics;
}

cairo_font_face_t *
gfxWindowsFont::CairoFontFace()
{
    if (!mFontFace)
        mFontFace = cairo_win32_font_face_create_for_logfontw_hfont(&mLogFont, mFont);

    NS_ASSERTION(mFontFace, "Failed to make font face");

    return mFontFace;
}

cairo_scaled_font_t *
gfxWindowsFont::CairoScaledFont()
{
    if (!mScaledFont) {
        cairo_matrix_t sizeMatrix;
        cairo_matrix_t identityMatrix;

        cairo_matrix_init_scale(&sizeMatrix, mAdjustedSize, mAdjustedSize);
        cairo_matrix_init_identity(&identityMatrix);

        cairo_font_options_t *fontOptions = cairo_font_options_create();
        mScaledFont = cairo_scaled_font_create(CairoFontFace(), &sizeMatrix,
                                               &identityMatrix, fontOptions);
        cairo_font_options_destroy(fontOptions);
    }

    NS_ASSERTION(mAdjustedSize == 0.0 ||
                 cairo_scaled_font_status(mScaledFont) == CAIRO_STATUS_SUCCESS,
                 "Failed to make scaled font");

    return mScaledFont;
}

HFONT
gfxWindowsFont::MakeHFONT()
{
    if (mFont)
        return mFont;

    mAdjustedSize = GetStyle()->size;
    if (GetStyle()->sizeAdjust > 0.0) {
        if (!mFont) {
            FillLogFont(mAdjustedSize);
            mFont = CreateFontIndirectW(&mLogFont);
        }

        Metrics *oldMetrics = mMetrics;
        ComputeMetrics();
        gfxFloat aspect = mMetrics->xHeight / mMetrics->emHeight;
        mAdjustedSize = GetStyle()->GetAdjustedSize(aspect);

        if (mMetrics != oldMetrics) {
            delete mMetrics;
            mMetrics = oldMetrics;
        }
        DeleteObject(mFont);
        mFont = nsnull;
    }

    if (!mFont) {
        FillLogFont(mAdjustedSize);
        mFont = CreateFontIndirectW(&mLogFont);
    }

    return mFont;
}

void
gfxWindowsFont::ComputeMetrics()
{
    if (!mMetrics)
        mMetrics = new gfxFont::Metrics;
    else
        NS_WARNING("Calling ComputeMetrics multiple times");

    HDC dc = GetDC((HWND)nsnull);
    SetGraphicsMode(dc, GM_ADVANCED);

    HGDIOBJ oldFont = SelectObject(dc, mFont);

    
    OUTLINETEXTMETRIC oMetrics;
    TEXTMETRIC& metrics = oMetrics.otmTextMetrics;

    if (0 < GetOutlineTextMetrics(dc, sizeof(oMetrics), &oMetrics)) {
        mMetrics->superscriptOffset = (double)oMetrics.otmptSuperscriptOffset.y;
        
        mMetrics->subscriptOffset = fabs((double)oMetrics.otmptSubscriptOffset.y);
        mMetrics->strikeoutSize = (double)oMetrics.otmsStrikeoutSize;
        mMetrics->strikeoutOffset = (double)oMetrics.otmsStrikeoutPosition;
        mMetrics->underlineSize = (double)oMetrics.otmsUnderscoreSize;
        mMetrics->underlineOffset = (double)oMetrics.otmsUnderscorePosition;

        const MAT2 kIdentityMatrix = { {0, 1}, {0, 0}, {0, 0}, {0, 1} };
        GLYPHMETRICS gm;
        DWORD len = GetGlyphOutlineW(dc, PRUnichar('x'), GGO_METRICS, &gm, 0, nsnull, &kIdentityMatrix);
        if (len == GDI_ERROR || gm.gmptGlyphOrigin.y <= 0) {
            
            mMetrics->xHeight = ROUND((double)metrics.tmAscent * 0.56);
        } else {
            mMetrics->xHeight = gm.gmptGlyphOrigin.y;
        }
        mMetrics->emHeight = metrics.tmHeight - metrics.tmInternalLeading;
        gfxFloat typEmHeight = (double)oMetrics.otmAscent - (double)oMetrics.otmDescent;
        mMetrics->emAscent = ROUND(mMetrics->emHeight * (double)oMetrics.otmAscent / typEmHeight);
        mMetrics->emDescent = mMetrics->emHeight - mMetrics->emAscent;
    } else {
        
        
        
        
        
        BOOL result = GetTextMetrics(dc, &metrics);
        if (!result) {
            NS_WARNING("Missing or corrupt font data, fasten your seatbelt");
            mIsValid = PR_FALSE;
            memset(mMetrics, 0, sizeof(*mMetrics));
            SelectObject(dc, oldFont);
            ReleaseDC((HWND)nsnull, dc);
            return;
        }

        mMetrics->xHeight = ROUND((float)metrics.tmAscent * 0.56f); 
        mMetrics->superscriptOffset = mMetrics->xHeight;
        mMetrics->subscriptOffset = mMetrics->xHeight;
        mMetrics->strikeoutSize = 1;
        mMetrics->strikeoutOffset = ROUND(mMetrics->xHeight / 2.0f); 
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
    mMetrics->aveCharWidth = PR_MAX(1, metrics.tmAveCharWidth);
    
    
    if (!(metrics.tmPitchAndFamily & TMPF_FIXED_PITCH)) {
      mMetrics->maxAdvance = mMetrics->aveCharWidth;
    }

    
    SIZE size;
    GetTextExtentPoint32(dc, " ", 1, &size);
    mMetrics->spaceWidth = ROUND(size.cx);

    
    
    
    
    if (GetTextExtentPoint32(dc, "0", 1, &size))
        mMetrics->zeroOrAveCharWidth = ROUND(size.cx);
    else
        mMetrics->zeroOrAveCharWidth = mMetrics->aveCharWidth;

    mSpaceGlyph = 0;
    if (metrics.tmPitchAndFamily & TMPF_TRUETYPE) {
        WORD glyph;
        DWORD ret = GetGlyphIndicesA(dc, " ", 1, &glyph,
                                     GGI_MARK_NONEXISTING_GLYPHS);
        if (ret != GDI_ERROR && glyph != 0xFFFF) {
            mSpaceGlyph = glyph;
        }
    }

    SelectObject(dc, oldFont);

    ReleaseDC((HWND)nsnull, dc);

    SanitizeMetrics(mMetrics, mFontEntry->IsBadUnderlineFont());
}

void
gfxWindowsFont::FillLogFont(gfxFloat aSize)
{
#define CLIP_TURNOFF_FONTASSOCIATION 0x40
    
    mLogFont.lfHeight = (LONG)-ROUND(aSize);

    if (mLogFont.lfHeight == 0)
        mLogFont.lfHeight = -1;

    
    mLogFont.lfWidth          = 0;
    mLogFont.lfEscapement     = 0;
    mLogFont.lfOrientation    = 0;
    mLogFont.lfUnderline      = FALSE;
    mLogFont.lfStrikeOut      = FALSE;
    mLogFont.lfCharSet        = DEFAULT_CHARSET;
    mLogFont.lfOutPrecision   = FontTypeToOutPrecision(mFontEntry->mFontType);
    mLogFont.lfClipPrecision  = CLIP_TURNOFF_FONTASSOCIATION;
    mLogFont.lfQuality        = DEFAULT_QUALITY;
    mLogFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    
    
    
    
    mLogFont.lfItalic         = (GetStyle()->style & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE)) ? TRUE : FALSE;
    mLogFont.lfWeight         = mFontEntry->mWeight;

    int len = PR_MIN(mName.Length(), LF_FACESIZE - 1);
    memcpy(mLogFont.lfFaceName, nsPromiseFlatString(mName).get(), len * 2);
    mLogFont.lfFaceName[len] = '\0';
}


nsString
gfxWindowsFont::GetUniqueName()
{
    nsString uniqueName;

    
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

PRBool
gfxWindowsFont::SetupCairoFont(gfxContext *aContext)
{
    cairo_scaled_font_t *scaledFont = CairoScaledFont();
    if (cairo_scaled_font_status(scaledFont) != CAIRO_STATUS_SUCCESS) {
        
        
        return PR_FALSE;
    }
    cairo_set_scaled_font(aContext->GetCairo(), scaledFont);
    return PR_TRUE;
}












already_AddRefed<gfxWindowsFont>
gfxWindowsFont::GetOrMakeFont(FontEntry *aFontEntry, const gfxFontStyle *aStyle)
{
    
    
    gfxFontStyle style(*aStyle);
    style.weight = aFontEntry->mWeight;
    
    if (style.sizeAdjust == 0.0)
        style.size = ROUND(style.size);

    nsRefPtr<gfxFont> font = gfxFontCache::GetCache()->Lookup(aFontEntry->GetName(), &style);
    if (!font) {
        font = new gfxWindowsFont(aFontEntry->GetName(), &style, aFontEntry);
        if (!font)
            return nsnull;
        gfxFontCache::GetCache()->AddNew(font);
    }
    gfxFont *f = nsnull;
    font.swap(f);
    return static_cast<gfxWindowsFont *>(f);
}

static PRBool
AddFontNameToArray(const nsAString& aName,
                   const nsACString& aGenericName,
                   void *closure)
{
    if (!aName.IsEmpty()) {
        nsTArray<nsString> *list = static_cast<nsTArray<nsString> *>(closure);

        if (list->IndexOf(aName) == list->NoIndex)
            list->AppendElement(aName);
    }

    return PR_TRUE;
}

void
gfxWindowsFontGroup::GroupFamilyListToArrayList(nsTArray<nsRefPtr<FontEntry> > *list)
{
    nsAutoTArray<nsString, 15> fonts;
    ForEachFont(AddFontNameToArray, &fonts);

    PRUint32 len = fonts.Length();
    for (PRUint32 i = 0; i < len; ++i) {
        nsRefPtr<FontEntry> fe = gfxWindowsPlatform::GetPlatform()->FindFontEntry(fonts[i], mStyle);
        list->AppendElement(fe);
    }
}

void
gfxWindowsFontGroup::FamilyListToArrayList(const nsString& aFamilies,
                                           const nsCString& aLangGroup,
                                           nsTArray<nsRefPtr<FontEntry> > *list)
{
    nsAutoTArray<nsString, 15> fonts;
    ForEachFont(aFamilies, aLangGroup, AddFontNameToArray, &fonts);

    PRUint32 len = fonts.Length();
    for (PRUint32 i = 0; i < len; ++i) {
        const nsString& str = fonts[i];
        nsRefPtr<FontEntry> fe = gfxWindowsPlatform::GetPlatform()->FindFontEntry(str, mStyle);
        list->AppendElement(fe);
    }
}

gfxWindowsFontGroup::gfxWindowsFontGroup(const nsAString& aFamilies, const gfxFontStyle *aStyle)
    : gfxFontGroup(aFamilies, aStyle)
{
    GroupFamilyListToArrayList(&mFontEntries);

    mFonts.AppendElements(mFontEntries.Length());

    
    
    while (mFontEntries.Length() > 0) {
        nsRefPtr<gfxWindowsFont> font =
            gfxWindowsFont::GetOrMakeFont(mFontEntries[0], &mStyle);
        if (!font->IsValid()) {
            mFontEntries.RemoveElementAt(0);
            mFonts.RemoveElementAt(0);
            continue;
        }
        mFonts[0] = font;
        break;
    }

    if (mFontEntries.Length() == 0) {
        
        
        nsAutoString str;
        HGDIOBJ hGDI = ::GetStockObject(DEFAULT_GUI_FONT);
        LOGFONTW logFont;
        if (hGDI && ::GetObjectW(hGDI, sizeof(logFont), &logFont)) {
            str.AppendLiteral("\"");
            str.Append(nsDependentString(logFont.lfFaceName));
            str.AppendLiteral("\"");
        }

        NONCLIENTMETRICSW ncm;
        ncm.cbSize = sizeof(ncm);
        BOOL status = ::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 
                                              sizeof(ncm), &ncm, 0);
        if (status) {
            str.AppendLiteral(",\"");
            str.Append(nsDependentString(ncm.lfMessageFont.lfFaceName));
            str.AppendLiteral("\"");
        }

        FamilyListToArrayList(str, mStyle.langGroup, &mFontEntries);

        
        
        
        
        
        
        
        mFonts.AppendElements(mFontEntries.Length());
    }

    if (!mStyle.systemFont) {
        for (PRUint32 i = 0; i < mFontEntries.Length(); ++i) {
            if (mFontEntries[i]->IsBadUnderlineFont()) {
                gfxFloat first = GetFontAt(0)->GetMetrics().underlineOffset;
                gfxFloat bad = GetFontAt(i)->GetMetrics().underlineOffset;
                mUnderlineOffset = PR_MIN(first, bad);
                break;
            }
        }
    }
}

gfxWindowsFontGroup::~gfxWindowsFontGroup()
{
}

gfxWindowsFont *
gfxWindowsFontGroup::GetFontAt(PRInt32 i)
{
    if (!mFonts[i]) {
        nsRefPtr<gfxWindowsFont> font =
            gfxWindowsFont::GetOrMakeFont(mFontEntries[i], &mStyle);
        mFonts[i] = font;
    }

    return static_cast<gfxWindowsFont*>(mFonts[i].get());
}

gfxFontGroup *
gfxWindowsFontGroup::Copy(const gfxFontStyle *aStyle)
{
    return new gfxWindowsFontGroup(mFamilies, aStyle);
}

static PRBool
CanTakeFastPath(PRUint32 aFlags)
{
    
    
    return (aFlags &
            (gfxTextRunFactory::TEXT_OPTIMIZE_SPEED | gfxTextRunFactory::TEXT_IS_RTL)) ==
        gfxTextRunFactory::TEXT_OPTIMIZE_SPEED;
}

gfxTextRun *
gfxWindowsFontGroup::MakeTextRun(const PRUnichar *aString, PRUint32 aLength,
                                 const Parameters *aParams, PRUint32 aFlags)
{
    
    
    

    gfxTextRun *textRun = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!textRun)
        return nsnull;
    NS_ASSERTION(aParams->mContext, "MakeTextRun called without a gfxContext");

    textRun->RecordSurrogates(aString);
    
#ifdef FORCE_UNISCRIBE
    const PRBool isComplex = PR_TRUE;
#else
    const PRBool isComplex = !CanTakeFastPath(aFlags) ||
                             ScriptIsComplex(aString, aLength, SIC_COMPLEX) == S_OK;
#endif
    if (isComplex)
        InitTextRunUniscribe(aParams->mContext, textRun, aString, aLength);
    else
        InitTextRunGDI(aParams->mContext, textRun, aString, aLength);

    textRun->FetchGlyphExtents(aParams->mContext);

    return textRun;
}

gfxTextRun *
gfxWindowsFontGroup::MakeTextRun(const PRUint8 *aString, PRUint32 aLength,
                                 const Parameters *aParams, PRUint32 aFlags)
{
    NS_ASSERTION(aFlags & TEXT_IS_8BIT, "should be marked 8bit");
 
    gfxTextRun *textRun = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!textRun)
        return nsnull;
    NS_ASSERTION(aParams->mContext, "MakeTextRun called without a gfxContext");

#ifdef FORCE_UNISCRIBE
    const PRBool isComplex = PR_TRUE;
#else
    const PRBool isComplex = !CanTakeFastPath(aFlags);
#endif

    


    if (!isComplex && (aFlags & TEXT_IS_ASCII)) {
        InitTextRunGDI(aParams->mContext, textRun,
                       reinterpret_cast<const char*>(aString), aLength);
    }
    else {
        nsDependentCSubstring cString(reinterpret_cast<const char*>(aString),
                                  reinterpret_cast<const char*>(aString + aLength));
        nsAutoString utf16;
        AppendASCIItoUTF16(cString, utf16);
        if (isComplex) {
            InitTextRunUniscribe(aParams->mContext, textRun, utf16.get(), aLength);
        } else {
            InitTextRunGDI(aParams->mContext, textRun, utf16.get(), aLength);
        }
    }

    textRun->FetchGlyphExtents(aParams->mContext);

    return textRun;
}






static PRBool
SetupDCFont(HDC dc, gfxWindowsFont *aFont)
{
    HFONT hfont = aFont->GetHFONT();
    if (!hfont)
        return PR_FALSE;
    SelectObject(dc, hfont);

    
    
    if (!aFont->GetFontEntry()->IsTrueType() || aFont->GetFontEntry()->mSymbolFont)
        return PR_FALSE;

    return PR_TRUE;
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
    if (!partialWidthArray.SetLength(length))
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
        NS_ASSERTION(!gfxFontGroup::IsInvalidChar(aRun->GetChar(i)),
                     "Invalid character detected!");
        if (advanceAppUnits >= 0 &&
            gfxTextRun::CompressedGlyph::IsSimpleAdvance(advanceAppUnits) &&
            gfxTextRun::CompressedGlyph::IsSimpleGlyphID(glyph)) {
            aRun->SetSimpleGlyph(i, g.SetSimpleGlyph(advanceAppUnits, glyph));
        } else {
            gfxTextRun::DetailedGlyph details;
            details.mGlyphID = glyph;
            details.mAdvance = advanceAppUnits;
            details.mXOffset = 0;
            details.mYOffset = 0;
            aRun->SetGlyphs(i, g.SetComplex(PR_TRUE, PR_TRUE, 1), &details);
        }
    }
    return PR_TRUE;
}

void
gfxWindowsFontGroup::InitTextRunGDI(gfxContext *aContext, gfxTextRun *aRun,
                                    const char *aString, PRUint32 aLength)
{
    nsRefPtr<gfxWindowsFont> font = GetFontAt(0);
    DCFromContext dc(aContext);
    if (SetupDCFont(dc, font)) {
        nsAutoTArray<WCHAR,500> glyphArray;
        if (!glyphArray.SetLength(aLength))
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
    nsRefPtr<gfxWindowsFont> font = GetFontAt(0);
    DCFromContext dc(aContext);
    if (SetupDCFont(dc, font)) {
        nsAutoTArray<WCHAR,500> glyphArray;
        if (!glyphArray.SetLength(aLength))
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
    { "LANG_ARABIC",     "ar" }, 
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
    { "LANG_ORIYA",      "x-orya" }, 
    { "LANG_TAMIL",      "x-tamil" }, 
    { "LANG_TELUGU",     "x-telu" },  
    { "LANG_KANNADA",    "x-knda" },  
    { "LANG_MALAYALAM",  "x-mlym" }, 
    { "LANG_ASSAMESE",   "x-beng" },    
    { "LANG_MARATHI",    "x-devanagari" }, 
    { "LANG_SANSKRIT",   "x-devanagari" }, 
    { "LANG_MONGOLIAN",  "mon" },
    { "TIBETAN",         "tib" }, 
    { nsnull, nsnull },
    { "KHMER",           "x-khmr" }, 
    { "LAO",             "lao" },
    { "MYANMAR",         "bur" }, 
    { "LANG_GALICIAN",   "glg" },
    { "LANG_KONKANI",    "kok" },
    { "LANG_MANIPURI",   "mni" },
    { "LANG_SINDHI",     "snd" },
    { "LANG_SYRIAC",     "syr" },
    { "SINHALESE",       "x-sinh" }, 
    { "CHEROKEE",        "chr" },
    { "INUKTITUT",       "x-cans" }, 
    { "ETHIOPIC",        "x-ethi" }, 
    { nsnull, nsnull },
    { "LANG_KASHMIRI",   "kas" },
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

#define ESTIMATE_MAX_GLYPHS(L) (((3 * (L)) >> 1) + 16)

class UniscribeItem
{
public:
    UniscribeItem(gfxContext *aContext, HDC aDC,
                  const PRUnichar *aString, PRUint32 aLength,
                  SCRIPT_ITEM *aItem,
                  gfxWindowsFontGroup *aGroup) :
        mContext(aContext), mDC(aDC), mRangeString(nsnull), mRangeLength(0),
        mItemString(aString), mItemLength(aLength), 
        mAlternativeString(nsnull), mScriptItem(aItem),
        mScript(aItem->a.eScript), mGroup(aGroup),
        mNumGlyphs(0), mMaxGlyphs(ESTIMATE_MAX_GLYPHS(aLength)),
        mFontSelected(PR_FALSE)
    {
        NS_ASSERTION(mMaxGlyphs < 65535, "UniscribeItem is too big, ScriptShape() will fail!");
        mGlyphs.SetLength(mMaxGlyphs);
        mClusters.SetLength(mItemLength + 1);
        mAttr.SetLength(mMaxGlyphs);
    }

    ~UniscribeItem() {
        free(mAlternativeString);
    }

    




    HRESULT ShapeUniscribe() {
        HRESULT rv;
        HDC shapeDC = nsnull;

        const PRUnichar *str = mAlternativeString ? mAlternativeString : mRangeString;

        mScriptItem->a.fLogicalOrder = PR_TRUE; 
        SCRIPT_ANALYSIS sa = mScriptItem->a;
        







        if (mRangeString > mItemString)
            sa.fLinkBefore = PR_FALSE;
        if (mRangeString + mRangeLength < mItemString + mItemLength)
            sa.fLinkAfter = PR_FALSE;

        while (PR_TRUE) {

            rv = ScriptShape(shapeDC, mCurrentFont->ScriptCache(),
                             str, mRangeLength,
                             mMaxGlyphs, &sa,
                             mGlyphs.Elements(), mClusters.Elements(),
                             mAttr.Elements(), &mNumGlyphs);

            if (rv == E_OUTOFMEMORY) {
                mGlyphs.SetLength(mMaxGlyphs);
                mAttr.SetLength(mMaxGlyphs);
                mMaxGlyphs *= 2;
                continue;
            }

            if (rv == E_PENDING) {
                if (shapeDC == mDC) {
                    
                    return E_PENDING;
                }

                SelectFont();

                shapeDC = mDC;
                continue;
            }

            return rv;
        }
    }

    HRESULT ShapeGDI() {
        SelectFont();

        mNumGlyphs = mRangeLength;
        GetGlyphIndicesW(mDC, mRangeString, mRangeLength,
                         (WORD*) mGlyphs.Elements(),
                         GGI_MARK_NONEXISTING_GLYPHS);

        for (PRUint32 i = 0; i < mRangeLength; ++i)
            mClusters[i] = i;

        return S_OK;
    }

    HRESULT Shape() {
        
        if (mCurrentFont->GetFontEntry()->mForceGDI)
            return ShapeGDI();

        return ShapeUniscribe();
    }

    PRBool ShapingEnabled() {
        return (mScriptItem->a.eScript != SCRIPT_UNDEFINED);
    }
    void DisableShaping() {
        mScriptItem->a.eScript = SCRIPT_UNDEFINED;
        
        
        
        
        GenerateAlternativeString();
    }
    void EnableShaping() {
        mScriptItem->a.eScript = mScript;
        if (mAlternativeString) {
            free(mAlternativeString);
            mAlternativeString = nsnull;
        }
    }

    PRBool IsGlyphMissing(SCRIPT_FONTPROPERTIES *aSFP, PRUint32 aGlyphIndex) {
        PRBool missing = PR_FALSE;
        if (GetCurrentFont()->GetFontEntry()->mForceGDI) {
            
            if (mGlyphs[aGlyphIndex] == 0xFFFF)
                missing = PR_TRUE;
        } else if (mGlyphs[aGlyphIndex] == aSFP->wgDefault) {
            missing = PR_TRUE;
        }
        return missing;
    }


    HRESULT PlaceUniscribe() {
        HRESULT rv;
        HDC placeDC = nsnull;

        while (PR_TRUE) {
            rv = ScriptPlace(placeDC, mCurrentFont->ScriptCache(),
                             mGlyphs.Elements(), mNumGlyphs,
                             mAttr.Elements(), &mScriptItem->a,
                             mAdvances.Elements(), mOffsets.Elements(), NULL);

            if (rv == E_PENDING) {
                SelectFont();
                placeDC = mDC;
                continue;
            }

            break;
        }

        return rv;
    }

    HRESULT PlaceGDI() {
        SelectFont();

        nsAutoTArray<int,500> partialWidthArray;
        
        
        if (!partialWidthArray.SetLength(mNumGlyphs))
            PR_Abort();
        SIZE size;

        GetTextExtentExPointI(mDC,
                              (WORD*) mGlyphs.Elements(),
                              mNumGlyphs,
                              INT_MAX,
                              NULL,
                              partialWidthArray.Elements(),
                              &size);

        PRInt32 lastWidth = 0;

        for (PRUint32 i = 0; i < mNumGlyphs; i++) {
            mAdvances[i] = partialWidthArray[i] - lastWidth;
            lastWidth = partialWidthArray[i];
            mOffsets[i].du = mOffsets[i].dv = 0;
        }
        return 0;
    }

    HRESULT Place() {
        mOffsets.SetLength(mNumGlyphs);
        mAdvances.SetLength(mNumGlyphs);

        PRBool allCJK = PR_TRUE;

        
        
        if (!mCurrentFont->GetFontEntry()->mForceGDI) {
            for (PRUint32 i = 0; i < mRangeLength; i++) {
                const PRUnichar ch = mRangeString[i];
                if (ch == ' ' || FindCharUnicodeRange(ch) == kRangeSetCJK)
                    continue;

                allCJK = PR_FALSE;
                break;
            }
        }

        if (allCJK)
            return PlaceGDI();

        return PlaceUniscribe();
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
        if (!logAttr.AppendElements(mRangeLength))
            return;
        HRESULT rv = ScriptBreak(mRangeString, mRangeLength,
                                 &mScriptItem->a, logAttr.Elements());
        if (FAILED(rv))
            return;
        gfxTextRun::CompressedGlyph g;
        
        
        
        for (PRUint32 i = 1; i < mRangeLength; ++i) {
            if (!logAttr[i].fCharStop) {
                aRun->SetGlyphs(i + aOffsetInRun, g.SetComplex(PR_FALSE, PR_TRUE, 0), nsnull);
            }
        }
    }

    void SaveGlyphs(gfxTextRun *aRun) {
        PRUint32 offsetInRun = mScriptItem->iCharPos + (mRangeString - mItemString);
        SetupClusterBoundaries(aRun, offsetInRun);

        aRun->AddGlyphRun(GetCurrentFont(), offsetInRun);

        
        SCRIPT_FONTPROPERTIES sfp;
        ScriptFontProperties(&sfp);

        PRUint32 offset = 0;
        nsAutoTArray<gfxTextRun::DetailedGlyph,1> detailedGlyphs;
        gfxTextRun::CompressedGlyph g;
        const PRUint32 appUnitsPerDevUnit = aRun->GetAppUnitsPerDevUnit();
        while (offset < mRangeLength) {
            PRUint32 runOffset = offsetInRun + offset;
            if (offset > 0 && mClusters[offset] == mClusters[offset - 1]) {
                g.SetComplex(aRun->IsClusterStart(runOffset), PR_FALSE, 0);
                aRun->SetGlyphs(runOffset, g, nsnull);
            } else {
                
                PRUint32 k = mClusters[offset];
                PRUint32 glyphCount = mNumGlyphs - k;
                PRUint32 nextClusterOffset;
                PRBool missing = IsGlyphMissing(&sfp, k);
                for (nextClusterOffset = offset + 1; nextClusterOffset < mRangeLength; ++nextClusterOffset) {
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
                NS_ASSERTION(!gfxFontGroup::IsInvalidChar(mRangeString[offset]),
                             "invalid character detected");
                if (missing) {
                    if (NS_IS_HIGH_SURROGATE(mRangeString[offset]) &&
                        offset + 1 < mRangeLength &&
                        NS_IS_LOW_SURROGATE(mRangeString[offset + 1])) {
                        aRun->SetMissingGlyph(runOffset,
                                              SURROGATE_TO_UCS4(mRangeString[offset],
                                                                mRangeString[offset + 1]));
                    } else {
                        aRun->SetMissingGlyph(runOffset, mRangeString[offset]);
                    }
                } else if (glyphCount == 1 && advance >= 0 &&
                    mOffsets[k].dv == 0 && mOffsets[k].du == 0 &&
                    gfxTextRun::CompressedGlyph::IsSimpleAdvance(advance) &&
                    gfxTextRun::CompressedGlyph::IsSimpleGlyphID(glyph)) {
                    aRun->SetSimpleGlyph(runOffset, g.SetSimpleGlyph(advance, glyph));
                } else {
                    if (detailedGlyphs.Length() < glyphCount) {
                        if (!detailedGlyphs.AppendElements(glyphCount - detailedGlyphs.Length()))
                            return;
                    }
                    PRUint32 i;
                    for (i = 0; i < glyphCount; ++i) {
                        gfxTextRun::DetailedGlyph *details = &detailedGlyphs[i];
                        details->mGlyphID = mGlyphs[k + i];
                        details->mAdvance = mAdvances[k + i]*appUnitsPerDevUnit;
                        details->mXOffset = float(mOffsets[k + i].du)*appUnitsPerDevUnit*aRun->GetDirection();
                        details->mYOffset = float(mOffsets[k + i].dv)*appUnitsPerDevUnit;
                    }
                    aRun->SetGlyphs(runOffset,
                        g.SetComplex(PR_TRUE, PR_TRUE, glyphCount), detailedGlyphs.Elements());
                }
            }
            ++offset;
        }
    }

    void SetCurrentFont(gfxWindowsFont *aFont) {
        if (mCurrentFont != aFont) {
            mCurrentFont = aFont;
            cairo_scaled_font_t *scaledFont = mCurrentFont->CairoScaledFont();
            cairo_win32_scaled_font_done_font(scaledFont);
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
        cairo_scaled_font_t *scaledFont = mCurrentFont->CairoScaledFont();
        cairo_win32_scaled_font_select_font(scaledFont, mDC);

        mFontSelected = PR_TRUE;
    }

    struct TextRange {
        TextRange(PRUint32 aStart,  PRUint32 aEnd) : start(aStart), end(aEnd) { }
        PRUint32 Length() const { return end - start; }
        nsRefPtr<gfxWindowsFont> font;
        PRUint32 start, end;
    };

    void SetRange(PRUint32 i) {
        nsRefPtr<gfxWindowsFont> font;
        if (mRanges[i].font)
            font = mRanges[i].font;
        else
            font = mGroup->GetFontAt(0);

        SetCurrentFont(font);

        mRangeString = mItemString + mRanges[i].start;
        mRangeLength = mRanges[i].Length();
    }

    PRBool HasCharacter(FontEntry *aFontEntry, PRUint32 ch) {
        if (aFontEntry->mCharacterMap.test(ch))
            return PR_TRUE;

        if (aFontEntry->mUnknownCMAP) {
            if (ch > 0xFFFF)
                return PR_FALSE;

            nsRefPtr<gfxWindowsFont> font =
                gfxWindowsFont::GetOrMakeFont(aFontEntry, mGroup->GetStyle());
            if (!font->IsValid())
                return PR_FALSE;

            HDC dc = GetDC((HWND)nsnull);
            SetGraphicsMode(dc, GM_ADVANCED);
            HFONT hfont = font->GetHFONT();
            HFONT oldFont = (HFONT)SelectObject(dc, hfont);

            PRUnichar str[1] = { (PRUnichar)ch };
            WORD glyph[1];

            PRBool hasGlyph = PR_FALSE;
            if (aFontEntry->IsType1()) {
                
                DWORD ret = GetGlyphIndicesW(dc, str, 1, glyph, GGI_MARK_NONEXISTING_GLYPHS);
                if (ret != GDI_ERROR && glyph[0] != 0xFFFF)
                    hasGlyph = PR_TRUE;
            } else {
                
                HRESULT rv = ScriptGetCMap(dc, font->ScriptCache(), str, 1, 0, glyph);
                if (rv == S_OK)
                    hasGlyph = PR_TRUE;
            }

            if (hasGlyph) {
                aFontEntry->mCharacterMap.set(ch);
                return PR_TRUE;
            }

            SelectObject(dc, oldFont);
            ReleaseDC(NULL, dc);
        }

        return PR_FALSE;
    }

    inline already_AddRefed<gfxWindowsFont>
    WhichFontSupportsChar(const nsTArray<nsRefPtr<FontEntry> >& fonts, PRUint32 ch) {
        for (PRUint32 i = 0; i < fonts.Length(); i++) {
            nsRefPtr<FontEntry> fe = fonts[i];
            if (fe->mSymbolFont && !mGroup->GetStyle()->familyNameQuirks)
                continue;
            if (HasCharacter(fe, ch)) {
                nsRefPtr<gfxWindowsFont> font =
                    gfxWindowsFont::GetOrMakeFont(fe, mGroup->GetStyle());
                
                if (!font->IsValid())
                    continue;
                return font.forget();
            }
        }
        return nsnull;
    }

    static inline bool IsJoiner(PRUint32 ch) {
        return (ch == 0x200C ||
                ch == 0x200D ||
                ch == 0x2060);
    }

    inline already_AddRefed<gfxWindowsFont>
    FindFontForChar(PRUint32 ch, PRUint32 prevCh, PRUint32 nextCh,
                    gfxWindowsFont *aFont) {
        nsRefPtr<gfxWindowsFont> selectedFont;

        
        
        if (IsJoiner(ch) || IsJoiner(prevCh) || IsJoiner(nextCh)) {
            if (aFont && HasCharacter(aFont->GetFontEntry(), ch)) {
                NS_ADDREF(aFont);
                return aFont;
            }
        }

        
        selectedFont = WhichFontSupportsChar(mGroup->GetFontList(), ch);

        
        if ((ch >= 0xE000  && ch <= 0xF8FF) || 
            (ch >= 0xF0000 && ch <= 0x10FFFD))
            return selectedFont.forget();

        
        if (!selectedFont) {
            nsAutoTArray<nsRefPtr<FontEntry>, 5> fonts;
            this->GetPrefFonts(mGroup->GetStyle()->langGroup.get(), fonts);
            selectedFont = WhichFontSupportsChar(fonts, ch);
        }

        
        if (!selectedFont) {
            
            const SCRIPT_PROPERTIES *sp = ScriptProperties();
            if (!sp->fAmbiguousCharSet) {
                WORD primaryId = PRIMARYLANGID(sp->langid);
                const char *langGroup = gScriptToText[primaryId].langCode;
                if (langGroup) {
                    PR_LOG(gFontLog, PR_LOG_DEBUG, (" - Trying to find fonts for: %s (%s)", langGroup, gScriptToText[primaryId].value));

                    nsAutoTArray<nsRefPtr<FontEntry>, 5> fonts;
                    this->GetPrefFonts(langGroup, fonts);
                    selectedFont = WhichFontSupportsChar(fonts, ch);
                }
            } else if (ch <= 0xFFFF) {
                PRUint32 unicodeRange = FindCharUnicodeRange(ch);

                
                if (unicodeRange == kRangeSetCJK) {
                    if (PR_LOG_TEST(gFontLog, PR_LOG_DEBUG))
                        PR_LOG(gFontLog, PR_LOG_DEBUG, (" - Trying to find fonts for: CJK"));

                    nsAutoTArray<nsRefPtr<FontEntry>, 15> fonts;
                    this->GetCJKPrefFonts(fonts);
                    selectedFont = WhichFontSupportsChar(fonts, ch);
                } else {
                    const char *langGroup = LangGroupFromUnicodeRange(unicodeRange);
                    if (langGroup) {
                        PR_LOG(gFontLog, PR_LOG_DEBUG, (" - Trying to find fonts for: %s", langGroup));

                        nsAutoTArray<nsRefPtr<FontEntry>, 5> fonts;
                        this->GetPrefFonts(langGroup, fonts);
                        selectedFont = WhichFontSupportsChar(fonts, ch);
                    }
                }
            }
        }

        
        if (!selectedFont && aFont && HasCharacter(aFont->GetFontEntry(), ch))
            selectedFont = aFont;

        
        if (!selectedFont) {
            PR_LOG(gFontLog, PR_LOG_DEBUG, (" - Looking for best match"));

            nsRefPtr<gfxWindowsFont> refFont = mGroup->GetFontAt(0);
            gfxWindowsPlatform *platform = gfxWindowsPlatform::GetPlatform();
            selectedFont = platform->FindFontForChar(ch, refFont);
        }

        return selectedFont.forget();
    }

    PRUint32 ComputeRanges() {
        if (mItemLength == 0)
            return 0;

        PR_LOG(gFontLog, PR_LOG_DEBUG, ("Computing ranges for string: (len = %d)", mItemLength));

        PRUint32 prevCh = 0;
        for (PRUint32 i = 0; i < mItemLength; i++) {
            const PRUint32 origI = i; 
            PRUint32 ch = mItemString[i];
            if ((i+1 < mItemLength) && NS_IS_HIGH_SURROGATE(ch) && NS_IS_LOW_SURROGATE(mItemString[i+1])) {
                i++;
                ch = SURROGATE_TO_UCS4(ch, mItemString[i]);
            }

            PR_LOG(gFontLog, PR_LOG_DEBUG, (" 0x%04x - ", ch));
            PRUint32 nextCh = 0;
            if (i+1 < mItemLength) {
                nextCh = mItemString[i+1];
                if ((i+2 < mItemLength) && NS_IS_HIGH_SURROGATE(nextCh) && NS_IS_LOW_SURROGATE(mItemString[i+2]))
                    nextCh = SURROGATE_TO_UCS4(nextCh, mItemString[i+2]);
            }
            nsRefPtr<gfxWindowsFont> font =
                FindFontForChar(ch,
                                prevCh,
                                nextCh,
                                (mRanges.Length() == 0) ? nsnull : mRanges[mRanges.Length() - 1].font);

            prevCh = ch;

            if (mRanges.Length() == 0) {
                TextRange r(0,1);
                r.font = font;
                mRanges.AppendElement(r);
            } else {
                TextRange& prevRange = mRanges[mRanges.Length() - 1];
                if (prevRange.font != font) {
                    
                    prevRange.end = origI;

                    TextRange r(origI, i+1);
                    r.font = font;
                    mRanges.AppendElement(r);
                }
            }
            if (PR_LOG_TEST(gFontLog, PR_LOG_DEBUG)) {
                if (font)
                    PR_LOG(gFontLog, PR_LOG_DEBUG, (" - Using %s", NS_LossyConvertUTF16toASCII(font->GetUniqueName()).get()));
                else
                    PR_LOG(gFontLog, PR_LOG_DEBUG, (" - Unable to find font"));
            }
        }
        mRanges[mRanges.Length()-1].end = mItemLength;

        PRUint32 nranges = mRanges.Length();
        PR_LOG(gFontLog, PR_LOG_DEBUG, (" Found %d ranges", nranges));
        return nranges;
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

    
    void GetPrefFonts(const char *aLangGroup, nsTArray<nsRefPtr<FontEntry> >& array) {
        NS_ASSERTION(aLangGroup, "aLangGroup is null");
        gfxWindowsPlatform *platform = gfxWindowsPlatform::GetPlatform();
        nsAutoTArray<nsRefPtr<FontEntry>, 5> fonts;
        
        nsCAutoString key(aLangGroup);
        key.Append("-");
        key.AppendInt(mGroup->GetStyle()->style);
        key.Append("-");
        key.AppendInt(mGroup->GetStyle()->weight);
        if (!platform->GetPrefFontEntries(key, &fonts)) {
            nsString fontString;
            platform->GetPrefFonts(aLangGroup, fontString);
            if (fontString.IsEmpty())
                return;

            mGroup->FamilyListToArrayList(fontString, nsDependentCString(aLangGroup),
                                          &fonts);

            platform->SetPrefFontEntries(key, fonts);
        }
        array.AppendElements(fonts);
    }

    
    void GetCJKPrefFonts(nsTArray<nsRefPtr<FontEntry> >& array) {
        gfxWindowsPlatform *platform = gfxWindowsPlatform::GetPlatform();

        nsCAutoString key("x-internal-cjk-");
        key.AppendInt(mGroup->GetStyle()->style);
        key.Append("-");
        key.AppendInt(mGroup->GetStyle()->weight);

        if (!platform->GetPrefFontEntries(key, &array)) {
            nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
            if (!prefs)
                return;

            nsCOMPtr<nsIPrefBranch> prefBranch;
            prefs->GetBranch(0, getter_AddRefs(prefBranch));
            if (!prefBranch)
                return;

            
            nsCAutoString list;
            nsCOMPtr<nsIPrefLocalizedString> val;
            nsresult rv = prefBranch->GetComplexValue("intl.accept_languages", NS_GET_IID(nsIPrefLocalizedString),
                                                      getter_AddRefs(val));
            if (NS_SUCCEEDED(rv) && val) {
                nsAutoString temp;
                val->ToString(getter_Copies(temp));
                LossyCopyUTF16toASCII(temp, list);
            }
            if (!list.IsEmpty()) {
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
                        GetPrefFonts(sCJKLangGroup[index], array);
                    p++;
                }
            }

            
            switch (::GetACP()) {
                case 932: GetPrefFonts(CJK_LANG_JA, array); break;
                case 936: GetPrefFonts(CJK_LANG_ZH_CN, array); break;
                case 949: GetPrefFonts(CJK_LANG_KO, array); break;
                
                case 950: GetPrefFonts(CJK_LANG_ZH_TW, array); break;
            }

            
            GetPrefFonts(CJK_LANG_JA, array);
            GetPrefFonts(CJK_LANG_KO, array);
            GetPrefFonts(CJK_LANG_ZH_CN, array);
            GetPrefFonts(CJK_LANG_ZH_HK, array);
            GetPrefFonts(CJK_LANG_ZH_TW, array);

            platform->SetPrefFontEntries(key, array);
        }
    }

    void GenerateAlternativeString() {
        if (mAlternativeString)
            free(mAlternativeString);
        mAlternativeString = (PRUnichar *)malloc(mRangeLength * sizeof(PRUnichar));
        if (!mAlternativeString)
            return;
        memcpy((void *)mAlternativeString, (const void *)mRangeString,
               mRangeLength * sizeof(PRUnichar));
        for (PRUint32 i = 0; i < mRangeLength; i++) {
            if (NS_IS_HIGH_SURROGATE(mRangeString[i]) || NS_IS_LOW_SURROGATE(mRangeString[i]))
                mAlternativeString[i] = PRUnichar(0xFFFD);
        }
    }

private:
    nsRefPtr<gfxContext> mContext;
    HDC mDC;

    SCRIPT_ITEM *mScriptItem;
    WORD mScript;

    
    const PRUnichar *mRangeString;
    PRUint32 mRangeLength;

    
    const PRUnichar *mItemString;
    const PRUint32 mItemLength;

    PRUnichar *mAlternativeString;

    gfxWindowsFontGroup *mGroup;

#define AVERAGE_ITEM_LENGTH 40

    nsAutoTArray<WORD, PRUint32(ESTIMATE_MAX_GLYPHS(AVERAGE_ITEM_LENGTH))> mGlyphs;
    nsAutoTArray<WORD, AVERAGE_ITEM_LENGTH + 1> mClusters;
    nsAutoTArray<SCRIPT_VISATTR, PRUint32(ESTIMATE_MAX_GLYPHS(AVERAGE_ITEM_LENGTH))> mAttr;
 
    nsAutoTArray<GOFFSET, 2 * AVERAGE_ITEM_LENGTH> mOffsets;
    nsAutoTArray<int, 2 * AVERAGE_ITEM_LENGTH> mAdvances;

#undef AVERAGE_ITEM_LENGTH

    int mMaxGlyphs;
    int mNumGlyphs;

    nsRefPtr<gfxWindowsFont> mCurrentFont;

    PRPackedBool mFontSelected;

    nsTArray<TextRange> mRanges;
};


#define MAX_ITEM_LENGTH 32768



static PRUint32 FindNextItemStart(int aOffset, int aLimit,
                                  nsTArray<SCRIPT_LOGATTR> &aLogAttr,
                                  const PRUnichar *aString)
{
    if (aOffset + MAX_ITEM_LENGTH >= aLimit) {
        
        
        return aLimit;
    }

    
    
    PRUint32 off;
    int boundary = -1;
    for (off = MAX_ITEM_LENGTH; off > 1; --off) {
      if (aLogAttr[off].fCharStop) {
          if (off > boundary) {
              boundary = off;
          }
          if (aString[aOffset+off] == ' ' || aString[aOffset+off - 1] == ' ')
            return aOffset+off;
      }
    }

    
    if (boundary > 0) {
      return aOffset+boundary;
    }

    
    
    
    return aOffset + MAX_ITEM_LENGTH;
}

class Uniscribe
{
public:
    Uniscribe(gfxContext *aContext, HDC aDC, const PRUnichar *aString, PRUint32 aLength, PRBool aIsRTL) :
        mContext(aContext), mDC(aDC), mString(aString), mLength(aLength), mIsRTL(aIsRTL),
        mItems(nsnull) {
    }
    ~Uniscribe() {
    }

    void Init() {
        memset(&mControl, 0, sizeof(SCRIPT_CONTROL));
        memset(&mState, 0, sizeof(SCRIPT_STATE));
        
        
        mState.uBidiLevel = mIsRTL;
        mState.fOverrideDirection = PR_TRUE;
    }

private:

    
    
    nsresult CopyItemSplitOversize(int aIndex, nsTArray<SCRIPT_ITEM> &aDest) {
        aDest.AppendElement(mItems[aIndex]);
        const int itemLength = mItems[aIndex+1].iCharPos - mItems[aIndex].iCharPos;
        if (ESTIMATE_MAX_GLYPHS(itemLength) > 65535) {
            
            

            
            nsTArray<SCRIPT_LOGATTR> logAttr;
            if (!logAttr.SetLength(itemLength))
                return NS_ERROR_FAILURE;
            HRESULT rv= ScriptBreak(mString+mItems[aIndex].iCharPos, itemLength,
                                    &mItems[aIndex].a, logAttr.Elements());
            if (FAILED(rv))
                return NS_ERROR_FAILURE;

            const int nextItemStart = mItems[aIndex+1].iCharPos;
            int start = FindNextItemStart(mItems[aIndex].iCharPos,
                                          nextItemStart, logAttr, mString);

            while (start < nextItemStart) {
                SCRIPT_ITEM item = mItems[aIndex];
                item.iCharPos = start;
                aDest.AppendElement(item);
                start = FindNextItemStart(start, nextItemStart, logAttr, mString);
            }
        } 
        return NS_OK;
    }

public:

    int Itemize() {
        HRESULT rv;

        int maxItems = 5;

        Init();

        
        
        if (!mItems.SetLength(maxItems + 1)) {
            return 0;
        }
        while ((rv = ScriptItemize(mString, mLength, maxItems, &mControl, &mState,
                                   mItems.Elements(), &mNumItems)) == E_OUTOFMEMORY) {
            maxItems *= 2;
            if (!mItems.SetLength(maxItems + 1)) {
                return 0;
            }
            Init();
        }

        if (ESTIMATE_MAX_GLYPHS(mLength) > 65535) {
            
            
            
            
            nsTArray<SCRIPT_ITEM> items;
            for (int i=0; i<mNumItems; i++) {
                nsresult nrs = CopyItemSplitOversize(i, items);
                NS_ASSERTION(NS_SUCCEEDED(nrs), "CopyItemSplitOversize() failed");
            }
            items.AppendElement(mItems[mNumItems]); 

            mItems = items;
            mNumItems = items.Length() - 1; 
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

    SCRIPT_CONTROL mControl;
    SCRIPT_STATE   mState;
    nsTArray<SCRIPT_ITEM> mItems;
    int mNumItems;
};

void
gfxWindowsFontGroup::InitTextRunUniscribe(gfxContext *aContext, gfxTextRun *aRun, const PRUnichar *aString,
                                          PRUint32 aLength)
{
    DCFromContext aDC(aContext);
 
    const PRBool isRTL = aRun->IsRightToLeft();

    HRESULT rv;

    Uniscribe us(aContext, aDC, aString, aLength, isRTL);

    
    int numItems = us.Itemize();

    for (int i = 0; i < numItems; ++i) {
        SaveDC(aDC);

        nsAutoPtr<UniscribeItem> item(us.GetItem(i, this));

        PRUint32 nranges = item->ComputeRanges();

        for (PRUint32 j = 0; j < nranges; ++j) {

            item->SetRange(j);

            if (!item->ShapingEnabled())
                item->EnableShaping();

            rv = item->Shape();
            if (FAILED(rv)) {
                PR_LOG(gFontLog, PR_LOG_DEBUG, ("shaping failed"));
                
                
                
                item->DisableShaping();
                rv = item->Shape();
            }

            NS_ASSERTION(SUCCEEDED(rv), "Failed to shape, twice -- we should never hit this");

            if (SUCCEEDED(rv)) {
                rv = item->Place();
                NS_ASSERTION(SUCCEEDED(rv), "Failed to place -- this is pretty bad.");
            }

            if (FAILED(rv)) {
                aRun->ResetGlyphRuns();

                
                item->GetCurrentFont()->GetFontEntry()->mForceGDI = PR_TRUE;
                break;
            }

            item->SaveGlyphs(aRun);
        }

        RestoreDC(aDC, -1);

        if (FAILED(rv)) {
            i = -1;
        }
    }
}
