








































#include "gfxContext.h"

#include "gfxOS2Platform.h"
#include "gfxOS2Surface.h"
#include "gfxOS2Fonts.h"
#include "nsTArray.h"

#include "nsIServiceManager.h"
#include "nsIPlatformCharset.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"





gfxOS2Font::gfxOS2Font(gfxOS2FontEntry *aFontEntry, const gfxFontStyle *aFontStyle)
    : gfxFont(aFontEntry, aFontStyle),
      mFontFace(nsnull), mScaledFont(nsnull),
      mMetrics(nsnull), mAdjustedSize(0),
      mHinting(FC_HINT_MEDIUM), mAntialias(FcTrue)
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Font[%p]::gfxOS2Font(%p \"%s\", aFontStyle)\n",
           (void *)this, (void *)aFontEntry,
           NS_LossyConvertUTF16toASCII(aFontEntry->Name()).get());
#endif
    
    nsCOMPtr<nsIPrefBranch> prefbranch = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefbranch) {
        int value;
        nsresult rv = prefbranch->GetIntPref("gfx.os2.font.hinting", &value);
        if (NS_SUCCEEDED(rv) && value >= FC_HINT_NONE && value <= FC_HINT_FULL)
            mHinting = value;

        PRBool enabled;
        rv = prefbranch->GetBoolPref("gfx.os2.font.antialiasing", &enabled);
        if (NS_SUCCEEDED(rv))
            mAntialias = enabled;
    }
#ifdef DEBUG_thebes_2
    printf("  font display options: hinting=%d, antialiasing=%s\n",
           mHinting, mAntialias ? "on" : "off");
#endif
}

gfxOS2Font::~gfxOS2Font()
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Font[%#x]::~gfxOS2Font()\n", (unsigned)this);
#endif
    if (mFontFace) {
        cairo_font_face_destroy(mFontFace);
    }
    if (mScaledFont) {
        cairo_scaled_font_destroy(mScaledFont);
    }
    if (mMetrics) {
        delete mMetrics;
    }
    mFontFace = nsnull;
    mScaledFont = nsnull;
    mMetrics = nsnull;
}


static void FillMetricsDefaults(gfxFont::Metrics *aMetrics)
{
    aMetrics->emAscent = 0.8 * aMetrics->emHeight;
    aMetrics->emDescent = 0.2 * aMetrics->emHeight;
    aMetrics->maxAscent = aMetrics->emAscent;
    aMetrics->maxDescent = aMetrics->maxDescent;
    aMetrics->maxHeight = aMetrics->emHeight;
    aMetrics->internalLeading = 0.0;
    aMetrics->externalLeading = 0.2 * aMetrics->emHeight;
    aMetrics->spaceWidth = 0.5 * aMetrics->emHeight;
    aMetrics->maxAdvance = aMetrics->spaceWidth;
    aMetrics->aveCharWidth = aMetrics->spaceWidth;
    aMetrics->zeroOrAveCharWidth = aMetrics->spaceWidth;
    aMetrics->xHeight = 0.5 * aMetrics->emHeight;
    aMetrics->underlineSize = aMetrics->emHeight / 14.0;
    aMetrics->underlineOffset = -aMetrics->underlineSize;
    aMetrics->strikeoutOffset = 0.25 * aMetrics->emHeight;
    aMetrics->strikeoutSize = aMetrics->underlineSize;
    aMetrics->superscriptOffset = aMetrics->xHeight;
    aMetrics->subscriptOffset = aMetrics->xHeight;
}



static void SnapLineToPixels(gfxFloat& aOffset, gfxFloat& aSize)
{
    gfxFloat snappedSize = PR_MAX(NS_floor(aSize + 0.5), 1.0);
    
    gfxFloat offset = aOffset - 0.5 * (aSize - snappedSize);
    
    aOffset = NS_floor(offset + 0.5);
    aSize = snappedSize;
}






const gfxFont::Metrics& gfxOS2Font::GetMetrics()
{
#ifdef DEBUG_thebes_1
    printf("gfxOS2Font[%#x]::GetMetrics()\n", (unsigned)this);
#endif
    if (mMetrics) {
        return *mMetrics;
    }

    
    mMetrics = new gfxFont::Metrics;
    mSpaceGlyph = 0;

    
    
    mMetrics->emHeight = NS_floor(GetStyle()->size + 0.5);

    FT_Face face = cairo_ft_scaled_font_lock_face(CairoScaledFont());
    if (!face) {
        
        
        FillMetricsDefaults(mMetrics);
        return *mMetrics;
    }
    if (!face->charmap) {
        
        
        
        cairo_ft_scaled_font_unlock_face(CairoScaledFont());
        FillMetricsDefaults(mMetrics);
        return *mMetrics;
    }

    
    gfxFloat emUnit = 1.0 * face->units_per_EM;
    gfxFloat xScale = face->size->metrics.x_ppem / emUnit;
    gfxFloat yScale = face->size->metrics.y_ppem / emUnit;

    FT_UInt gid; 

    
    gid = FT_Get_Char_Index(face, ' ');
    if (gid) {
        
        
        
        FT_Load_Glyph(face, gid, FT_LOAD_DEFAULT);
        
        mMetrics->spaceWidth = face->glyph->advance.x >> 6;
        
        mSpaceGlyph = gid;
    } else {
        NS_ASSERTION(gid, "this font doesn't have a space glyph!");
        mMetrics->spaceWidth = face->max_advance_width * xScale;
    }

    
    gid = FT_Get_Char_Index(face, 'x'); 
    if (gid) {
        
        FT_Load_Glyph(face, gid, FT_LOAD_NO_SCALE);
        mMetrics->xHeight = face->glyph->metrics.height * yScale;
        mMetrics->aveCharWidth = face->glyph->metrics.horiAdvance * xScale;
    } else {
        
        
        mMetrics->xHeight = mMetrics->emHeight * 0.5;
        mMetrics->aveCharWidth = mMetrics->emHeight * 0.5;
    }

    
    gid = FT_Get_Char_Index(face, '0');
    if (gid) {
        FT_Load_Glyph(face, gid, FT_LOAD_NO_SCALE);
        mMetrics->zeroOrAveCharWidth = face->glyph->metrics.horiAdvance * xScale;
    } else {
        
        mMetrics->zeroOrAveCharWidth = mMetrics->aveCharWidth;
    }

    
    if (mAdjustedSize == 0 && GetStyle()->sizeAdjust != 0) {
        gfxFloat aspect = mMetrics->xHeight / GetStyle()->size;
        mAdjustedSize = GetStyle()->GetAdjustedSize(aspect);
        mMetrics->emHeight = mAdjustedSize;
    }

    
    TT_OS2 *os2 = (TT_OS2 *)FT_Get_Sfnt_Table(face, ft_sfnt_os2);
    if (os2 && os2->version != 0xFFFF) { 
        
        mMetrics->aveCharWidth = PR_MAX(mMetrics->aveCharWidth,
                                        os2->xAvgCharWidth * xScale);

        mMetrics->superscriptOffset = PR_MAX(os2->ySuperscriptYOffset * yScale, 1.0);
        
        mMetrics->subscriptOffset   = PR_MAX(fabs(os2->ySubscriptYOffset * yScale),
                                             1.0);
        mMetrics->strikeoutOffset   = os2->yStrikeoutPosition * yScale;
        mMetrics->strikeoutSize     = os2->yStrikeoutSize * yScale;
    } else {
        
        mMetrics->superscriptOffset = mMetrics->emHeight * 0.5;
        mMetrics->subscriptOffset   = mMetrics->emHeight * 0.2;
        mMetrics->strikeoutOffset   = mMetrics->emHeight * 0.3;
        mMetrics->strikeoutSize     = face->underline_thickness * yScale;
    }
    SnapLineToPixels(mMetrics->strikeoutOffset, mMetrics->strikeoutSize);

    
    mMetrics->underlineOffset = face->underline_position * yScale;
    mMetrics->underlineSize   = face->underline_thickness * yScale;

    
    mMetrics->emAscent        = face->ascender * yScale;
    mMetrics->emDescent       = -face->descender * yScale;
    mMetrics->maxHeight       = face->height * yScale;
    
    mMetrics->maxAscent       = PR_MAX(face->bbox.yMax * yScale,
                                       mMetrics->emAscent);
    mMetrics->maxDescent      = PR_MAX(-face->bbox.yMin * yScale,
                                       mMetrics->emDescent);
    mMetrics->maxAdvance      = PR_MAX(face->max_advance_width * xScale,
                                       mMetrics->aveCharWidth);

    
    
    
    mMetrics->internalLeading = NS_floor(mMetrics->maxHeight
                                         - mMetrics->emHeight + 0.5);
    gfxFloat lineHeight = NS_floor(mMetrics->maxHeight + 0.5);
    mMetrics->externalLeading = lineHeight
                              - mMetrics->internalLeading - mMetrics->emHeight;

    SanitizeMetrics(mMetrics, PR_FALSE);

#ifdef DEBUG_thebes_1
    printf("gfxOS2Font[%#x]::GetMetrics():\n"
           "  %s (%s)\n"
           "  emHeight=%f == %f=gfxFont::style.size == %f=adjSz\n"
           "  maxHeight=%f  xHeight=%f\n"
           "  aveCharWidth=%f(x) zeroOrAveWidth=%f(0) spaceWidth=%f\n"
           "  supOff=%f SubOff=%f   strOff=%f strSz=%f\n"
           "  undOff=%f undSz=%f    intLead=%f extLead=%f\n"
           "  emAsc=%f emDesc=%f maxH=%f\n"
           "  maxAsc=%f maxDes=%f maxAdv=%f\n",
           (unsigned)this,
           NS_LossyConvertUTF16toASCII(GetName()).get(),
           os2 && os2->version != 0xFFFF ? "has OS/2 table" : "no OS/2 table!",
           mMetrics->emHeight, GetStyle()->size, mAdjustedSize,
           mMetrics->maxHeight, mMetrics->xHeight,
           mMetrics->aveCharWidth, mMetrics->zeroOrAveCharWidth, mMetrics->spaceWidth,
           mMetrics->superscriptOffset, mMetrics->subscriptOffset,
           mMetrics->strikeoutOffset, mMetrics->strikeoutSize,
           mMetrics->underlineOffset, mMetrics->underlineSize,
           mMetrics->internalLeading, mMetrics->externalLeading,
           mMetrics->emAscent, mMetrics->emDescent, mMetrics->maxHeight,
           mMetrics->maxAscent, mMetrics->maxDescent, mMetrics->maxAdvance
          );
#endif
    cairo_ft_scaled_font_unlock_face(CairoScaledFont());
    return *mMetrics;
}



static const PRInt8 nFcWeight = 2; 
static const int fcWeight[] = {
    
    
    
    
    FC_WEIGHT_REGULAR, 
    
    
    FC_WEIGHT_BOLD,
    
    
};





cairo_font_face_t *gfxOS2Font::CairoFontFace()
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Font[%#x]::CairoFontFace()\n", (unsigned)this);
#endif
    if (!mFontFace) {
#ifdef DEBUG_thebes
        printf("gfxOS2Font[%#x]::CairoFontFace(): create it for %s, %f\n",
               (unsigned)this, NS_LossyConvertUTF16toASCII(GetName()).get(), GetStyle()->size);
#endif
        FcPattern *fcPattern = FcPatternCreate();

        
        
        FcPatternAddString(fcPattern, FC_FAMILY,
                           (FcChar8 *)NS_ConvertUTF16toUTF8(GetName()).get());

        
        
        
        
        PRInt8 weight, offset;
        GetStyle()->ComputeWeightAndOffset(&weight, &offset);
        
        
        
        PRInt16 fcW = 40 * weight - 80; 
        
        PRInt8 i = 0;
        while (i < nFcWeight && fcWeight[i] < fcW) {
            i++;
        }
        
        i += offset;
        if (i < 0) {
            i = 0;
        } else if (i >= nFcWeight) {
            i = nFcWeight - 1;
        }
        fcW = fcWeight[i];

        
        FcPatternAddInteger(fcPattern, FC_WEIGHT, fcW);

        PRUint8 fcProperty;
        
        switch (GetStyle()->style) {
        case FONT_STYLE_ITALIC:
            fcProperty = FC_SLANT_ITALIC;
            break;
        case FONT_STYLE_OBLIQUE:
            fcProperty = FC_SLANT_OBLIQUE;
            break;
        case FONT_STYLE_NORMAL:
        default:
            fcProperty = FC_SLANT_ROMAN;
        }
        FcPatternAddInteger(fcPattern, FC_SLANT, fcProperty);

        
        FcPatternAddDouble(fcPattern, FC_PIXEL_SIZE,
                           mAdjustedSize ? mAdjustedSize : GetStyle()->size);

        
        FcResult fcRes;
        FcPattern *fcMatch = FcFontMatch(NULL, fcPattern, &fcRes);
        FcPatternDestroy(fcPattern);

        if (fcMatch) {
            int w = FC_WEIGHT_REGULAR;
            FcPatternGetInteger(fcMatch, FC_WEIGHT, 0, &w);
            if (fcW >= FC_WEIGHT_DEMIBOLD && w < FC_WEIGHT_DEMIBOLD) {
                
                
                FcPatternAddBool(fcMatch, FC_EMBOLDEN, FcTrue);
            }
            FcPatternAddBool(fcMatch, FC_ANTIALIAS, mAntialias);
            FcPatternAddInteger(fcMatch, FC_HINT_STYLE, mHinting);

            
            mFontFace = cairo_ft_font_face_create_for_pattern(fcMatch);

            FcPatternDestroy(fcMatch);
        } else {
#ifdef DEBUG
            printf("Could not match font for:\n"
                   "  family=%s, weight=%d, slant=%d, size=%f\n",
                   NS_LossyConvertUTF16toASCII(GetName()).get(),
                   GetStyle()->weight, GetStyle()->style, GetStyle()->size);
#endif
        }
    }

    NS_ASSERTION(mFontFace, "Failed to make font face");
    return mFontFace;
}

cairo_scaled_font_t *gfxOS2Font::CairoScaledFont()
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Font[%#x]::CairoScaledFont()\n", (unsigned)this);
#endif
    if (mScaledFont) {
        return mScaledFont;
    }
#ifdef DEBUG_thebes_2
    printf("gfxOS2Font[%#x]::CairoScaledFont(): create it for %s, %f\n",
           (unsigned)this, NS_LossyConvertUTF16toASCII(GetName()).get(), GetStyle()->size);
#endif

    double size = mAdjustedSize ? mAdjustedSize : GetStyle()->size;
    cairo_matrix_t identityMatrix;
    cairo_matrix_init_identity(&identityMatrix);
    cairo_matrix_t fontMatrix;
    
    if (!mFontEntry->mItalic &&
        (mStyle.style & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE)))
    {
        const double kSkewFactor = 0.2126; 
        cairo_matrix_init(&fontMatrix, size, 0, -kSkewFactor*size, size, 0, 0);
    } else {
        cairo_matrix_init_scale(&fontMatrix, size, size);
    }
    cairo_font_options_t *fontOptions = cairo_font_options_create();
    mScaledFont = cairo_scaled_font_create(CairoFontFace(), &fontMatrix,
                                           &identityMatrix, fontOptions);
    cairo_font_options_destroy(fontOptions);

    NS_ASSERTION(cairo_scaled_font_status(mScaledFont) == CAIRO_STATUS_SUCCESS,
                 "Failed to make scaled font");
    return mScaledFont;
}

nsString gfxOS2Font::GetUniqueName()
{
#ifdef DEBUG_thebes
    printf("gfxOS2Font::GetUniqueName()=%s\n", (char *)GetName().get());
#endif
    
    
    
    return GetName();
}

PRBool gfxOS2Font::SetupCairoFont(gfxContext *aContext)
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Font[%#x]::SetupCairoFont(%#x)\n",
           (unsigned)this, (unsigned) aContext);
#endif
    

    
    cairo_scaled_font_t *scaledFont = CairoScaledFont();
    if (cairo_scaled_font_status(scaledFont) != CAIRO_STATUS_SUCCESS) {
        
        
        return PR_FALSE;
    }
    cairo_set_scaled_font(aContext->GetCairo(), scaledFont);
    return PR_TRUE;
}






already_AddRefed<gfxOS2Font> gfxOS2Font::GetOrMakeFont(const nsAString& aName,
                                                       const gfxFontStyle *aStyle)
{
    nsRefPtr<gfxFont> font = gfxFontCache::GetCache()->Lookup(aName, aStyle);
    if (!font) {
        nsRefPtr<gfxOS2FontEntry> fe = new gfxOS2FontEntry(aName);
        font = new gfxOS2Font(fe, aStyle);
        if (!font)
            return nsnull;
        gfxFontCache::GetCache()->AddNew(font);
    }
    gfxFont *f = nsnull;
    font.swap(f);
    return static_cast<gfxOS2Font *>(f);
}





gfxOS2FontGroup::gfxOS2FontGroup(const nsAString& aFamilies,
                                 const gfxFontStyle* aStyle,
                                 gfxUserFontSet *aUserFontSet)
    : gfxFontGroup(aFamilies, aStyle, aUserFontSet)
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2FontGroup[%#x]::gfxOS2FontGroup(\"%s\", %#x)\n",
           (unsigned)this, NS_LossyConvertUTF16toASCII(aFamilies).get(),
           (unsigned)aStyle);
#endif

    
    
    int pos = 0;
    if ((pos = mFamilies.Find("WarpSans", PR_FALSE, 0, -1)) > -1) {
        mFamilies.Replace(pos, 8, NS_LITERAL_STRING("Workplace Sans"));
    }

    nsTArray<nsString> familyArray;
    ForEachFont(FontCallback, &familyArray);

    
    
    
    
    nsString fontString;
    gfxPlatform::GetPlatform()->GetPrefFonts("x-unicode", fontString, PR_FALSE);
    ForEachFont(fontString, NS_LITERAL_CSTRING("x-unicode"), FontCallback, &familyArray);
    gfxPlatform::GetPlatform()->GetPrefFonts("x-user-def", fontString, PR_FALSE);
    ForEachFont(fontString, NS_LITERAL_CSTRING("x-user-def"), FontCallback, &familyArray);

    
    
    
    if (familyArray.Length() == 0) {
        familyArray.AppendElement(NS_LITERAL_STRING("Helv"));
    }

    for (PRUint32 i = 0; i < familyArray.Length(); i++) {
        nsRefPtr<gfxOS2Font> font = gfxOS2Font::GetOrMakeFont(familyArray[i], &mStyle);
        if (font) {
            mFonts.AppendElement(font);
        }
    }
}

gfxOS2FontGroup::~gfxOS2FontGroup()
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2FontGroup[%#x]::~gfxOS2FontGroup()\n", (unsigned)this);
#endif
}

gfxFontGroup *gfxOS2FontGroup::Copy(const gfxFontStyle *aStyle)
{
    return new gfxOS2FontGroup(mFamilies, aStyle, mUserFontSet);
}






static PRInt32 AppendDirectionalIndicatorUTF8(PRBool aIsRTL, nsACString& aString)
{
    static const PRUnichar overrides[2][2] = { { 0x202d, 0 }, { 0x202e, 0 }}; 
    AppendUTF16toUTF8(overrides[aIsRTL], aString);
    return 3; 
}

gfxTextRun *gfxOS2FontGroup::MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                                         const Parameters* aParams, PRUint32 aFlags)
{
    NS_ASSERTION(aLength > 0, "should use MakeEmptyTextRun for zero-length text");
    gfxTextRun *textRun = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!textRun)
        return nsnull;

    mEnableKerning = !(aFlags & gfxTextRunFactory::TEXT_OPTIMIZE_SPEED);

    nsCAutoString utf8;
    PRInt32 headerLen = AppendDirectionalIndicatorUTF8(textRun->IsRightToLeft(), utf8);
    AppendUTF16toUTF8(Substring(aString, aString + aLength), utf8);

#ifdef DEBUG_thebes_2
    NS_ConvertUTF8toUTF16 u16(utf8);
    printf("gfxOS2FontGroup[%#x]::MakeTextRun(PRUnichar %s, %d, %#x, %d)\n",
           (unsigned)this, NS_LossyConvertUTF16toASCII(u16).get(), aLength, (unsigned)aParams, aFlags);
#endif

    InitTextRun(textRun, (PRUint8 *)utf8.get(), utf8.Length(), headerLen);

    textRun->FetchGlyphExtents(aParams->mContext);

    return textRun;
}

gfxTextRun *gfxOS2FontGroup::MakeTextRun(const PRUint8* aString, PRUint32 aLength,
                                         const Parameters* aParams, PRUint32 aFlags)
{
#ifdef DEBUG_thebes_2
    const char *cStr = reinterpret_cast<const char *>(aString);
    NS_ConvertASCIItoUTF16 us(cStr, aLength);
    printf("gfxOS2FontGroup[%#x]::MakeTextRun(PRUint8 %s, %d, %#x, %d)\n",
           (unsigned)this, NS_LossyConvertUTF16toASCII(us).get(), aLength, (unsigned)aParams, aFlags);
#endif
    NS_ASSERTION(aLength > 0, "should use MakeEmptyTextRun for zero-length text");
    NS_ASSERTION(aFlags & TEXT_IS_8BIT, "8bit should have been set");
    gfxTextRun *textRun = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!textRun)
        return nsnull;

    mEnableKerning = !(aFlags & gfxTextRunFactory::TEXT_OPTIMIZE_SPEED);

    const char *chars = reinterpret_cast<const char *>(aString);
    PRBool isRTL = textRun->IsRightToLeft();
    if ((aFlags & TEXT_IS_ASCII) && !isRTL) {
        
        
        InitTextRun(textRun, (PRUint8 *)chars, aLength, 0);
    } else {
        
        
        
        NS_ConvertASCIItoUTF16 unicodeString(chars, aLength);
        nsCAutoString utf8;
        PRInt32 headerLen = AppendDirectionalIndicatorUTF8(isRTL, utf8);
        AppendUTF16toUTF8(unicodeString, utf8);
        InitTextRun(textRun, (PRUint8 *)utf8.get(), utf8.Length(), headerLen);
    }

    textRun->FetchGlyphExtents(aParams->mContext);

    return textRun;
}

void gfxOS2FontGroup::InitTextRun(gfxTextRun *aTextRun, const PRUint8 *aUTF8Text,
                                  PRUint32 aUTF8Length,
                                  PRUint32 aUTF8HeaderLength)
{
    CreateGlyphRunsFT(aTextRun, aUTF8Text + aUTF8HeaderLength,
                      aUTF8Length - aUTF8HeaderLength);
}




PRUint32 getUTF8CharAndNext(const PRUint8 *aString, PRUint8 *aLength)
{
    *aLength = 1;
    if (aString[0] < 0x80) { 
        return aString[0];
    }
    if ((aString[0] >> 5) == 6) { 
        *aLength = 2;
        return ((aString[0] & 0x1F) << 6) + (aString[1] & 0x3F);
    }
    if ((aString[0] >> 4) == 14) { 
        *aLength = 3;
        return ((aString[0] & 0x0F) << 12) + ((aString[1] & 0x3F) << 6) +
               (aString[2] & 0x3F);
    }
    if ((aString[0] >> 4) == 15) { 
        *aLength = 4;
        return ((aString[0] & 0x07) << 18) + ((aString[1] & 0x3F) << 12) +
               ((aString[2] & 0x3F) <<  6) + (aString[3] & 0x3F);
    }
    return aString[0];
}

void gfxOS2FontGroup::CreateGlyphRunsFT(gfxTextRun *aTextRun, const PRUint8 *aUTF8,
                                        PRUint32 aUTF8Length)
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2FontGroup::CreateGlyphRunsFT(%#x, _aUTF8_, %d)\n",
           (unsigned)aTextRun,  aUTF8Length);
    for (PRUint32 i = 0; i < FontListLength(); i++) {
        gfxOS2Font *font = GetFontAt(i);
        printf("  i=%d, name=%s, size=%f\n", i, NS_LossyConvertUTF16toASCII(font->GetName()).get(),
               font->GetStyle()->size);
    }
#endif
    PRUint32 lastFont = FontListLength()-1;
    gfxOS2Font *font0 = GetFontAt(0);
    const PRUint8 *p = aUTF8;
    PRUint32 utf16Offset = 0;
    gfxTextRun::CompressedGlyph g;
    const PRUint32 appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();
    gfxOS2Platform *platform = gfxOS2Platform::GetPlatform();

    aTextRun->AddGlyphRun(font0, 0);
    
    
    FT_Face face0 = cairo_ft_scaled_font_lock_face(font0->CairoScaledFont());
    while (p < aUTF8 + aUTF8Length) {
        PRBool glyphFound = PR_FALSE;
        
        PRUint8 chLen;
        PRUint32 ch = getUTF8CharAndNext(p, &chLen);
        p += chLen; 
#ifdef DEBUG_thebes_2
        printf("\'%c\' (%d, %#x, %s) [%#x %#x]:", (char)ch, ch, ch, ch >=0x10000 ? "non-BMP!" : "BMP", ch >=0x10000 ? H_SURROGATE(ch) : 0, ch >=0x10000 ? L_SURROGATE(ch) : 0);
#endif

        if (ch == 0 || platform->noFontWithChar(ch)) {
            
            aTextRun->SetMissingGlyph(utf16Offset, ch);
        } else {
            
            
            
            
            
            for (PRUint32 i = 0; i <= lastFont; i++) {
                gfxOS2Font *font = font0;
                FT_Face face = face0;
                if (i > 0) {
                    font = GetFontAt(i);
                    face = cairo_ft_scaled_font_lock_face(font->CairoScaledFont());
#ifdef DEBUG_thebes_2
                    if (i == lastFont) {
                        printf("Last font %d (%s) for ch=%#x (pos=%d)",
                               i, NS_LossyConvertUTF16toASCII(font->GetName()).get(), ch, utf16Offset);
                    }
#endif
                }
                if (!face || !face->charmap) { 
                    if (face && face != face0)
                        cairo_ft_scaled_font_unlock_face(font->CairoScaledFont());
                    continue; 
                }

                NS_ASSERTION(!IsInvalidChar(ch), "Invalid char detected");
                FT_UInt gid = FT_Get_Char_Index(face, ch); 

                if (gid == 0 && i == lastFont) {
                    
                    nsRefPtr<gfxOS2Font> fontX = platform->FindFontForChar(ch, font0);
                    if (fontX) {
                        font = fontX; 
                        cairo_ft_scaled_font_unlock_face(font->CairoScaledFont());
                        face = cairo_ft_scaled_font_lock_face(fontX->CairoScaledFont());
                        gid = FT_Get_Char_Index(face, ch);
                        
                        
                        mFonts.AppendElement(fontX);
                        lastFont = FontListLength()-1;
                    }
                }

                
                aTextRun->AddGlyphRun(font, utf16Offset);

                PRInt32 advance = 0;
                if (gid == font->GetSpaceGlyph()) {
                    advance = (int)(font->GetMetrics().spaceWidth * appUnitsPerDevUnit);
                } else if (gid == 0) {
                    advance = -1; 
                } else {
                    
                    
                    PRUint32 chNext = 0;
                    FT_UInt gidNext = 0;
                    FT_Pos lsbDeltaNext = 0;
#ifdef DEBUG_thebes_2
                    printf("(kerning=%s/%s)", mEnableKerning ? "enable" : "disable", FT_HAS_KERNING(face) ? "yes" : "no");
#endif
                    if (mEnableKerning && FT_HAS_KERNING(face) && p < aUTF8 + aUTF8Length) {
                        chNext = getUTF8CharAndNext(p, &chLen);
                        if (chNext) {
                            gidNext = FT_Get_Char_Index(face, chNext);
                            if (gidNext && gidNext != font->GetSpaceGlyph()) {
                                FT_Load_Glyph(face, gidNext, FT_LOAD_DEFAULT);
                                lsbDeltaNext = face->glyph->lsb_delta;
                            }
                        }
                    }

                    
                    FT_Load_Glyph(face, gid, FT_LOAD_DEFAULT); 
                    advance = face->glyph->advance.x;

                    
                    if (chNext && gidNext) {
                        FT_Vector kerning;
                        FT_Get_Kerning(face, gid, gidNext, FT_KERNING_DEFAULT, &kerning);
                        advance += kerning.x;
                        if (face->glyph->rsb_delta - lsbDeltaNext >= 32) {
                            advance -= 64;
                        } else if (face->glyph->rsb_delta - lsbDeltaNext < -32) {
                            advance += 64;
                        }
                    }

                    
                    advance = (advance >> 6) * appUnitsPerDevUnit;
                }
#ifdef DEBUG_thebes_2
                printf(" gid=%d, advance=%d (%s)\n", gid, advance,
                       NS_LossyConvertUTF16toASCII(font->GetName()).get());
#endif

                if (advance >= 0 &&
                    gfxTextRun::CompressedGlyph::IsSimpleAdvance(advance) &&
                    gfxTextRun::CompressedGlyph::IsSimpleGlyphID(gid))
                {
                    aTextRun->SetSimpleGlyph(utf16Offset,
                                             g.SetSimpleGlyph(advance, gid));
                    glyphFound = PR_TRUE;
                } else if (gid == 0) {
                    
                    if (i == lastFont) {
                        
                        
                        aTextRun->SetMissingGlyph(utf16Offset, ch);
                    }
                    glyphFound = PR_FALSE;
                } else {
                    gfxTextRun::DetailedGlyph details;
                    details.mGlyphID = gid;
                    NS_ASSERTION(details.mGlyphID == gid, "Seriously weird glyph ID detected!");
                    details.mAdvance = advance;
                    details.mXOffset = 0;
                    details.mYOffset = 0;
                    g.SetComplex(aTextRun->IsClusterStart(utf16Offset), PR_TRUE, 1);
                    aTextRun->SetGlyphs(utf16Offset, g, &details);
                    glyphFound = PR_TRUE;
                }

                if (i > 0) {
                    cairo_ft_scaled_font_unlock_face(font->CairoScaledFont());
                }

                if (glyphFound) {
                    break;
                }
            }
        } 

        NS_ASSERTION(!IS_SURROGATE(ch), "Surrogates shouldn't appear in UTF8");
        if (ch >= 0x10000) {
            
            ++utf16Offset;
        }

        ++utf16Offset;
    }
    cairo_ft_scaled_font_unlock_face(font0->CairoScaledFont());
}


PRBool gfxOS2FontGroup::FontCallback(const nsAString& aFontName,
                                     const nsACString& aGenericName,
                                     void *aClosure)
{
    nsTArray<nsString> *sa = static_cast<nsTArray<nsString>*>(aClosure);
    if (!aFontName.IsEmpty() && !sa->Contains(aFontName)) {
        sa->AppendElement(aFontName);
    }
    return PR_TRUE;
}
