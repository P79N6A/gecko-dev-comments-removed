








































#include "gfxContext.h"

#include "gfxOS2Platform.h"
#include "gfxOS2Surface.h"
#include "gfxOS2Fonts.h"

#include "nsIServiceManager.h"
#include "nsIPlatformCharset.h"





gfxOS2Font::gfxOS2Font(const nsAString &aName, const gfxFontStyle *aFontStyle)
    : gfxFont(aName, aFontStyle),
      mFontFace(nsnull), mScaledFont(nsnull),
      mMetrics(nsnull), mAdjustedSize(0)
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






const gfxFont::Metrics& gfxOS2Font::GetMetrics()
{
#ifdef DEBUG_thebes_1
    printf("gfxOS2Font[%#x]::GetMetrics()\n", (unsigned)this);
#endif
    if (!mMetrics) {
        mMetrics = new gfxFont::Metrics;

        mMetrics->emHeight = GetStyle()->size;

        FT_UInt gid; 
        FT_Face face = cairo_ft_scaled_font_lock_face(CairoScaledFont());
        if (!face) {
            
            
            
            return *mMetrics;
        }

        double emUnit = 1.0 * face->units_per_EM;
        double xScale = face->size->metrics.x_ppem / emUnit;
        double yScale = face->size->metrics.y_ppem / emUnit;

        
        gid = FT_Get_Char_Index(face, ' ');
        
        
        
        FT_Load_Glyph(face, gid, FT_LOAD_DEFAULT);
        
        mMetrics->spaceWidth = face->glyph->advance.x >> 6;
        
        mSpaceGlyph = gid;

        
        gid = FT_Get_Char_Index(face, 'x'); 
        if (gid) {
            
            FT_Load_Glyph(face, gid, FT_LOAD_NO_SCALE);
            mMetrics->xHeight = face->glyph->metrics.height * yScale;
            mMetrics->aveCharWidth = face->glyph->metrics.width * xScale;
        } else {
            
            
            mMetrics->xHeight = mMetrics->emHeight * 0.5;
            mMetrics->aveCharWidth = mMetrics->emHeight * 0.5;
        }

        
        if (mAdjustedSize == 0 && GetStyle()->sizeAdjust != 0) {
            gfxFloat aspect = mMetrics->xHeight / GetStyle()->size;
            mAdjustedSize = GetStyle()->GetAdjustedSize(aspect);
            mMetrics->emHeight = mAdjustedSize;
        }

        
        TT_OS2 *os2 = (TT_OS2 *)FT_Get_Sfnt_Table(face, ft_sfnt_os2);
        if (os2 && os2->version != 0xFFFF) { 
            
            mMetrics->aveCharWidth = os2->xAvgCharWidth * xScale;

            mMetrics->superscriptOffset = os2->ySuperscriptYOffset * yScale;
            mMetrics->superscriptOffset = PR_MAX(1, mMetrics->superscriptOffset);
            
            mMetrics->subscriptOffset   = fabs(os2->ySubscriptYOffset * yScale);
            mMetrics->subscriptOffset   = PR_MAX(1, fabs(mMetrics->subscriptOffset));
            mMetrics->strikeoutOffset   = os2->yStrikeoutPosition * yScale;
            mMetrics->strikeoutSize     = PR_MAX(1, os2->yStrikeoutSize * yScale);
        } else {
            
            mMetrics->superscriptOffset = mMetrics->emHeight * 0.5;
            mMetrics->subscriptOffset   = mMetrics->emHeight * 0.2;
            mMetrics->strikeoutOffset   = mMetrics->emHeight * 0.3;
            mMetrics->strikeoutSize     = face->underline_thickness * yScale;
        }
        
        mMetrics->underlineOffset = face->underline_position * yScale;
        mMetrics->underlineSize   = PR_MAX(1, face->underline_thickness * yScale);

        
        mMetrics->emAscent        = face->ascender * yScale;
        mMetrics->emDescent       = -face->descender * yScale;
        mMetrics->maxHeight       = face->height * yScale;
        mMetrics->maxAscent       = face->bbox.yMax * yScale;
        mMetrics->maxDescent      = -face->bbox.yMin * yScale;
        mMetrics->maxAdvance      = face->max_advance_width * xScale;
        
        double lineHeight = mMetrics->maxAscent + mMetrics->maxDescent;
        if (lineHeight > mMetrics->emHeight) {
            mMetrics->internalLeading = lineHeight - mMetrics->emHeight;
        } else {
            mMetrics->internalLeading = 0;
        }
        mMetrics->externalLeading = 0; 

#ifdef DEBUG_thebes_1
        printf("gfxOS2Font[%#x]::GetMetrics():\n"
               "  %s (%s)\n"
               "  emHeight=%f == %f=gfxFont::style.size == %f=adjSz\n"
               "  maxHeight=%f  xHeight=%f\n"
               "  aveCharWidth=%f==xWidth  spaceWidth=%f\n"
               "  supOff=%f SubOff=%f   strOff=%f strSz=%f\n"
               "  undOff=%f undSz=%f    intLead=%f extLead=%f\n"
               "  emAsc=%f emDesc=%f maxH=%f\n"
               "  maxAsc=%f maxDes=%f maxAdv=%f\n",
               (unsigned)this,
               NS_LossyConvertUTF16toASCII(mName).get(),
               os2 && os2->version != 0xFFFF ? "has OS/2 table" : "no OS/2 table!",
               mMetrics->emHeight, GetStyle()->size, mAdjustedSize,
               mMetrics->maxHeight, mMetrics->xHeight,
               mMetrics->aveCharWidth, mMetrics->spaceWidth,
               mMetrics->superscriptOffset, mMetrics->subscriptOffset,
               mMetrics->strikeoutOffset, mMetrics->strikeoutSize,
               mMetrics->underlineOffset, mMetrics->underlineSize,
               mMetrics->internalLeading, mMetrics->externalLeading,
               mMetrics->emAscent, mMetrics->emDescent, mMetrics->maxHeight,
               mMetrics->maxAscent, mMetrics->maxDescent, mMetrics->maxAdvance
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
               (unsigned)this, NS_LossyConvertUTF16toASCII(mName).get(), GetStyle()->size);
#endif
        FcPattern *fcPattern = FcPatternCreate();

        
        
        FcPatternAddString(fcPattern, FC_FAMILY,
                           (FcChar8 *)NS_LossyConvertUTF16toASCII(mName).get());

        
        
        
        
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
               NS_LossyConvertUTF16toASCII(mName).get(),
               GetStyle()->weight, GetStyle()->style, GetStyle()->size,
               (char *)str1, w1, i1, s1,
               (char *)str2, w2, i2, s2);
#endif
        FcPatternDestroy(fcPattern);
        if (mName == NS_LITERAL_STRING("Workplace Sans") && fcW >= FC_WEIGHT_DEMIBOLD) {
            
            
            FcPatternAddBool(fcMatch, FC_EMBOLDEN, FcTrue);
        }
        
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
               (unsigned)this, NS_LossyConvertUTF16toASCII(mName).get(), GetStyle()->size);
#endif

        double size = mAdjustedSize ? mAdjustedSize : GetStyle()->size;
        cairo_matrix_t fontMatrix;
        cairo_matrix_init_scale(&fontMatrix, size, size);
        cairo_font_options_t *fontOptions = cairo_font_options_create();
        mScaledFont = cairo_scaled_font_create(CairoFontFace(), &fontMatrix,
                                               reinterpret_cast<cairo_matrix_t*>(&mCTM),
                                               fontOptions);
        cairo_font_options_destroy(fontOptions);
    }

    NS_ASSERTION(cairo_scaled_font_status(mScaledFont) == CAIRO_STATUS_SUCCESS,
                 "Failed to make scaled font");
    return mScaledFont;
}

nsString gfxOS2Font::GetUniqueName()
{
#ifdef DEBUG_thebes
    printf("gfxOS2Font::GetUniqueName()=%s\n", (char *)mName.get());
#endif
    
    
    
    return mName;
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










static already_AddRefed<gfxOS2Font> GetOrMakeFont(const nsAString& aName,
                                                  const gfxFontStyle *aStyle)
{
    nsRefPtr<gfxFont> font = gfxFontCache::GetCache()->Lookup(aName, aStyle);
    if (!font) {
        font = new gfxOS2Font(aName, aStyle);
        if (!font)
            return nsnull;
        gfxFontCache::GetCache()->AddNew(font);
    }
    gfxFont *f = nsnull;
    font.swap(f);
    return static_cast<gfxOS2Font *>(f);
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

    
    
    int pos = 0;
    if ((pos = mFamilies.Find("WarpSans", PR_FALSE, 0, -1)) > -1) {
        mFamilies.Replace(pos, 8, NS_LITERAL_STRING("Workplace Sans"));
    }

    nsStringArray familyArray;
    ForEachFont(FontCallback, &familyArray);
    FindGenericFontFromStyle(FontCallback, &familyArray);

    
    
    
    
    nsString fontString;
    gfxPlatform::GetPlatform()->GetPrefFonts("x-unicode", fontString, PR_FALSE);
    ForEachFont(fontString, NS_LITERAL_CSTRING("x-unicode"), FontCallback, &familyArray);
    gfxPlatform::GetPlatform()->GetPrefFonts("x-user-def", fontString, PR_FALSE);
    ForEachFont(fontString, NS_LITERAL_CSTRING("x-user-def"), FontCallback, &familyArray);

    
    
    
    if (familyArray.Count() == 0) {
        familyArray.AppendString(NS_LITERAL_STRING("Helv"));
    }

    for (int i = 0; i < familyArray.Count(); i++) {
        nsRefPtr<gfxOS2Font> font = GetOrMakeFont(*familyArray[i], &mStyle);
        if (font) {
            mFonts.AppendElement(font);
        }
    }

#ifdef REALLY_DESPERATE_FONT_MATCHING
    
    nsStringArray fontList;
    nsCAutoString generic;
    if (!gfxPlatform::GetPlatform()->GetFontList(GetStyle()->langGroup, generic, fontList)) {
        
        
        fontList.RemoveString(NS_LITERAL_STRING("MARKSYM"));
        fontList.RemoveString(NS_LITERAL_STRING("MT Extra"));
        fontList.RemoveString(NS_LITERAL_STRING("Math1"));
        fontList.RemoveString(NS_LITERAL_STRING("Math2"));
        fontList.RemoveString(NS_LITERAL_STRING("Math3"));
        fontList.RemoveString(NS_LITERAL_STRING("Math4"));
        fontList.RemoveString(NS_LITERAL_STRING("Math5"));
        fontList.RemoveString(NS_LITERAL_STRING("Math1Mono"));
        fontList.RemoveString(NS_LITERAL_STRING("Math2Mono"));
        fontList.RemoveString(NS_LITERAL_STRING("Math3Mono"));
        fontList.RemoveString(NS_LITERAL_STRING("Math4Mono"));
        fontList.RemoveString(NS_LITERAL_STRING("Math5Mono"));
        
        for (int i = 3; i < fontList.Count(); i++) {
            
            if (familyArray.IndexOf(*fontList[i]) == -1) {
                nsRefPtr<gfxOS2Font> font = GetOrMakeFont(*fontList[i], &mStyle);
                if (font) {
                    mFonts.AppendElement(font);
                }
            }
        }
    }
#endif
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
    gfxTextRun *textRun = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!textRun)
        return nsnull;

    textRun->RecordSurrogates(aString);

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
    NS_ASSERTION(aFlags & TEXT_IS_8BIT, "8bit should have been set");
    gfxTextRun *textRun = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
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
    PRUint32 fontlistLast = FontListLength()-1;
    gfxOS2Font *font0 = GetFontAt(0);
    const PRUint8 *p = aUTF8;
    PRUint32 utf16Offset = 0;
    gfxTextRun::CompressedGlyph g;
    const PRUint32 appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();

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

        if (ch == 0) {
            
            aTextRun->SetMissingGlyph(utf16Offset, 0);
        } else {
            
            
            
            
            for (PRUint32 i = 0; i <= fontlistLast; i++) {
                gfxOS2Font *font = font0;
                FT_Face face = face0;
                if (i > 0) {
                    font = GetFontAt(i);
                    face = cairo_ft_scaled_font_lock_face(font->CairoScaledFont());
#ifdef DEBUG_thebes_2
                    if (i == fontlistLast) {
                        printf("Last font %d (%s) for ch=%#x (pos=%d)",
                               i, NS_LossyConvertUTF16toASCII(font->GetName()).get(), ch, utf16Offset);
                    }
#endif
                }
                
                aTextRun->AddGlyphRun(font, utf16Offset);

                NS_ASSERTION(!IsInvalidChar(ch), "Invalid char detected");
                FT_UInt gid = FT_Get_Char_Index(face, ch); 
                PRInt32 advance = 0;
                if (gid == font->GetSpaceGlyph()) {
                    advance = (int)(font->GetMetrics().spaceWidth * appUnitsPerDevUnit);
                } else if (gid == 0) {
                    advance = -1; 
                } else {
                    FT_Load_Glyph(face, gid, FT_LOAD_DEFAULT); 
                    advance = (face->glyph->advance.x >> 6) * appUnitsPerDevUnit;
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
                    
                    if (i == fontlistLast) {
                        
                        
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
    nsStringArray *sa = static_cast<nsStringArray*>(aClosure);
    if (sa->IndexOf(aFontName) < 0) {
        sa->AppendString(aFontName);
    }
    return PR_TRUE;
}
