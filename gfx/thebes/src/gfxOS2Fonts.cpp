








































#include "gfxContext.h"

#include "gfxOS2Platform.h"
#include "gfxOS2Surface.h"
#include "gfxOS2Fonts.h"

#include "nsIServiceManager.h"
#include "nsIPlatformCharset.h"





gfxOS2Font::gfxOS2Font(const nsAString &aName, const gfxFontStyle *aFontStyle)
    : gfxFont(aName, aFontStyle),
      mFontFace(nsnull), mScaledFont(nsnull),
      mMetrics(nsnull)
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Font[%#x]::gfxOS2Font(\"%s\", aFontStyle)\n",
           (unsigned)this, NS_LossyConvertUTF16toASCII(aName).get());
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




#define MOZ_FT_ROUND(x) (((x) + 32) & ~63) // 63 = 2^6 - 1
#define MOZ_FT_TRUNC(x) ((x) >> 6)
#define CONVERT_DESIGN_UNITS_TO_PIXELS(v, s) \
        MOZ_FT_TRUNC(MOZ_FT_ROUND(FT_MulFix((v), (s))))

const gfxFont::Metrics& gfxOS2Font::GetMetrics()
{
#ifdef DEBUG_thebes_1
    printf("gfxOS2Font[%#x]::GetMetrics()\n", (unsigned)this);
#endif
    if (!mMetrics) {
        mMetrics = new gfxFont::Metrics;
    
        
        
        FT_UInt gid;
        FT_Face face = cairo_ft_scaled_font_lock_face(CairoScaledFont());

#if 0
        
        
        gid = FT_Get_Char_Index(face, 'm'); 
        FT_Load_Glyph(face, gid, FT_LOAD_DEFAULT); 
        gfxFloat scale = face->glyph->metrics.height / mStyle.size;
        
#endif
        
        
        gfxFloat scale = face->units_per_EM / 8;
    
        
        gid = FT_Get_Char_Index(face, 'x'); 
        FT_Load_Glyph(face, gid, FT_LOAD_DEFAULT); 
        mMetrics->xHeight = face->glyph->metrics.height / scale;
        mMetrics->aveCharWidth = face->glyph->metrics.width / scale;
        
        gid = FT_Get_Char_Index(face, ' ');
        FT_Load_Glyph(face, gid, FT_LOAD_DEFAULT);
        
        
        mMetrics->spaceWidth = face->glyph->advance.x / scale * 2;
        
        mSpaceGlyph = gid;
    
        
        TT_OS2 *os2 = (TT_OS2 *)FT_Get_Sfnt_Table(face, ft_sfnt_os2);
        if (os2 && os2->version !=  0xFFFF) { 
            mMetrics->superscriptOffset = PR_MAX(1, os2->ySuperscriptYOffset) / scale;
            
            mMetrics->subscriptOffset   = PR_MAX(1, fabs(os2->ySubscriptYOffset)) / scale;
        } else {
            mMetrics->superscriptOffset = mMetrics->xHeight / scale;
            mMetrics->subscriptOffset   = mMetrics->xHeight / scale;
        }
        
        mMetrics->strikeoutOffset = mMetrics->xHeight / 2.0 / scale;
        mMetrics->strikeoutSize   = face->underline_thickness / scale;
        mMetrics->underlineOffset = face->underline_position / scale;
        mMetrics->underlineSize   = face->underline_thickness / scale;
    
        
        mMetrics->emHeight        = face->size->metrics.y_ppem;
        mMetrics->emAscent        = face->ascender / scale;
        mMetrics->emDescent       = face->descender / scale;
        mMetrics->maxHeight       = face->height / scale;
        mMetrics->maxAscent       = face->bbox.yMax / scale;
        mMetrics->maxDescent      = face->bbox.yMin / scale;
        mMetrics->maxAdvance      = face->max_advance_width / scale;
        
        mMetrics->internalLeading = (face->bbox.yMax - face->bbox.yMin
                                     - mMetrics->xHeight) / scale;
        mMetrics->externalLeading = 0; 
    
#ifdef DEBUG_thebes_1
        printf("gfxOS2Font[%#x]::GetMetrics():\n"
               "  scale=%f\n"
               "  emHeight=%f==%f=gfxFont::mStyle.size\n"
               "  maxHeight=%f\n"
               "  xHeight=%f\n"
               "  aveCharWidth=%f==xWidth\n"
               "  spaceWidth=%f\n",
               (unsigned)this,
               scale,
               mStyle.size,
               mMetrics->emHeight,
               mMetrics->maxHeight,
               mMetrics->xHeight,
               mMetrics->aveCharWidth,
               mMetrics->spaceWidth);
#endif
        cairo_ft_scaled_font_unlock_face(CairoScaledFont());
    }
    return *mMetrics;
}

cairo_font_face_t *gfxOS2Font::CairoFontFace()
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Font[%#x]::CairoFontFace()\n", (unsigned)this);
#endif
    if (!mFontFace) {
#ifdef DEBUG_thebes
        printf("gfxOS2Font[%#x]::CairoFontFace(): create it for %s, %f\n",
               (unsigned)this, NS_LossyConvertUTF16toASCII(mName).get(), mStyle.size);
#endif
        FcPattern *fcPattern = FcPatternCreate();

        
        
        FcPatternAddString(fcPattern, FC_FAMILY,
                           (FcChar8 *)NS_LossyConvertUTF16toASCII(mName).get());

        PRUint8 fcProperty;
        
        switch (mStyle.weight) {
        case FONT_WEIGHT_NORMAL:
            fcProperty = FC_WEIGHT_NORMAL;
            break;
        case FONT_WEIGHT_BOLD:
            fcProperty = FC_WEIGHT_BOLD;
            break;
        default:
            fcProperty = FC_WEIGHT_MEDIUM;
        }
        FcPatternAddInteger(fcPattern, FC_WEIGHT, fcProperty);

        
        switch (mStyle.style) {
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

        
        FcPatternAddDouble(fcPattern, FC_PIXEL_SIZE, mStyle.size);

        
        FcResult fcRes;
        FcPattern *fcMatch = FcFontMatch(NULL, fcPattern, &fcRes);
#ifdef DEBUG_thebes
        FcChar8 *str1, *str2;
        int w1, w2, i1, i2;
        double s1, s2;
        FcPatternGetString(fcPattern, FC_FAMILY, 0, &str1);
        FcPatternGetInteger(fcPattern, FC_WEIGHT, 0, &w1);
        FcPatternGetInteger(fcPattern, FC_SLANT, 0, &i1);
        FcPatternGetDouble(fcPattern, FC_PIXEL_SIZE, 0, &s1);
        FcPatternGetString(fcMatch, FC_FAMILY, 0, &str2);
        FcPatternGetInteger(fcMatch, FC_WEIGHT, 0, &w2);
        FcPatternGetInteger(fcMatch, FC_SLANT, 0, &i2);
        FcPatternGetDouble(fcMatch, FC_PIXEL_SIZE, 0, &s2);
        printf("  input=%s,%d,%d,%f\n  fcPattern=%s,%d,%d,%f\n  fcMatch=%s,%d,%d,%f\n",
               NS_LossyConvertUTF16toASCII(mName).get(), mStyle.weight, mStyle.style, mStyle.size,
               (char *)str1, w1, i1, s1,
               (char *)str2, w2, i2, s2);
#endif
        FcPatternDestroy(fcPattern);
        
        mFontFace = cairo_ft_font_face_create_for_pattern(fcMatch);
        FcPatternDestroy(fcMatch);
    }
#ifdef DEBUG_thebes_2
    printf("  cairo_font_face_type=%s (%d)\n",
           cairo_font_face_get_type(mFontFace) == CAIRO_FONT_TYPE_FT ? "FT" : "??",
           cairo_font_face_get_type(mFontFace));
#endif

    NS_ASSERTION(mFontFace, "Failed to make font face");
    return mFontFace;
}

cairo_scaled_font_t *gfxOS2Font::CairoScaledFont()
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Font[%#x]::CairoScaledFont()\n", (unsigned)this);
#endif
    if (!mScaledFont) {
#ifdef DEBUG_thebes_2
        printf("gfxOS2Font[%#x]::CairoScaledFont(): create it for %s, %f\n",
               (unsigned)this, NS_LossyConvertUTF16toASCII(mName).get(), mStyle.size);
#endif

        double size = mStyle.size;
        cairo_matrix_t fontMatrix;
        cairo_matrix_init_scale(&fontMatrix, size, size);
        cairo_font_options_t *fontOptions = cairo_font_options_create();
        mScaledFont = cairo_scaled_font_create(CairoFontFace(), &fontMatrix,
                                               reinterpret_cast<cairo_matrix_t*>(&mCTM),
                                               fontOptions);
        cairo_font_options_destroy(fontOptions);
    }

    NS_ASSERTION(mScaledFont, "Failed to make scaled font");
    return mScaledFont;
}

nsString gfxOS2Font::GetUniqueName()
{
#ifdef DEBUG_thebes
    printf("gfxOS2Font::GetUniqueName()=%s\n", (char *)mName.get());
#endif
    
    
    
    return mName;
}

void gfxOS2Font::SetupCairoFont(cairo_t *aCR)
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Font[%#x]::SetupCairoFont(%#x)\n",
           (unsigned)this, (unsigned) aCR);
#endif
    

    
    cairo_set_scaled_font(aCR, CairoScaledFont());
}




gfxOS2FontGroup::gfxOS2FontGroup(const nsAString& aFamilies,
                                 const gfxFontStyle* aStyle)
    : gfxFontGroup(aFamilies, aStyle)
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2FontGroup[%#x]::gfxOS2FontGroup(\"%s\", %#x)\n",
           (unsigned)this, NS_LossyConvertUTF16toASCII(aFamilies).get(),
           (unsigned)aStyle);
#endif

    nsStringArray familyArray;
    mFontCache.Init(15);
    ForEachFont(FontCallback, &familyArray);
    FindGenericFontFromStyle(FontCallback, &familyArray);
    if (familyArray.Count() == 0) {
        
        
        familyArray.AppendString(NS_LITERAL_STRING("WarpSans"));
    }
    for (int i = 0; i < familyArray.Count(); i++) {
        mFonts.AppendElement(new gfxOS2Font(*familyArray[i], &mStyle));
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
    return new gfxOS2FontGroup(mFamilies, aStyle);
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
#ifdef DEBUG_thebes_2
    printf("gfxOS2FontGroup[%#x]::MakeTextRun(PRUnichar aString, %d, %#x, %d)\n",
           (unsigned)this,  aLength, (unsigned)aParams, aFlags);
#endif
    NS_ASSERTION(!(aFlags & TEXT_NEED_BOUNDING_BOX), "Glyph extents not yet supported");
    gfxTextRun *textRun = new gfxTextRun(aParams, aString, aLength, this, aFlags);
    if (!textRun)
        return nsnull;
    NS_ASSERTION(aParams->mContext, "MakeTextRun called without a gfxContext");

    textRun->RecordSurrogates(aString);
    
    nsCAutoString utf8;
    PRInt32 headerLen = AppendDirectionalIndicatorUTF8(textRun->IsRightToLeft(), utf8);
    AppendUTF16toUTF8(Substring(aString, aString + aLength), utf8);
    InitTextRun(textRun, (PRUint8 *)utf8.get(), utf8.Length(), headerLen, aString, aLength);

#ifdef DEBUG_thebes_2
    printf("gfxOS2FontGroup[%#x]::MakeTextRun(PRUnichar aString, %d, %#x) is done: %#x\n",
           (unsigned)this,  aLength, (unsigned)aParams, (unsigned)textRun);
#endif
    return textRun;
}

gfxTextRun *gfxOS2FontGroup::MakeTextRun(const PRUint8* aString, PRUint32 aLength,
                                         const Parameters* aParams, PRUint32 aFlags)
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2FontGroup[%#x]::MakeTextRun(PRUint8 aString, %d, %#x, %d)\n",
           (unsigned)this,  aLength, (unsigned)aParams, aFlags);
#endif
    NS_ASSERTION(aFlags & TEXT_IS_8BIT, "should be marked 8bit");
    gfxTextRun *textRun = new gfxTextRun(aParams, aString, aLength, this, aFlags);
    if (!textRun)
        return nsnull;
    NS_ASSERTION(aParams->mContext, "MakeTextRun called without a gfxContext");

    const char *utf8Chars = NS_REINTERPRET_CAST(const char *, aString);
    PRBool isRTL = textRun->IsRightToLeft();
    if (!isRTL) {
        
        
        InitTextRun(textRun, (PRUint8 *)utf8Chars, aLength, 0, nsnull, 0);
    } else {
        NS_ConvertASCIItoUTF16 unicodeString(utf8Chars, aLength);
        nsCAutoString utf8;
        PRInt32 headerLen = AppendDirectionalIndicatorUTF8(isRTL, utf8);
        AppendUTF16toUTF8(unicodeString, utf8);
        InitTextRun(textRun, (PRUint8 *)utf8.get(), utf8.Length(), headerLen, nsnull, 0);
    }

#ifdef DEBUG_thebes_2
    printf("gfxOS2FontGroup[%#x]::MakeTextRun(PRUint8 aString, %d, %#x) is done: %#x\n",
           (unsigned)this,  aLength, (unsigned)aParams, (unsigned)textRun);
#endif
    return textRun;
}

void gfxOS2FontGroup::InitTextRun(gfxTextRun *aTextRun, const PRUint8 *aUTF8Text,
                                  PRUint32 aUTF8Length,
                                  PRUint32 aUTF8HeaderLength,
                                  const PRUnichar *aUTF16Text,
                                  PRUint32 aUTF16Length)
{
    CreateGlyphRunsFT(aTextRun, aUTF8Text + aUTF8HeaderLength,
                      aUTF8Length - aUTF8HeaderLength);
}

static void SetMissingGlyphForUCS4(gfxTextRun *aTextRun, PRUint32 aIndex,
                                   PRUint32 aCh)
{
    if (aCh < 0x10000) {
        aTextRun->SetMissingGlyph(aIndex, PRUnichar(aCh));
        return;
    }

    
    aTextRun->SetMissingGlyph(aIndex, H_SURROGATE(aCh));
    if (aIndex + 1 < aTextRun->GetLength()) {
        aTextRun->SetMissingGlyph(aIndex + 1, L_SURROGATE(aCh));
    }
}

#define IS_MISSING_GLYPH(g) (((g) & 0x10000000) || (g) == 0x0FFFFFFF || (g) == 0)




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
        return ((aString[0] & 0x1F) << 12) + ((aString[1] & 0x3F) << 6) + 
               (aString[2] & 0x3F);
    }
    if ((aString[0] >> 4) == 15) { 
        *aLength = 4;
        return ((aString[0] & 0x1F) << 18) + ((aString[1] & 0x3F) << 12) + 
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
#endif
    const PRUint8 *p = aUTF8;
    gfxOS2Font *font = GetFontAt(0);
    PRUint32 utf16Offset = 0;
    gfxTextRun::CompressedGlyph g;
    const PRUint32 appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();

    aTextRun->AddGlyphRun(font, 0);
    
    FT_Face face = cairo_ft_scaled_font_lock_face(font->CairoScaledFont());
    while (p < aUTF8 + aUTF8Length) {
        
        PRUint8 chLen;
        PRUint32 ch = getUTF8CharAndNext(p, &chLen);
        p += chLen; 
#ifdef DEBUG_thebes_2
        printf("\'%c\' (%d):", (char)ch, ch);
#endif

        if (ch == 0) {
            
            aTextRun->SetMissingGlyph(utf16Offset, 0);
        } else {
            FT_UInt gid = FT_Get_Char_Index(face, ch); 
            PRInt32 advance = 0;
            if (gid == font->GetSpaceGlyph()) {
                advance = (int)(font->GetMetrics().spaceWidth * appUnitsPerDevUnit);
            } else {
                FT_Load_Glyph(face, gid, FT_LOAD_DEFAULT); 
                advance = MOZ_FT_TRUNC(face->glyph->advance.x) * appUnitsPerDevUnit;
            }
#ifdef DEBUG_thebes_2
            printf(" gid=%d, advance=%d (%d)\n", gid, advance, appUnitsPerDevUnit);
#endif
            
            if (advance >= 0 &&
                gfxTextRun::CompressedGlyph::IsSimpleAdvance(advance) &&
                gfxTextRun::CompressedGlyph::IsSimpleGlyphID(gid))
            {
                aTextRun->SetCharacterGlyph(utf16Offset,
                                            g.SetSimpleGlyph(advance, gid));
            } else if (IS_MISSING_GLYPH(gid)) {
                
                
                SetMissingGlyphForUCS4(aTextRun, utf16Offset, ch);
            } else {
                gfxTextRun::DetailedGlyph details;
                details.mIsLastGlyph = PR_TRUE;
                details.mGlyphID = gid;
                NS_ASSERTION(details.mGlyphID == gid, "Seriously weird glyph ID detected!");
                details.mAdvance = advance;
                details.mXOffset = 0;
                details.mYOffset = 0;
                aTextRun->SetDetailedGlyphs(utf16Offset, &details, 1);
            }

            NS_ASSERTION(!IS_SURROGATE(ch), "Surrogates shouldn't appear in UTF8");
            if (ch >= 0x10000) {
                
                ++utf16Offset;
            }
        }
        ++utf16Offset;
    }
    cairo_ft_scaled_font_unlock_face(font->CairoScaledFont());
}

PRBool gfxOS2FontGroup::FontCallback(const nsAString& aFontName,
                                     const nsACString& aGenericName,
                                     void *aClosure)
{
    nsStringArray *sa = NS_STATIC_CAST(nsStringArray*, aClosure);
    if (sa->IndexOf(aFontName) < 0) {
        sa->AppendString(aFontName);
    }
    return PR_TRUE;
}
