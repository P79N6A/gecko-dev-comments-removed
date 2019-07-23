








































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
#define CONVERT_DESIGN_UNITS_TO_PIXELS_X(v) \
        CONVERT_DESIGN_UNITS_TO_PIXELS(v, face->size->metrics.x_scale)
#define CONVERT_DESIGN_UNITS_TO_PIXELS_Y(v) \
        CONVERT_DESIGN_UNITS_TO_PIXELS(v, face->size->metrics.y_scale)






const gfxFont::Metrics& gfxOS2Font::GetMetrics()
{
#ifdef DEBUG_thebes_1
    printf("gfxOS2Font[%#x]::GetMetrics()\n", (unsigned)this);
#endif
    if (!mMetrics) {
        mMetrics = new gfxFont::Metrics;
    
        FT_UInt gid;
        FT_Face face = cairo_ft_scaled_font_lock_face(CairoScaledFont());

        
        gid = FT_Get_Char_Index(face, 'x'); 
        FT_Load_Glyph(face, gid, FT_LOAD_DEFAULT); 
        mMetrics->xHeight = CONVERT_DESIGN_UNITS_TO_PIXELS_Y(face->glyph->metrics.height);
        mMetrics->aveCharWidth = CONVERT_DESIGN_UNITS_TO_PIXELS_X(face->glyph->metrics.width);
        
        gid = FT_Get_Char_Index(face, ' ');
        FT_Load_Glyph(face, gid, FT_LOAD_DEFAULT);
        
        
        mMetrics->spaceWidth = CONVERT_DESIGN_UNITS_TO_PIXELS_X(face->glyph->advance.x);
        
        mSpaceGlyph = gid;
    
        
        TT_OS2 *os2 = (TT_OS2 *)FT_Get_Sfnt_Table(face, ft_sfnt_os2);
        if (os2 && os2->version !=  0xFFFF) { 
            mMetrics->superscriptOffset = CONVERT_DESIGN_UNITS_TO_PIXELS_Y(os2->ySuperscriptYOffset);
            mMetrics->superscriptOffset = PR_MAX(1, mMetrics->superscriptOffset);
            
            mMetrics->subscriptOffset   = CONVERT_DESIGN_UNITS_TO_PIXELS_Y(os2->ySubscriptYOffset);
            mMetrics->subscriptOffset   = PR_MAX(1, fabs(mMetrics->subscriptOffset));
        } else {
            mMetrics->superscriptOffset = mMetrics->xHeight;
            mMetrics->subscriptOffset   = mMetrics->xHeight;
        }
        
        mMetrics->strikeoutOffset = mMetrics->xHeight / 2.0;
        mMetrics->strikeoutSize   = CONVERT_DESIGN_UNITS_TO_PIXELS_Y(face->underline_thickness);
        
        mMetrics->underlineOffset = CONVERT_DESIGN_UNITS_TO_PIXELS_Y(face->underline_position);
        mMetrics->underlineSize   = CONVERT_DESIGN_UNITS_TO_PIXELS_Y(face->underline_thickness);
    
        
        mMetrics->emHeight        = face->size->metrics.y_ppem;
        mMetrics->emAscent        = CONVERT_DESIGN_UNITS_TO_PIXELS_Y(face->ascender);
        mMetrics->emDescent       = -CONVERT_DESIGN_UNITS_TO_PIXELS_Y(face->descender);
        mMetrics->maxHeight       = CONVERT_DESIGN_UNITS_TO_PIXELS_Y(face->height);
        mMetrics->maxAscent       = CONVERT_DESIGN_UNITS_TO_PIXELS_Y(face->bbox.yMax);
        mMetrics->maxDescent      = -CONVERT_DESIGN_UNITS_TO_PIXELS_Y(face->bbox.yMin);
        mMetrics->maxAdvance      = CONVERT_DESIGN_UNITS_TO_PIXELS_X(face->max_advance_width);
        
        double lineHeight = mMetrics->maxAscent + mMetrics->maxDescent;
        if (lineHeight > mMetrics->emHeight) {
            mMetrics->internalLeading = lineHeight - mMetrics->emHeight;
        } else {
            mMetrics->internalLeading = 0;
        }
        mMetrics->externalLeading = 0; 
    
#ifdef DEBUG_thebes_1
        printf("gfxOS2Font[%#x]::GetMetrics():\n"
               "  %s\n"
               "  emHeight=%f==%f=gfxFont::mStyle.size\n"
               "  maxHeight=%f\n"
               "  xHeight=%f\n"
               "  aveCharWidth=%f==xWidth\n"
               "  spaceWidth=%f\n"
               "  others: %f %f   %f %f   %f %f\n      %f %f %f   %f %f %f\n      %f %f\n",
               (unsigned)this,
               NS_LossyConvertUTF16toASCII(mName).get(),
               mMetrics->emHeight, mStyle.size,
               mMetrics->maxHeight,
               mMetrics->xHeight,
               mMetrics->aveCharWidth,
               mMetrics->spaceWidth,
               mMetrics->superscriptOffset, mMetrics->subscriptOffset,
               mMetrics->strikeoutOffset, mMetrics->strikeoutSize,
               mMetrics->underlineOffset, mMetrics->underlineSize,
               mMetrics->emAscent, mMetrics->emDescent, mMetrics->maxHeight,
               mMetrics->maxAscent, mMetrics->maxDescent, mMetrics->maxAdvance,
               mMetrics->internalLeading, mMetrics->externalLeading
              );
#endif
        cairo_ft_scaled_font_unlock_face(CairoScaledFont());
    }
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
               (unsigned)this, NS_LossyConvertUTF16toASCII(mName).get(), mStyle.size);
#endif
        FcPattern *fcPattern = FcPatternCreate();

        
        
        FcPatternAddString(fcPattern, FC_FAMILY,
                           (FcChar8 *)NS_LossyConvertUTF16toASCII(mName).get());

        
        
        
        
        PRInt8 weight, offset;
        mStyle.ComputeWeightAndOffset(&weight, &offset);
        
        
        
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

PRBool gfxOS2Font::SetupCairoFont(cairo_t *aCR)
{
#ifdef DEBUG_thebes_2
    printf("gfxOS2Font[%#x]::SetupCairoFont(%#x)\n",
           (unsigned)this, (unsigned) aCR);
#endif
    

    
    cairo_scaled_font_t *scaledFont = CairoScaledFont();
    if (NS_LIKELY(scaledFont)) {
        cairo_set_scaled_font(aCR, scaledFont);
        return PR_TRUE;
    }
    return PR_FALSE;
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
    gfxTextRun *textRun = new gfxTextRun(aParams, aString, aLength, this, aFlags);
    if (!textRun)
        return nsnull;

    textRun->RecordSurrogates(aString);
    
    nsCAutoString utf8;
    PRInt32 headerLen = AppendDirectionalIndicatorUTF8(textRun->IsRightToLeft(), utf8);
    AppendUTF16toUTF8(Substring(aString, aString + aLength), utf8);
    InitTextRun(textRun, (PRUint8 *)utf8.get(), utf8.Length(), headerLen);

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
    NS_ASSERTION(aFlags & TEXT_IS_8BIT, "8bit should have been set");
    gfxTextRun *textRun = new gfxTextRun(aParams, aString, aLength, this, aFlags);
    if (!textRun)
        return nsnull;

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

#ifdef DEBUG_thebes_2
    printf("gfxOS2FontGroup[%#x]::MakeTextRun(PRUint8 aString, %d, %#x) is done: %#x\n",
           (unsigned)this,  aLength, (unsigned)aParams, (unsigned)textRun);
#endif
    return textRun;
}

void gfxOS2FontGroup::InitTextRun(gfxTextRun *aTextRun, const PRUint8 *aUTF8Text,
                                  PRUint32 aUTF8Length,
                                  PRUint32 aUTF8HeaderLength)
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
            NS_ASSERTION(!IsInvalidChar(ch), "Invalid char detected");
            FT_UInt gid = FT_Get_Char_Index(face, ch); 
            PRInt32 advance = 0;
            if (gid == font->GetSpaceGlyph()) {
                advance = (int)(font->GetMetrics().spaceWidth * appUnitsPerDevUnit);
            } else if (gid == 0) {
                advance = -1; 
            } else {
                FT_Load_Glyph(face, gid, FT_LOAD_DEFAULT); 
                advance = MOZ_FT_TRUNC(face->glyph->advance.x) * appUnitsPerDevUnit;
            }
#ifdef DEBUG_thebes_2
            printf(" gid=%d, advance=%d (%s)\n", gid, advance,
                   NS_LossyConvertUTF16toASCII(font->GetName()).get());
#endif
            
            if (advance >= 0 &&
                gfxTextRun::CompressedGlyph::IsSimpleAdvance(advance) &&
                gfxTextRun::CompressedGlyph::IsSimpleGlyphID(gid))
            {
                aTextRun->SetCharacterGlyph(utf16Offset,
                                            g.SetSimpleGlyph(advance, gid));
            } else if (gid == 0) {
                
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
    nsStringArray *sa = static_cast<nsStringArray*>(aClosure);
    if (sa->IndexOf(aFontName) < 0) {
        sa->AppendString(aFontName);
    }
    return PR_TRUE;
}
