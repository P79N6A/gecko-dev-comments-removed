



































#include "gfxQtPlatform.h"
#include "gfxTypes.h"
#include "gfxQtFonts.h"
#include "qrect.h"
#include <locale.h>
#include <qfontinfo.h>
#include "cairo-ft.h"
#include <freetype/tttables.h>
#include "gfxFontUtils.h"





FontEntry::FontEntry(const FontEntry& aFontEntry) :
    mFaceName(aFontEntry.mFaceName),
    mUnicodeFont(aFontEntry.mUnicodeFont),
    mSymbolFont(aFontEntry.mSymbolFont),
    mItalic(aFontEntry.mItalic),
    mWeight(aFontEntry.mWeight),
    mCharacterMap(aFontEntry.mCharacterMap)
{
    if (aFontEntry.mFontFace)
        mFontFace = cairo_font_face_reference(aFontEntry.mFontFace);
    else
        mFontFace = nsnull;
}

FontEntry::~FontEntry()
{
    if (mFontFace) {
        cairo_font_face_destroy(mFontFace);
        mFontFace = nsnull;
    }
}

static void
FTFontDestroyFunc(void *data)
{
    FT_Face face = (FT_Face)data;
    FT_Done_Face(face);
    printf("deleting face\n");
}

cairo_font_face_t *
FontEntry::CairoFontFace()
{
    static cairo_user_data_key_t key;
    if (!mFontFace) {
        FT_Face face;
        FT_New_Face(gfxQtPlatform::GetPlatform()->GetFTLibrary(), mFilename.get(), mFTFontIndex, &face);
        mFontFace = cairo_ft_font_face_create_for_ft_face(face, 0);
        cairo_font_face_set_user_data(mFontFace, &key, face, FTFontDestroyFunc);
    }
    return mFontFace;
}

FontEntry *
FontFamily::FindFontEntry(const gfxFontStyle& aFontStyle)
{
    PRBool italic = (aFontStyle.style & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE)) != 0;

    FontEntry *weightList[10] = { 0 };
    for (PRUint32 j = 0; j < 2; j++) {
        PRBool matchesSomething = PR_FALSE;
        
        for (PRUint32 i = 0; i < mFaces.Length(); i++) {
            FontEntry *fe = mFaces[i];
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







PRBool
gfxQtFontGroup::FontCallback(const nsAString& fontName,
                             const nsACString& genericName,
                             void *closure)
{
    nsStringArray *sa = static_cast<nsStringArray*>(closure);

    if (sa->IndexOf(fontName) < 0) {
        sa->AppendString(fontName);
    }

    return PR_TRUE;
}






static already_AddRefed<gfxQtFont>
GetOrMakeFont(const nsAString& aName, const gfxFontStyle *aStyle)
{
    nsRefPtr<gfxFont> font = gfxFontCache::GetCache()->Lookup(aName, aStyle);
    if (!font) {
        FontEntry *fe = gfxQtPlatform::GetPlatform()->FindFontEntry(aName, *aStyle);
        if (!fe) {
            printf("Failed to find font entry for %s\n", NS_ConvertUTF16toUTF8(aName).get());
            return nsnull;
        }

        font = new gfxQtFont(fe, aStyle);
        if (!font)
            return nsnull;
        gfxFontCache::GetCache()->AddNew(font);
    }
    gfxFont *f = nsnull;
    font.swap(f);
    return static_cast<gfxQtFont *>(f);
}


gfxQtFontGroup::gfxQtFontGroup(const nsAString& families,
                               const gfxFontStyle *aStyle)
    : gfxFontGroup(families, aStyle)
{
    nsStringArray familyArray;
    ForEachFont(FontCallback, &familyArray);

    if (familyArray.Count() == 0) {
        QFont defaultFont;
        QFontInfo fi (defaultFont);
        familyArray.AppendString(nsDependentString(static_cast<const PRUnichar *>(fi.family().utf16())));
    }

    for (int i = 0; i < familyArray.Count(); i++) {
        nsRefPtr<gfxQtFont> font = GetOrMakeFont(*familyArray[i], &mStyle);
        if (font) {
            mFonts.AppendElement(font);
        }
    }
}

gfxQtFontGroup::~gfxQtFontGroup()
{
}

gfxFontGroup *
gfxQtFontGroup::Copy(const gfxFontStyle *aStyle)
{
     return new gfxQtFontGroup(mFamilies, aStyle);
}






static PRInt32 AppendDirectionalIndicatorUTF8(PRBool aIsRTL, nsACString& aString)
{
    static const PRUnichar overrides[2][2] = { { 0x202d, 0 }, { 0x202e, 0 }}; 
    AppendUTF16toUTF8(overrides[aIsRTL], aString);
    return 3; 
}

gfxTextRun *gfxQtFontGroup::MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                                        const Parameters* aParams, PRUint32 aFlags)
{
    gfxTextRun *textRun = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!textRun)
        return nsnull;

    textRun->RecordSurrogates(aString);

    mString.Assign(nsDependentSubstring(aString, aString + aLength));

    InitTextRun(textRun);

    textRun->FetchGlyphExtents(aParams->mContext);

    return textRun;
}

gfxTextRun *gfxQtFontGroup::MakeTextRun(const PRUint8 *aString, PRUint32 aLength,
                                        const Parameters *aParams, PRUint32 aFlags)
{
    NS_ASSERTION(aFlags & TEXT_IS_8BIT, "8bit should have been set");
    gfxTextRun *textRun = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!textRun)
        return nsnull;

    const char *chars = reinterpret_cast<const char *>(aString);

    mString.Assign(NS_ConvertASCIItoUTF16(nsDependentCSubstring(chars, chars + aLength)));

    InitTextRun(textRun);

    textRun->FetchGlyphExtents(aParams->mContext);

    return textRun;
}

void gfxQtFontGroup::InitTextRun(gfxTextRun *aTextRun)
{
    CreateGlyphRunsFT(aTextRun);
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








PRBool
HasCharacter(gfxQtFont *aFont, PRUint32 ch)
{
    if (aFont->GetFontEntry()->mCharacterMap.test(ch))
        return PR_TRUE;

    
    FT_Face face = cairo_ft_scaled_font_lock_face(aFont->CairoScaledFont());
    FT_UInt gid = FT_Get_Char_Index(face, ch);
    cairo_ft_scaled_font_unlock_face(aFont->CairoScaledFont());

    if (gid != 0) {
        aFont->GetFontEntry()->mCharacterMap.set(ch);
        return PR_TRUE;
    }
    return PR_FALSE;
}

#if 0
inline FontEntry *
gfxQtFontGroup::WhichFontSupportsChar(const nsTArray<>& foo, PRUint32 ch)
{
    for (int i = 0; i < aGroup->FontListLength(); i++) {
        nsRefPtr<gfxQtFont> font = aGroup->GetFontAt(i);
        if (HasCharacter(font, ch))
            return font;
    }
    return nsnull;
}
#endif

inline gfxQtFont *
gfxQtFontGroup::FindFontForChar(PRUint32 ch, PRUint32 prevCh, PRUint32 nextCh, gfxQtFont *aFont)
{
    gfxQtFont *selectedFont;

    
    
    if (gfxFontUtils::IsJoiner(ch) || gfxFontUtils::IsJoiner(prevCh) || gfxFontUtils::IsJoiner(nextCh)) {
        if (aFont && HasCharacter(aFont, ch))
            return aFont;
    }

    for (PRUint32 i = 0; i < FontListLength(); i++) {
        nsRefPtr<gfxQtFont> font = GetFontAt(i);
        if (HasCharacter(font, ch))
            return font;
    }
    return nsnull;

#if 0
    
    selectedFont = WhichFontSupportsChar(mGroup->GetFontList(), ch);


    
    if ((ch >= 0xE000  && ch <= 0xF8FF) || 
        (ch >= 0xF0000 && ch <= 0x10FFFD))
        return selectedFont;

    
    if (!selectedFont) {
        nsAutoTArray<nsRefPtr<FontEntry>, 5> fonts;
        this->GetPrefFonts(mGroup->GetStyle()->langGroup.get(), fonts);
        selectedFont = WhichFontSupportsChar(fonts, ch);
    }

    
    if (!selectedFont) {
        
        if (ch <= 0xFFFF) {
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

    
    if (!selectedFont && aFont && HasCharacter(aFont, ch))
        selectedFont = aFont;

    
    if (!selectedFont) {
        PR_LOG(gFontLog, PR_LOG_DEBUG, (" - Looking for best match"));
        
        nsRefPtr<gfxWindowsFont> refFont = mGroup->GetFontAt(0);
        gfxWindowsPlatform *platform = gfxWindowsPlatform::GetPlatform();
        selectedFont = platform->FindFontForChar(ch, refFont);
    }

    return selectedFont;
#endif
}

PRUint32
gfxQtFontGroup::ComputeRanges()
{
    const PRUnichar *str = mString.get();
    PRUint32 len = mString.Length();

    mRanges.Clear();

    PRUint32 prevCh = 0;
    for (PRUint32 i = 0; i < len; i++) {
        const PRUint32 origI = i; 
        PRUint32 ch = str[i];
        if ((i+1 < len) && NS_IS_HIGH_SURROGATE(ch) && NS_IS_LOW_SURROGATE(str[i+1])) {
            i++;
            ch = SURROGATE_TO_UCS4(ch, str[i]);
        }

        PRUint32 nextCh = 0;
        if (i+1 < len) {
            nextCh = str[i+1];
            if ((i+2 < len) && NS_IS_HIGH_SURROGATE(ch) && NS_IS_LOW_SURROGATE(str[i+2]))
                nextCh = SURROGATE_TO_UCS4(nextCh, str[i+2]);
        }
        gfxQtFont *fe = FindFontForChar(ch,
                                        prevCh,
                                        nextCh,
                                        (mRanges.Length() == 0) ? nsnull : mRanges[mRanges.Length() - 1].font);

        prevCh = ch;

        if (mRanges.Length() == 0) {
            TextRange r(0,1);
            r.font = fe;
            mRanges.AppendElement(r);
        } else {
            TextRange& prevRange = mRanges[mRanges.Length() - 1];
            if (prevRange.font != fe) {
                
                prevRange.end = origI;

                TextRange r(origI, i+1);
                r.font = fe;
                mRanges.AppendElement(r);
            }
        }
    }
    mRanges[mRanges.Length()-1].end = len;

    PRUint32 nranges = mRanges.Length();
    return nranges;
}

void gfxQtFontGroup::CreateGlyphRunsFT(gfxTextRun *aTextRun)
{
#if 0
    QString str(aUTF8, aUTF8Length);
    QStackTextEngine engine(str, mQFont);
    const Qt::LayoutDirection dir = aTextRun->IsRightToLeft() ? Qt::RightToLeft : Qt::LeftTRight;
    engine.option.setTextDirection(dir);
    engine.ignoreBidi = true;

    
    engine.itemize();


    
    
    
    QScriptLine line;
    line.length = str.length();
    engine.shapeLine(line);

    int nItems = engine.layoutData->items.size();
    QVarLengthArray<int> visualOrder(nItems);
    QVarLengthArray<uchar> levels(nItems);
    for (int i = 0; i < nItems; ++i)
        levels[i] = engine.layoutData->items[i].analysis.bidiLevel;
    QTextEngine::bidiReorder(nItems, levels.data(), visualOrder.data());

    QFixed x = QFixed::fromReal(p.x());
    QFixed ox = x;

    for (int i = 0; i < nItems; ++i) {
        int item = visualOrder[i];
        const QScriptItem &si = engine.layoutData->items.at(item);
        if (si.analysis.flags >= QScriptAnalysis::TabOrObject) {
            x += si.width;
            continue;
        }
        QFont f = engine.font(si);
        











        const PRUint8 *p = aUTF8;
        PRUint32 utf16Offset = 0;
        gfxTextRun::CompressedGlyph g;

        aTextRun->AddGlyphRun(font, 0);
        
        
        FT_Face face =  font->GetQFont().freetypeFace();
        while (p < aUTF8 + aUTF8Length) {
            
            PRUint8 chLen;
        }
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

        x += si.width;
    }
#endif

    ComputeRanges();

    const PRUnichar *strStart = mString.get();
    for (PRUint32 i = 0; i < mRanges.Length(); ++i) {
        const TextRange& range = mRanges[i];
        const PRUnichar *rangeString = strStart + range.start;
        PRUint32 rangeLength = range.Length();

        gfxQtFont *font = range.font ? range.font.get() : GetFontAt(0);
        AddRange(aTextRun, font, rangeString, rangeLength);
    }
    
}

void
gfxQtFontGroup::AddRange(gfxTextRun *aTextRun, gfxQtFont *font, const PRUnichar *str, PRUint32 len)
{
    const PRUint32 appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();
    
    FT_Face face = cairo_ft_scaled_font_lock_face(font->CairoScaledFont());

    gfxTextRun::CompressedGlyph g;

    aTextRun->AddGlyphRun(font, 0);
    for (PRUint32 i = 0; i < len; i++) {
        PRUint32 ch = str[i];

        if (ch == 0) {
            
            aTextRun->SetMissingGlyph(i, 0);
            continue;
        }

        NS_ASSERTION(!IsInvalidChar(ch), "Invalid char detected");
        FT_UInt gid = FT_Get_Char_Index(face, ch); 
        PRInt32 advance = 0;
#if 0
        if (gid == font->GetSpaceGlyph()) {
            advance = (int)(font->GetMetrics().spaceWidth * appUnitsPerDevUnit);
        } else 
#endif
        if (gid == 0) {
            advance = -1; 
        } else {
            
            
            PRUint32 chNext = 0;
            FT_UInt gidNext = 0;
            FT_Pos lsbDeltaNext = 0;

            if (FT_HAS_KERNING(face) && i + 1 < len) {
                chNext = str[i+1];
                if (chNext != 0) {
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
                FT_Vector kerning; kerning.x = 0;
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
            
            aTextRun->SetSimpleGlyph(i, g.SetSimpleGlyph(advance, gid));
        } else if (gid == 0) {
            
            
            aTextRun->SetMissingGlyph(i, ch);
        } else {
            
            gfxTextRun::DetailedGlyph details;
            details.mGlyphID = gid;
            NS_ASSERTION(details.mGlyphID == gid, "Seriously weird glyph ID detected!");
            details.mAdvance = advance;
            details.mXOffset = 0;
            details.mYOffset = 0;
            g.SetComplex(aTextRun->IsClusterStart(i), PR_TRUE, 1);
            aTextRun->SetGlyphs(i, g, &details);
        }


    }

    cairo_ft_scaled_font_unlock_face(font->CairoScaledFont());
}




gfxQtFont::gfxQtFont(FontEntry *aFontEntry,
                     const gfxFontStyle *aFontStyle)
    : gfxFont(aFontEntry->GetName(), aFontStyle),
    mScaledFont(nsnull),
    mHasSpaceGlyph(PR_FALSE),
    mSpaceGlyph(0),
    mHasMetrics(PR_FALSE),
    mAdjustedSize(0),
    mFontEntry(aFontEntry)
{
}

gfxQtFont::~gfxQtFont()
{
    if (mScaledFont) {
        cairo_scaled_font_destroy(mScaledFont);
        mScaledFont = nsnull;
    }

    printf("deleting gfxQtFont\n");
}




#define MOZ_FT_ROUND(x) (((x) + 32) & ~63) // 63 = 2^6 - 1
#define MOZ_FT_TRUNC(x) ((x) >> 6)
#define CONVERT_DESIGN_UNITS_TO_PIXELS(v, s) \
        MOZ_FT_TRUNC(MOZ_FT_ROUND(FT_MulFix((v) , (s))))

const gfxFont::Metrics&
gfxQtFont::GetMetrics()
{
    if (mHasMetrics)
        return mMetrics;

    mMetrics.emHeight = GetStyle()->size;

    FT_Face face = cairo_ft_scaled_font_lock_face(CairoScaledFont());

    if (!face) {
        
        
        
        return mMetrics;
    }

    mMetrics.emHeight = GetStyle()->size;

    FT_UInt gid; 

    const double emUnit = 1.0 * face->units_per_EM;
    const double xScale = face->size->metrics.x_ppem / emUnit;
    const double yScale = face->size->metrics.y_ppem / emUnit;

    
    gid = FT_Get_Char_Index(face, ' ');
    if (gid) {
        FT_Load_Glyph(face, gid, FT_LOAD_DEFAULT);
        
        mMetrics.spaceWidth = face->glyph->advance.x >> 6;
        
        mSpaceGlyph = gid;
    } else {
        NS_ASSERTION(0, "blah");
    }
            
    
    gid = FT_Get_Char_Index(face, 'x'); 
    if (gid) {
        
        FT_Load_Glyph(face, gid, FT_LOAD_NO_SCALE);
        mMetrics.xHeight = face->glyph->metrics.height * yScale;
        mMetrics.aveCharWidth = face->glyph->metrics.width * xScale;
    } else {
        
        
        mMetrics.xHeight = mMetrics.emHeight * 0.5;
        mMetrics.aveCharWidth = mMetrics.emHeight * 0.5;
    }

    
    if (mAdjustedSize == 0 && GetStyle()->sizeAdjust != 0) {
        gfxFloat aspect = mMetrics.xHeight / GetStyle()->size;
        mAdjustedSize = GetStyle()->GetAdjustedSize(aspect);
        mMetrics.emHeight = mAdjustedSize;
    }

    
    TT_OS2 *os2 = (TT_OS2 *)FT_Get_Sfnt_Table(face, ft_sfnt_os2);
    if (os2 && os2->version != 0xFFFF) { 
        
        mMetrics.aveCharWidth = os2->xAvgCharWidth * xScale;

        mMetrics.superscriptOffset = os2->ySuperscriptYOffset * yScale;
        mMetrics.superscriptOffset = PR_MAX(1, mMetrics.superscriptOffset);
        
        mMetrics.subscriptOffset   = fabs(os2->ySubscriptYOffset * yScale);
        mMetrics.subscriptOffset   = PR_MAX(1, fabs(mMetrics.subscriptOffset));
        mMetrics.strikeoutOffset   = os2->yStrikeoutPosition * yScale;
        mMetrics.strikeoutSize     = os2->yStrikeoutSize * yScale;
    } else {
        
        mMetrics.superscriptOffset = mMetrics.emHeight * 0.5;
        mMetrics.subscriptOffset   = mMetrics.emHeight * 0.2;
        mMetrics.strikeoutOffset   = mMetrics.emHeight * 0.3;
        mMetrics.strikeoutSize     = face->underline_thickness * yScale;
    }
    
    mMetrics.underlineOffset = face->underline_position * yScale;
    mMetrics.underlineSize   = face->underline_thickness * yScale;

    
    mMetrics.emAscent        = face->ascender * yScale;
    mMetrics.emDescent       = -face->descender * yScale;
    mMetrics.maxHeight       = face->height * yScale;
    mMetrics.maxAscent       = face->bbox.yMax * yScale;
    mMetrics.maxDescent      = -face->bbox.yMin * yScale;
    mMetrics.maxAdvance      = face->max_advance_width * xScale;
    
    double lineHeight = mMetrics.maxAscent + mMetrics.maxDescent;
    if (lineHeight > mMetrics.emHeight) {
        mMetrics.internalLeading = lineHeight - mMetrics.emHeight;
    } else {
        mMetrics.internalLeading = 0;
    }
    mMetrics.externalLeading = 0; 

    SanitizeMetrics(&mMetrics, PR_FALSE);

    





















    
    cairo_ft_scaled_font_unlock_face(CairoScaledFont());

    mHasMetrics = PR_TRUE;
    return mMetrics;
}


nsString
gfxQtFont::GetUniqueName()
{
    return mName;
}

PRUint32
gfxQtFont::GetSpaceGlyph()
{
    NS_ASSERTION (GetStyle ()->size != 0,
    "forgot to short-circuit a text run with zero-sized font?");

    if(!mHasSpaceGlyph)
    {
        FT_UInt gid = 0; 
        FT_Face face = cairo_ft_scaled_font_lock_face(CairoScaledFont());
        gid = FT_Get_Char_Index(face, ' ');
        FT_Load_Glyph(face, gid, FT_LOAD_DEFAULT);
        mSpaceGlyph = gid;
        mHasSpaceGlyph = PR_TRUE;
        cairo_ft_scaled_font_unlock_face(CairoScaledFont());
    }
    return mSpaceGlyph;
}

cairo_font_face_t *
gfxQtFont::CairoFontFace()
{
    return mFontEntry->CairoFontFace();
}

cairo_scaled_font_t *
gfxQtFont::CairoScaledFont()
{
    if (!mScaledFont) {
        cairo_matrix_t sizeMatrix;
        cairo_matrix_t identityMatrix;

        
        cairo_matrix_init_scale(&sizeMatrix, mStyle.size, mStyle.size);
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

PRBool
gfxQtFont::SetupCairoFont(gfxContext *aContext)
{
    cairo_scaled_font_t *scaledFont = CairoScaledFont();

    if (cairo_scaled_font_status(scaledFont) != CAIRO_STATUS_SUCCESS) {
        
        
        return PR_FALSE;
    }
    
    
    cairo_set_scaled_font(aContext->GetCairo(), scaledFont);
    return PR_TRUE;
}
