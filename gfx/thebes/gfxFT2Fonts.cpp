



































#if defined(MOZ_WIDGET_GTK2)
#include "gfxPlatformGtk.h"
#define gfxToolkitPlatform gfxPlatformGtk
#elif defined(MOZ_WIDGET_QT)
#include <qfontinfo.h>
#include "gfxQtPlatform.h"
#define gfxToolkitPlatform gfxQtPlatform
#elif defined(XP_WIN)
#include "gfxWindowsPlatform.h"
#define gfxToolkitPlatform gfxWindowsPlatform
#elif defined(ANDROID)
#include "gfxAndroidPlatform.h"
#define gfxToolkitPlatform gfxAndroidPlatform
#endif

#include "gfxTypes.h"
#include "gfxFT2Fonts.h"
#include "gfxFT2FontBase.h"
#include "gfxFT2Utils.h"
#include "gfxFT2FontList.h"
#include <locale.h>
#include "gfxHarfBuzzShaper.h"
#include "gfxUnicodeProperties.h"
#include "gfxAtoms.h"
#include "nsTArray.h"
#include "nsUnicodeRange.h"
#include "nsCRT.h"

#include "prlog.h"
#include "prinit.h"

#include "mozilla/Preferences.h"

static PRLogModuleInfo *gFontLog = PR_NewLogModule("ft2fonts");




#define MOZ_FT_ROUND(x) (((x) + 32) & ~63) // 63 = 2^6 - 1
#define MOZ_FT_TRUNC(x) ((x) >> 6)
#define CONVERT_DESIGN_UNITS_TO_PIXELS(v, s) \
        MOZ_FT_TRUNC(MOZ_FT_ROUND(FT_MulFix((v) , (s))))

#ifndef ANDROID 




bool
gfxFT2FontGroup::FontCallback(const nsAString& fontName,
                              const nsACString& genericName,
                              bool aUseFontSet,
                              void *closure)
{
    nsTArray<nsString> *sa = static_cast<nsTArray<nsString>*>(closure);

    if (!fontName.IsEmpty() && !sa->Contains(fontName)) {
        sa->AppendElement(fontName);
#ifdef DEBUG_pavlov
        printf(" - %s\n", NS_ConvertUTF16toUTF8(fontName).get());
#endif
    }

    return true;
}

gfxFT2FontGroup::gfxFT2FontGroup(const nsAString& families,
                                 const gfxFontStyle *aStyle,
                                 gfxUserFontSet *aUserFontSet)
    : gfxFontGroup(families, aStyle, aUserFontSet)
{
#ifdef DEBUG_pavlov
    printf("Looking for %s\n", NS_ConvertUTF16toUTF8(families).get());
#endif
    nsTArray<nsString> familyArray;
    ForEachFont(FontCallback, &familyArray);

    if (familyArray.Length() == 0) {
        nsAutoString prefFamilies;
        gfxToolkitPlatform::GetPlatform()->GetPrefFonts(aStyle->language, prefFamilies, nsnull);
        if (!prefFamilies.IsEmpty()) {
            ForEachFont(prefFamilies, aStyle->language, FontCallback, &familyArray);
        }
    }
    if (familyArray.Length() == 0) {
#if defined(MOZ_WIDGET_QT) 
        printf("failde to find a font. sadface\n");
        
        QFont defaultFont;
        QFontInfo fi (defaultFont);
        familyArray.AppendElement(nsDependentString(static_cast<const PRUnichar *>(fi.family().utf16())));
#elif defined(MOZ_WIDGET_GTK2)
        FcResult result;
        FcChar8 *family = nsnull;
        FcPattern* pat = FcPatternCreate();
        FcPattern *match = FcFontMatch(nsnull, pat, &result);
        if (match)
            FcPatternGetString(match, FC_FAMILY, 0, &family);
        if (family)
            familyArray.AppendElement(NS_ConvertUTF8toUTF16((char*)family));
#elif defined(XP_WIN)
        HGDIOBJ hGDI = ::GetStockObject(SYSTEM_FONT);
        LOGFONTW logFont;
        if (hGDI && ::GetObjectW(hGDI, sizeof(logFont), &logFont))
            familyArray.AppendElement(nsDependentString(logFont.lfFaceName));
#elif defined(ANDROID)
        familyArray.AppendElement(NS_LITERAL_STRING("Droid Sans"));
#else
#error "Platform not supported"
#endif
    }

    for (PRUint32 i = 0; i < familyArray.Length(); i++) {
        nsRefPtr<gfxFT2Font> font = gfxFT2Font::GetOrMakeFont(familyArray[i], &mStyle);
        if (font) {
            mFonts.AppendElement(font);
        }
    }
    NS_ASSERTION(mFonts.Length() > 0, "We need at least one font in a fontgroup");
}

gfxFT2FontGroup::~gfxFT2FontGroup()
{
}

gfxFontGroup *
gfxFT2FontGroup::Copy(const gfxFontStyle *aStyle)
{
    return new gfxFT2FontGroup(mFamilies, aStyle, nsnull);
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


static bool
AddFontNameToArray(const nsAString& aName,
                   const nsACString& aGenericName,
                   bool aUseFontSet,
                   void *aClosure)
{
    if (!aName.IsEmpty()) {
        nsTArray<nsString> *list = static_cast<nsTArray<nsString> *>(aClosure);

        if (list->IndexOf(aName) == list->NoIndex)
            list->AppendElement(aName);
    }

    return true;
}

void
gfxFT2FontGroup::FamilyListToArrayList(const nsString& aFamilies,
                                       nsIAtom *aLangGroup,
                                       nsTArray<nsRefPtr<gfxFontEntry> > *aFontEntryList)
{
    nsAutoTArray<nsString, 15> fonts;
    ForEachFont(aFamilies, aLangGroup, AddFontNameToArray, &fonts);

    PRUint32 len = fonts.Length();
    for (PRUint32 i = 0; i < len; ++i) {
        const nsString& str = fonts[i];
        nsRefPtr<gfxFontEntry> fe = (gfxToolkitPlatform::GetPlatform()->FindFontEntry(str, mStyle));
        aFontEntryList->AppendElement(fe);
    }
}

void gfxFT2FontGroup::GetPrefFonts(nsIAtom *aLangGroup, nsTArray<nsRefPtr<gfxFontEntry> >& aFontEntryList)
{
    NS_ASSERTION(aLangGroup, "aLangGroup is null");
    gfxToolkitPlatform *platform = gfxToolkitPlatform::GetPlatform();
    nsAutoTArray<nsRefPtr<gfxFontEntry>, 5> fonts;
    nsCAutoString key;
    aLangGroup->ToUTF8String(key);
    key.Append("-");
    key.AppendInt(GetStyle()->style);
    key.Append("-");
    key.AppendInt(GetStyle()->weight);
    if (!platform->GetPrefFontEntries(key, &fonts)) {
        nsString fontString;
        platform->GetPrefFonts(aLangGroup, fontString);
        if (fontString.IsEmpty())
            return;

        FamilyListToArrayList(fontString, aLangGroup, &fonts);

        platform->SetPrefFontEntries(key, fonts);
    }
    aFontEntryList.AppendElements(fonts);
}

static PRInt32 GetCJKLangGroupIndex(const char *aLangGroup) {
    PRInt32 i;
    for (i = 0; i < COUNT_OF_CJK_LANG_GROUP; i++) {
        if (!PL_strcasecmp(aLangGroup, sCJKLangGroup[i]))
            return i;
    }
    return -1;
}


void gfxFT2FontGroup::GetCJKPrefFonts(nsTArray<nsRefPtr<gfxFontEntry> >& aFontEntryList) {
    gfxToolkitPlatform *platform = gfxToolkitPlatform::GetPlatform();

    nsCAutoString key("x-internal-cjk-");
    key.AppendInt(mStyle.style);
    key.Append("-");
    key.AppendInt(mStyle.weight);

    if (!platform->GetPrefFontEntries(key, &aFontEntryList)) {
        NS_ENSURE_TRUE(Preferences::GetRootBranch(), );
        
        nsAdoptingCString list = Preferences::GetLocalizedCString("intl.accept_languages");
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
                lang.CompressWhitespace(false, true);
                PRInt32 index = GetCJKLangGroupIndex(lang.get());
                if (index >= 0) {
                    nsCOMPtr<nsIAtom> atom = do_GetAtom(sCJKLangGroup[index]);
                    GetPrefFonts(atom, aFontEntryList);
                }
                p++;
            }
        }

        
#ifdef XP_WIN
        switch (::GetACP()) {
            case 932: GetPrefFonts(gfxAtoms::ja, aFontEntryList); break;
            case 936: GetPrefFonts(gfxAtoms::zh_cn, aFontEntryList); break;
            case 949: GetPrefFonts(gfxAtoms::ko, aFontEntryList); break;
            
            case 950: GetPrefFonts(gfxAtoms::zh_tw, aFontEntryList); break;
        }
#else
        const char *ctype = setlocale(LC_CTYPE, NULL);
        if (ctype) {
            if (!PL_strncasecmp(ctype, "ja", 2)) {
                GetPrefFonts(gfxAtoms::ja, aFontEntryList);
            } else if (!PL_strncasecmp(ctype, "zh_cn", 5)) {
                GetPrefFonts(gfxAtoms::zh_cn, aFontEntryList);
            } else if (!PL_strncasecmp(ctype, "zh_hk", 5)) {
                GetPrefFonts(gfxAtoms::zh_hk, aFontEntryList);
            } else if (!PL_strncasecmp(ctype, "zh_tw", 5)) {
                GetPrefFonts(gfxAtoms::zh_tw, aFontEntryList);
            } else if (!PL_strncasecmp(ctype, "ko", 2)) {
                GetPrefFonts(gfxAtoms::ko, aFontEntryList);
            }
        }
#endif

        
        GetPrefFonts(gfxAtoms::ja, aFontEntryList);
        GetPrefFonts(gfxAtoms::ko, aFontEntryList);
        GetPrefFonts(gfxAtoms::zh_cn, aFontEntryList);
        GetPrefFonts(gfxAtoms::zh_hk, aFontEntryList);
        GetPrefFonts(gfxAtoms::zh_tw, aFontEntryList);

        platform->SetPrefFontEntries(key, aFontEntryList);
    }
}

already_AddRefed<gfxFT2Font>
gfxFT2FontGroup::WhichFontSupportsChar(const nsTArray<nsRefPtr<gfxFontEntry> >& aFontEntryList, PRUint32 aCh)
{
    for (PRUint32 i = 0; i < aFontEntryList.Length(); i++) {
        gfxFontEntry *fe = aFontEntryList[i].get();
        if (fe->HasCharacter(aCh)) {
            nsRefPtr<gfxFT2Font> font =
                gfxFT2Font::GetOrMakeFont(static_cast<FontEntry*>(fe), &mStyle);
            return font.forget();
        }
    }
    return nsnull;
}

already_AddRefed<gfxFont>
gfxFT2FontGroup::WhichPrefFontSupportsChar(PRUint32 aCh)
{
    if (aCh > 0xFFFF)
        return nsnull;

    nsRefPtr<gfxFT2Font> selectedFont;

    
    nsAutoTArray<nsRefPtr<gfxFontEntry>, 5> fonts;
    GetPrefFonts(mStyle.language, fonts);
    selectedFont = WhichFontSupportsChar(fonts, aCh);

    
    if (!selectedFont) {
        PRUint32 unicodeRange = FindCharUnicodeRange(aCh);

        
        if (unicodeRange == kRangeSetCJK) {
            if (PR_LOG_TEST(gFontLog, PR_LOG_DEBUG)) {
                PR_LOG(gFontLog, PR_LOG_DEBUG, (" - Trying to find fonts for: CJK"));
            }

            nsAutoTArray<nsRefPtr<gfxFontEntry>, 15> fonts;
            GetCJKPrefFonts(fonts);
            selectedFont = WhichFontSupportsChar(fonts, aCh);
        } else {
            nsIAtom *langGroup = LangGroupFromUnicodeRange(unicodeRange);
            if (langGroup) {
                PR_LOG(gFontLog, PR_LOG_DEBUG, (" - Trying to find fonts for: %s", nsAtomCString(langGroup).get()));

                nsAutoTArray<nsRefPtr<gfxFontEntry>, 5> fonts;
                GetPrefFonts(langGroup, fonts);
                selectedFont = WhichFontSupportsChar(fonts, aCh);
            }
        }
    }

    if (selectedFont) {
        nsRefPtr<gfxFont> f = static_cast<gfxFont*>(selectedFont.get());
        return f.forget();
    }

    return nsnull;
}

already_AddRefed<gfxFont>
gfxFT2FontGroup::WhichSystemFontSupportsChar(PRUint32 aCh)
{
#if defined(XP_WIN) || defined(ANDROID)
    FontEntry *fe = static_cast<FontEntry*>
        (gfxPlatformFontList::PlatformFontList()->FindFontForChar(aCh, GetFontAt(0)));
    if (fe) {
        nsRefPtr<gfxFT2Font> f = gfxFT2Font::GetOrMakeFont(fe, &mStyle);
        nsRefPtr<gfxFont> font = f.get();
        return font.forget();
    }
#else
    nsRefPtr<gfxFont> selectedFont;
    nsRefPtr<gfxFont> refFont = GetFontAt(0);
    gfxToolkitPlatform *platform = gfxToolkitPlatform::GetPlatform();
    selectedFont = platform->FindFontForChar(aCh, refFont);
    if (selectedFont)
        return selectedFont.forget();
#endif
    return nsnull;
}

#endif 





bool
gfxFT2Font::InitTextRun(gfxContext *aContext,
                        gfxTextRun *aTextRun,
                        const PRUnichar *aString,
                        PRUint32 aRunStart,
                        PRUint32 aRunLength,
                        PRInt32 aRunScript,
                        bool aPreferPlatformShaping)
{
    bool ok = false;

    if (gfxPlatform::GetPlatform()->UseHarfBuzzForScript(aRunScript)) {
        if (!mHarfBuzzShaper) {
            gfxFT2LockedFace face(this);
            mFUnitsConvFactor = face.XScale();

            mHarfBuzzShaper = new gfxHarfBuzzShaper(this);
        }
        ok = mHarfBuzzShaper->InitTextRun(aContext, aTextRun, aString,
                                          aRunStart, aRunLength, aRunScript);
    }

    if (!ok) {
        AddRange(aTextRun, aString, aRunStart, aRunLength);
    }

    aTextRun->AdjustAdvancesForSyntheticBold(aContext, aRunStart, aRunLength);

    return true;
}

void
gfxFT2Font::AddRange(gfxTextRun *aTextRun, const PRUnichar *str, PRUint32 offset, PRUint32 len)
{
    const PRUint32 appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();
    
    gfxFT2LockedFace faceLock(this);
    FT_Face face = faceLock.get();

    gfxTextRun::CompressedGlyph g;

    const gfxFT2Font::CachedGlyphData *cgd = nsnull, *cgdNext = nsnull;

    FT_UInt spaceGlyph = GetSpaceGlyph();

    for (PRUint32 i = 0; i < len; i++) {
        PRUint32 ch = str[offset + i];

        if (ch == 0) {
            
            aTextRun->SetMissingGlyph(offset + i, 0);
            continue;
        }

        NS_ASSERTION(!gfxFontGroup::IsInvalidChar(ch), "Invalid char detected");

        if (cgdNext) {
            cgd = cgdNext;
            cgdNext = nsnull;
        } else {
            cgd = GetGlyphDataForChar(ch);
        }

        FT_UInt gid = cgd->glyphIndex;
        PRInt32 advance = 0;

        if (gid == 0) {
            advance = -1; 
        } else {
            
            
            PRUint32 chNext = 0;
            FT_UInt gidNext = 0;
            FT_Pos lsbDeltaNext = 0;

            if (FT_HAS_KERNING(face) && i + 1 < len) {
                chNext = str[offset + i + 1];
                if (chNext != 0) {
                    cgdNext = GetGlyphDataForChar(chNext);
                    gidNext = cgdNext->glyphIndex;
                    if (gidNext && gidNext != spaceGlyph)
                        lsbDeltaNext = cgdNext->lsbDelta;
                }
            }

            advance = cgd->xAdvance;

            
            if (chNext && gidNext) {
                FT_Vector kerning; kerning.x = 0;
                FT_Get_Kerning(face, gid, gidNext, FT_KERNING_DEFAULT, &kerning);
                advance += kerning.x;
                if (cgd->rsbDelta - lsbDeltaNext >= 32) {
                    advance -= 64;
                } else if (cgd->rsbDelta - lsbDeltaNext < -32) {
                    advance += 64;
                }
            }

            
            
            
            advance = ((advance * appUnitsPerDevUnit + 32) >> 6);
        }
#ifdef DEBUG_thebes_2
        printf(" gid=%d, advance=%d (%s)\n", gid, advance,
               NS_LossyConvertUTF16toASCII(font->GetName()).get());
#endif

        if (advance >= 0 &&
            gfxTextRun::CompressedGlyph::IsSimpleAdvance(advance) &&
            gfxTextRun::CompressedGlyph::IsSimpleGlyphID(gid)) {
            aTextRun->SetSimpleGlyph(offset + i, g.SetSimpleGlyph(advance, gid));
        } else if (gid == 0) {
            
            aTextRun->SetMissingGlyph(offset + i, ch);
        } else {
            gfxTextRun::DetailedGlyph details;
            details.mGlyphID = gid;
            NS_ASSERTION(details.mGlyphID == gid, "Seriously weird glyph ID detected!");
            details.mAdvance = advance;
            details.mXOffset = 0;
            details.mYOffset = 0;
            g.SetComplex(aTextRun->IsClusterStart(offset + i), true, 1);
            aTextRun->SetGlyphs(offset + i, g, &details);
        }
    }
}

gfxFT2Font::gfxFT2Font(cairo_scaled_font_t *aCairoFont,
                       FT2FontEntry *aFontEntry,
                       const gfxFontStyle *aFontStyle,
                       bool aNeedsBold)
    : gfxFT2FontBase(aCairoFont, aFontEntry, aFontStyle)
{
    NS_ASSERTION(mFontEntry, "Unable to find font entry for font.  Something is whack.");
    mApplySyntheticBold = aNeedsBold;
    mCharGlyphCache.Init(64);
}

gfxFT2Font::~gfxFT2Font()
{
}

cairo_font_face_t *
gfxFT2Font::CairoFontFace()
{
    return GetFontEntry()->CairoFontFace();
}






already_AddRefed<gfxFT2Font>
gfxFT2Font::GetOrMakeFont(const nsAString& aName, const gfxFontStyle *aStyle,
                          bool aNeedsBold)
{
#ifdef ANDROID
    FT2FontEntry *fe = static_cast<FT2FontEntry*>
        (gfxPlatformFontList::PlatformFontList()->
            FindFontForFamily(aName, aStyle, aNeedsBold));
#else
    FT2FontEntry *fe = static_cast<FT2FontEntry*>
        (gfxToolkitPlatform::GetPlatform()->FindFontEntry(aName, *aStyle));
#endif
    if (!fe) {
        NS_WARNING("Failed to find font entry for font!");
        return nsnull;
    }

    nsRefPtr<gfxFT2Font> font = GetOrMakeFont(fe, aStyle, aNeedsBold);
    return font.forget();
}

already_AddRefed<gfxFT2Font>
gfxFT2Font::GetOrMakeFont(FT2FontEntry *aFontEntry, const gfxFontStyle *aStyle,
                          bool aNeedsBold)
{
    nsRefPtr<gfxFont> font = gfxFontCache::GetCache()->Lookup(aFontEntry, aStyle);
    if (!font) {
        cairo_scaled_font_t *scaledFont = aFontEntry->CreateScaledFont(aStyle);
        font = new gfxFT2Font(scaledFont, aFontEntry, aStyle, aNeedsBold);
        cairo_scaled_font_destroy(scaledFont);
        if (!font)
            return nsnull;
        gfxFontCache::GetCache()->AddNew(font);
    }
    gfxFont *f = nsnull;
    font.swap(f);
    return static_cast<gfxFT2Font *>(f);
}

void
gfxFT2Font::FillGlyphDataForChar(PRUint32 ch, CachedGlyphData *gd)
{
    gfxFT2LockedFace faceLock(this);
    FT_Face face = faceLock.get();

    FT_UInt gid = FT_Get_Char_Index(face, ch);

    if (gid == 0) {
        
        NS_ASSERTION(gid != 0, "We don't have a glyph, but font indicated that it supported this char in tables?");
        gd->glyphIndex = 0;
        return;
    }

#ifdef MOZ_GFX_OPTIMIZE_MOBILE
    FT_Error err = FT_Load_Glyph(face, gid, FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING);
#else
    FT_Error err = FT_Load_Glyph(face, gid, FT_LOAD_DEFAULT);
#endif

    if (err) {
        
        NS_WARNING("Failed to load glyph that we got from Get_Char_index");

        gd->glyphIndex = 0;
        return;
    }

    gd->glyphIndex = gid;
    gd->lsbDelta = face->glyph->lsb_delta;
    gd->rsbDelta = face->glyph->rsb_delta;
    gd->xAdvance = face->glyph->advance.x;
}
