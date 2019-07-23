



































#if defined(MOZ_WIDGET_GTK2)
#include "gfxPlatformGtk.h"
#define gfxToolkitPlatform gfxPlatformGtk
#elif defined(MOZ_WIDGET_QT)
#include "gfxQtPlatform.h"
#include <qfontinfo.h>
#define gfxToolkitPlatform gfxQtPlatform
#elif defined(XP_WIN)
#ifdef WINCE
#define SHGetSpecialFolderPathW SHGetSpecialFolderPath
#endif
#include "gfxWindowsPlatform.h"
#define gfxToolkitPlatform gfxWindowsPlatform
#endif
#include "gfxTypes.h"
#include "gfxFT2Fonts.h"
#include "gfxFT2FontBase.h"
#include "gfxFT2Utils.h"
#include <locale.h>
#include "cairo-ft.h"
#include FT_TRUETYPE_TAGS_H
#include FT_TRUETYPE_TABLES_H
#include "gfxFontUtils.h"
#include "nsTArray.h"
#include "nsUnicodeRange.h"
#include "nsIPrefService.h"
#include "nsIPrefLocalizedString.h"
#include "nsServiceManagerUtils.h"
#include "nsCRT.h"

#include "prlog.h"
#include "prinit.h"
static PRLogModuleInfo *gFontLog = PR_NewLogModule("ft2fonts");

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




#define MOZ_FT_ROUND(x) (((x) + 32) & ~63) // 63 = 2^6 - 1
#define MOZ_FT_TRUNC(x) ((x) >> 6)
#define CONVERT_DESIGN_UNITS_TO_PIXELS(v, s) \
        MOZ_FT_TRUNC(MOZ_FT_ROUND(FT_MulFix((v) , (s))))





FontEntry::FontEntry(const FontEntry& aFontEntry) :
    gfxFontEntry(aFontEntry)
{
    mFTFace = aFontEntry.mFTFace;
    if (aFontEntry.mFontFace)
        mFontFace = cairo_font_face_reference(aFontEntry.mFontFace);
    else
        mFontFace = nsnull;
}

FontEntry::~FontEntry()
{
    
    mFTFace = nsnull;

    if (mFontFace) {
        cairo_font_face_destroy(mFontFace);
        mFontFace = nsnull;
    }
}


FontEntry*
FontEntry::CreateFontEntry(const gfxProxyFontEntry &aProxyEntry,
                           const PRUint8 *aFontData,
                           PRUint32 aLength)
{
    
    
    
    FT_Face face;
    FT_Error error =
        FT_New_Memory_Face(gfxToolkitPlatform::GetPlatform()->GetFTLibrary(),
                           aFontData, aLength, 0, &face);
    if (error != FT_Err_Ok) {
        NS_Free((void*)aFontData);
        return nsnull;
    }
    FontEntry* fe = FontEntry::CreateFontEntryFromFace(face, aFontData);
    fe->mItalic = aProxyEntry.mItalic;
    fe->mWeight = aProxyEntry.mWeight;
    fe->mStretch = aProxyEntry.mStretch;
    return fe;
}

class FTUserFontData {
public:
    FTUserFontData(FT_Face aFace, const PRUint8* aData)
        : mFace(aFace), mFontData(aData)
    {
    }

    ~FTUserFontData()
    {
        FT_Done_Face(mFace);
        if (mFontData) {
            NS_Free((void*)mFontData);
        }
    }

private:
    FT_Face        mFace;
    const PRUint8 *mFontData;
};

static void
FTFontDestroyFunc(void *data)
{
    FTUserFontData *userFontData = static_cast<FTUserFontData*>(data);
    delete userFontData;
}

 FontEntry*
FontEntry::CreateFontEntryFromFace(FT_Face aFace, const PRUint8 *aFontData) {
    static cairo_user_data_key_t key;

    if (!aFace->family_name) {
        FT_Done_Face(aFace);
        return nsnull;
    }
    
    
    NS_ConvertUTF8toUTF16 fontName(aFace->family_name);
    if (aFace->style_name && strcmp("Regular", aFace->style_name)) {
        fontName.AppendLiteral(" ");
        AppendUTF8toUTF16(aFace->style_name, fontName);
    }
    FontEntry *fe = new FontEntry(fontName);
    fe->mItalic = aFace->style_flags & FT_STYLE_FLAG_ITALIC;
    fe->mFTFace = aFace;
    fe->mFontFace = cairo_ft_font_face_create_for_ft_face(aFace, 0);

    FTUserFontData *userFontData = new FTUserFontData(aFace, aFontData);
    cairo_font_face_set_user_data(fe->mFontFace, &key,
                                  userFontData, FTFontDestroyFunc);

    TT_OS2 *os2 = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(aFace, ft_sfnt_os2));
    PRUint16 os2weight = 0;
    if (os2 && os2->version != 0xffff) {
        
        
        
        
        if (os2->usWeightClass >= 100 && os2->usWeightClass <= 900)
            os2weight = os2->usWeightClass;
        else if (os2->usWeightClass >= 1 && os2->usWeightClass <= 9)
            os2weight = os2->usWeightClass * 100;
    }

    if (os2weight != 0)
        fe->mWeight = os2weight;
    else if (aFace->style_flags & FT_STYLE_FLAG_BOLD)
        fe->mWeight = 700;
    else
        fe->mWeight = 400;

    NS_ASSERTION(fe->mWeight >= 100 && fe->mWeight <= 900, "Invalid final weight in font!");

    return fe;
}

FontEntry*
gfxFT2Font::GetFontEntry()
{
    return static_cast<FontEntry*> (mFontEntry.get());
}

cairo_font_face_t *
FontEntry::CairoFontFace()
{
    static cairo_user_data_key_t key;

    if (!mFontFace) {
        FT_Face face;
        FT_New_Face(gfxToolkitPlatform::GetPlatform()->GetFTLibrary(), mFilename.get(), mFTFontIndex, &face);
        mFTFace = face;
        mFontFace = cairo_ft_font_face_create_for_ft_face(face, 0);
        FTUserFontData *userFontData = new FTUserFontData(face, nsnull);
        cairo_font_face_set_user_data(mFontFace, &key,
                                      userFontData, FTFontDestroyFunc);
    }
    return mFontFace;
}

nsresult
FontEntry::ReadCMAP()
{
    if (mCmapInitialized) return NS_OK;

    
    mCmapInitialized = PR_TRUE;

    
    CairoFontFace();
    NS_ENSURE_TRUE(mFTFace, NS_ERROR_FAILURE);

    FT_Error status;
    FT_ULong len = 0;
    status = FT_Load_Sfnt_Table(mFTFace, TTAG_cmap, 0, nsnull, &len);
    NS_ENSURE_TRUE(status == 0, NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(len != 0, NS_ERROR_FAILURE);

    nsAutoTArray<PRUint8,16384> buffer;
    if (!buffer.AppendElements(len))
        return NS_ERROR_FAILURE;
    PRUint8 *buf = buffer.Elements();

    status = FT_Load_Sfnt_Table(mFTFace, TTAG_cmap, 0, buf, &len);
    NS_ENSURE_TRUE(status == 0, NS_ERROR_FAILURE);

    PRPackedBool unicodeFont;
    PRPackedBool symbolFont;
    return gfxFontUtils::ReadCMAP(buf, len, mCharacterMap,
                                  unicodeFont, symbolFont);
}

FontEntry *
FontFamily::FindFontEntry(const gfxFontStyle& aFontStyle)
{
    PRBool needsBold = PR_FALSE;
    return static_cast<FontEntry*>(FindFontForStyle(aFontStyle, needsBold));
}

PRBool
FontFamily::FindWeightsForStyle(gfxFontEntry* aFontsForWeights[],
                                PRBool anItalic, PRInt16 aStretch)
{
    PRBool matchesSomething = PR_FALSE;

    for (PRUint32 j = 0; j < 2; j++) {
        
        for (PRUint32 i = 0; i < mAvailableFonts.Length(); i++) {
            gfxFontEntry *fe = mAvailableFonts[i];
            const PRUint8 weight = (fe->mWeight / 100);
            if (fe->mItalic == anItalic) {
                aFontsForWeights[weight] = fe;
                matchesSomething = PR_TRUE;
            }
        }
        if (matchesSomething)
            break;
        anItalic = !anItalic;
    }

    return matchesSomething;
}





PRBool
gfxFT2FontGroup::FontCallback(const nsAString& fontName,
                             const nsACString& genericName,
                             void *closure)
{
    nsTArray<nsString> *sa = static_cast<nsTArray<nsString>*>(closure);

    if (!fontName.IsEmpty() && !sa->Contains(fontName)) {
        sa->AppendElement(fontName);
#ifdef DEBUG_pavlov
        printf(" - %s\n", NS_ConvertUTF16toUTF8(fontName).get());
#endif
    }

    return PR_TRUE;
}

gfxFT2FontGroup::gfxFT2FontGroup(const nsAString& families,
                               const gfxFontStyle *aStyle)
    : gfxFontGroup(families, aStyle)
{
#ifdef DEBUG_pavlov
    printf("Looking for %s\n", NS_ConvertUTF16toUTF8(families).get());
#endif
    nsTArray<nsString> familyArray;
    ForEachFont(FontCallback, &familyArray);

    if (familyArray.Length() == 0) {
        nsAutoString prefFamilies;
        gfxToolkitPlatform::GetPlatform()->GetPrefFonts(aStyle->langGroup.get(), prefFamilies, nsnull);
        if (!prefFamilies.IsEmpty()) {
            ForEachFont(prefFamilies, aStyle->langGroup, FontCallback, &familyArray);
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
     return new gfxFT2FontGroup(mFamilies, aStyle);
}






static PRInt32 AppendDirectionalIndicatorUTF8(PRBool aIsRTL, nsACString& aString)
{
    static const PRUnichar overrides[2][2] = { { 0x202d, 0 }, { 0x202e, 0 }}; 
    AppendUTF16toUTF8(overrides[aIsRTL], aString);
    return 3; 
}

gfxTextRun *gfxFT2FontGroup::MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                                        const Parameters* aParams, PRUint32 aFlags)
{
    NS_ASSERTION(aLength > 0, "should use MakeEmptyTextRun for zero-length text");
    gfxTextRun *textRun = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!textRun)
        return nsnull;

    mString.Assign(nsDependentSubstring(aString, aString + aLength));

    InitTextRun(textRun);

    textRun->FetchGlyphExtents(aParams->mContext);

    return textRun;
}

gfxTextRun *gfxFT2FontGroup::MakeTextRun(const PRUint8 *aString, PRUint32 aLength,
                                        const Parameters *aParams, PRUint32 aFlags)
{
    NS_ASSERTION(aLength > 0, "should use MakeEmptyTextRun for zero-length text");
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

void gfxFT2FontGroup::InitTextRun(gfxTextRun *aTextRun)
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


static PRBool
AddFontNameToArray(const nsAString& aName,
                   const nsACString& aGenericName,
                   void *aClosure)
{
    if (!aName.IsEmpty()) {
        nsTArray<nsString> *list = static_cast<nsTArray<nsString> *>(aClosure);

        if (list->IndexOf(aName) == list->NoIndex)
            list->AppendElement(aName);
    }

    return PR_TRUE;
}

void
gfxFT2FontGroup::FamilyListToArrayList(const nsString& aFamilies,
                                       const nsCString& aLangGroup,
                                       nsTArray<nsRefPtr<FontEntry> > *aFontEntryList)
{
    nsAutoTArray<nsString, 15> fonts;
    ForEachFont(aFamilies, aLangGroup, AddFontNameToArray, &fonts);

    PRUint32 len = fonts.Length();
    for (PRUint32 i = 0; i < len; ++i) {
        const nsString& str = fonts[i];
        nsRefPtr<FontEntry> fe = gfxToolkitPlatform::GetPlatform()->FindFontEntry(str, mStyle);
        aFontEntryList->AppendElement(fe);
    }
}

void gfxFT2FontGroup::GetPrefFonts(const char *aLangGroup, nsTArray<nsRefPtr<FontEntry> >& aFontEntryList) {
    NS_ASSERTION(aLangGroup, "aLangGroup is null");
    gfxToolkitPlatform *platform = gfxToolkitPlatform::GetPlatform();
    nsAutoTArray<nsRefPtr<FontEntry>, 5> fonts;
    
    nsCAutoString key(aLangGroup);
    key.Append("-");
    key.AppendInt(GetStyle()->style);
    key.Append("-");
    key.AppendInt(GetStyle()->weight);
    if (!platform->GetPrefFontEntries(key, &fonts)) {
        nsString fontString;
        platform->GetPrefFonts(aLangGroup, fontString);
        if (fontString.IsEmpty())
            return;

        FamilyListToArrayList(fontString, nsDependentCString(aLangGroup),
                                      &fonts);

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


void gfxFT2FontGroup::GetCJKPrefFonts(nsTArray<nsRefPtr<FontEntry> >& aFontEntryList) {
    gfxToolkitPlatform *platform = gfxToolkitPlatform::GetPlatform();

    nsCAutoString key("x-internal-cjk-");
    key.AppendInt(mStyle.style);
    key.Append("-");
    key.AppendInt(mStyle.weight);

    if (!platform->GetPrefFontEntries(key, &aFontEntryList)) {
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
                    GetPrefFonts(sCJKLangGroup[index], aFontEntryList);
                p++;
            }
        }

        
#ifdef XP_WIN
        switch (::GetACP()) {
            case 932: GetPrefFonts(CJK_LANG_JA, aFontEntryList); break;
            case 936: GetPrefFonts(CJK_LANG_ZH_CN, aFontEntryList); break;
            case 949: GetPrefFonts(CJK_LANG_KO, aFontEntryList); break;
            
            case 950: GetPrefFonts(CJK_LANG_ZH_TW, aFontEntryList); break;
        }
#else
        const char *ctype = setlocale(LC_CTYPE, NULL);
        if (ctype) {
            if (!PL_strncasecmp(ctype, "ja", 2)) {
                GetPrefFonts(CJK_LANG_JA, aFontEntryList);
            } else if (!PL_strncasecmp(ctype, "zh_cn", 5)) {
                GetPrefFonts(CJK_LANG_ZH_CN, aFontEntryList);
            } else if (!PL_strncasecmp(ctype, "zh_hk", 5)) {
                GetPrefFonts(CJK_LANG_ZH_HK, aFontEntryList);
            } else if (!PL_strncasecmp(ctype, "zh_tw", 5)) {
                GetPrefFonts(CJK_LANG_ZH_TW, aFontEntryList);
            } else if (!PL_strncasecmp(ctype, "ko", 2)) {
                GetPrefFonts(CJK_LANG_KO, aFontEntryList);
            }
        }
#endif

        
        GetPrefFonts(CJK_LANG_JA, aFontEntryList);
        GetPrefFonts(CJK_LANG_KO, aFontEntryList);
        GetPrefFonts(CJK_LANG_ZH_CN, aFontEntryList);
        GetPrefFonts(CJK_LANG_ZH_HK, aFontEntryList);
        GetPrefFonts(CJK_LANG_ZH_TW, aFontEntryList);

        platform->SetPrefFontEntries(key, aFontEntryList);
    }
}

already_AddRefed<gfxFT2Font>
gfxFT2FontGroup::WhichFontSupportsChar(const nsTArray<nsRefPtr<FontEntry> >& aFontEntryList, PRUint32 aCh)
{
    for (PRUint32 i = 0; i < aFontEntryList.Length(); i++) {
        nsRefPtr<FontEntry> fe = aFontEntryList[i];
        if (fe->HasCharacter(aCh)) {
            nsRefPtr<gfxFT2Font> font =
                gfxFT2Font::GetOrMakeFont(fe, &mStyle);
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

    
    nsAutoTArray<nsRefPtr<FontEntry>, 5> fonts;
    GetPrefFonts(mStyle.langGroup.get(), fonts);
    selectedFont = WhichFontSupportsChar(fonts, aCh);

    
    if (!selectedFont) {
        PRUint32 unicodeRange = FindCharUnicodeRange(aCh);

        
        if (unicodeRange == kRangeSetCJK) {
            if (PR_LOG_TEST(gFontLog, PR_LOG_DEBUG)) {
                PR_LOG(gFontLog, PR_LOG_DEBUG, (" - Trying to find fonts for: CJK"));
            }

            nsAutoTArray<nsRefPtr<FontEntry>, 15> fonts;
            GetCJKPrefFonts(fonts);
            selectedFont = WhichFontSupportsChar(fonts, aCh);
        } else {
            const char *langGroup = LangGroupFromUnicodeRange(unicodeRange);
            if (langGroup) {
                PR_LOG(gFontLog, PR_LOG_DEBUG, (" - Trying to find fonts for: %s", langGroup));

                nsAutoTArray<nsRefPtr<FontEntry>, 5> fonts;
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
    nsRefPtr<gfxFont> selectedFont;
    nsRefPtr<gfxFT2Font> refFont = GetFontAt(0);
    gfxToolkitPlatform *platform = gfxToolkitPlatform::GetPlatform();
    selectedFont = platform->FindFontForChar(aCh, refFont);
    if (selectedFont)
        return selectedFont.forget();
    return nsnull;
}

void gfxFT2FontGroup::CreateGlyphRunsFT(gfxTextRun *aTextRun)
{
    ComputeRanges(mRanges, mString.get(), 0, mString.Length());

    PRUint32 offset = 0;
    for (PRUint32 i = 0; i < mRanges.Length(); ++i) {
        const gfxTextRange& range = mRanges[i];
        PRUint32 rangeLength = range.Length();
        gfxFT2Font *font = static_cast<gfxFT2Font *>(range.font ? range.font.get() : GetFontAt(0));
        AddRange(aTextRun, font, mString.get(), offset, rangeLength);
        offset += rangeLength;
    }

}

void
gfxFT2FontGroup::AddRange(gfxTextRun *aTextRun, gfxFT2Font *font, const PRUnichar *str, PRUint32 offset, PRUint32 len)
{
    const PRUint32 appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();
    
    gfxFT2LockedFace faceLock(font);
    FT_Face face = faceLock.get();

    gfxTextRun::CompressedGlyph g;

    const gfxFT2Font::CachedGlyphData *cgd = nsnull, *cgdNext = nsnull;

    FT_UInt spaceGlyph = font->GetSpaceGlyph();

    aTextRun->AddGlyphRun(font, offset);
    for (PRUint32 i = 0; i < len; i++) {
        PRUint32 ch = str[offset + i];

        if (ch == 0) {
            
            aTextRun->SetMissingGlyph(offset + i, 0);
            continue;
        }

        NS_ASSERTION(!IsInvalidChar(ch), "Invalid char detected");

        if (cgdNext) {
            cgd = cgdNext;
            cgdNext = nsnull;
        } else {
            cgd = font->GetGlyphDataForChar(ch);
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
                    cgdNext = font->GetGlyphDataForChar(chNext);
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

            
            advance = MOZ_FT_TRUNC(advance) * appUnitsPerDevUnit;
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
            g.SetComplex(aTextRun->IsClusterStart(offset + i), PR_TRUE, 1);
            aTextRun->SetGlyphs(offset + i, g, &details);
        }
    }
}




gfxFT2Font::gfxFT2Font(cairo_scaled_font_t *aCairoFont,
                       FontEntry *aFontEntry,
                       const gfxFontStyle *aFontStyle)
    : gfxFT2FontBase(aCairoFont, aFontEntry, aFontStyle)
{
    NS_ASSERTION(mFontEntry, "Unable to find font entry for font.  Something is whack.");

    mCharGlyphCache.Init(64);
}

gfxFT2Font::~gfxFT2Font()
{
}

cairo_font_face_t *
gfxFT2Font::CairoFontFace()
{
    
    if (mStyle.weight >= 600 && mFontEntry->mWeight < 600) {
        
    }
    return GetFontEntry()->CairoFontFace();
}

static cairo_scaled_font_t *
CreateScaledFont(FontEntry *aFontEntry, const gfxFontStyle *aStyle)
{
    cairo_scaled_font_t *scaledFont = NULL;

    cairo_matrix_t sizeMatrix;
    cairo_matrix_t identityMatrix;

    
    cairo_matrix_init_scale(&sizeMatrix, aStyle->size, aStyle->size);
    cairo_matrix_init_identity(&identityMatrix);

    
    PRBool needsOblique = (!aFontEntry->mItalic && (aStyle->style & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE)));

    if (needsOblique) {
        const double kSkewFactor = 0.25;

        cairo_matrix_t style;
        cairo_matrix_init(&style,
                          1,                
                          0,                
                          -1 * kSkewFactor,  
                          1,                
                          0,                
                          0);               
        cairo_matrix_multiply(&sizeMatrix, &sizeMatrix, &style);
    }

    cairo_font_options_t *fontOptions = cairo_font_options_create();
    scaledFont = cairo_scaled_font_create(aFontEntry->CairoFontFace(),
                                          &sizeMatrix,
                                          &identityMatrix, fontOptions);
    cairo_font_options_destroy(fontOptions);

    NS_ASSERTION(cairo_scaled_font_status(scaledFont) == CAIRO_STATUS_SUCCESS,
                 "Failed to make scaled font");

    return scaledFont;
}






already_AddRefed<gfxFT2Font>
gfxFT2Font::GetOrMakeFont(const nsAString& aName, const gfxFontStyle *aStyle)
{
    FontEntry *fe = gfxToolkitPlatform::GetPlatform()->FindFontEntry(aName, *aStyle);
    if (!fe) {
        NS_WARNING("Failed to find font entry for font!");
        return nsnull;
    }

    nsRefPtr<gfxFT2Font> font = GetOrMakeFont(fe, aStyle);
    return font.forget();
}

already_AddRefed<gfxFT2Font>
gfxFT2Font::GetOrMakeFont(FontEntry *aFontEntry, const gfxFontStyle *aStyle)
{
    nsRefPtr<gfxFont> font = gfxFontCache::GetCache()->Lookup(aFontEntry->Name(), aStyle);
    if (!font) {
        cairo_scaled_font_t *scaledFont = CreateScaledFont(aFontEntry, aStyle);
        font = new gfxFT2Font(scaledFont, aFontEntry, aStyle);
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

    FT_Error err = FT_Load_Glyph(face, gid, FT_LOAD_DEFAULT);

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
