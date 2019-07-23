







































#include "gfxWindowsPlatform.h"

#include "gfxImageSurface.h"
#include "gfxWindowsSurface.h"

#include "nsUnicharUtils.h"

#include "nsIPref.h"
#include "nsServiceManagerUtils.h"

#include "nsIWindowsRegKey.h"

#include "gfxWindowsFonts.h"

#include <string>

#include "lcms.h"



static nsresult ReadCMAP(HDC hdc, FontEntry *aFontEntry);

int PR_CALLBACK
gfxWindowsPlatform::PrefChangedCallback(const char *aPrefName, void *closure)
{
    
    
    gfxWindowsPlatform *plat = static_cast<gfxWindowsPlatform *>(closure);
    plat->mPrefFonts.Clear();
    return 0;
}

gfxWindowsPlatform::gfxWindowsPlatform()
{
    mFonts.Init(200);
    mFontAliases.Init(20);
    mFontSubstitutes.Init(50);
    mPrefFonts.Init(10);

    UpdateFontList();

    nsCOMPtr<nsIPref> pref = do_GetService(NS_PREF_CONTRACTID);
    pref->RegisterCallback("font.", PrefChangedCallback, this);
    pref->RegisterCallback("font.name-list.", PrefChangedCallback, this);
    pref->RegisterCallback("intl.accept_languages", PrefChangedCallback, this);
    
}

gfxWindowsPlatform::~gfxWindowsPlatform()
{
}

already_AddRefed<gfxASurface>
gfxWindowsPlatform::CreateOffscreenSurface(const gfxIntSize& size,
                                           gfxASurface::gfxImageFormat imageFormat)
{
    gfxASurface *surf = new gfxWindowsSurface(size, imageFormat);
    NS_IF_ADDREF(surf);
    return surf;
}

int CALLBACK 
gfxWindowsPlatform::FontEnumProc(const ENUMLOGFONTEXW *lpelfe,
                                 const NEWTEXTMETRICEXW *nmetrics,
                                 DWORD fontType, LPARAM data)
{
    FontTable *ht = reinterpret_cast<FontTable*>(data);

    const NEWTEXTMETRICW& metrics = nmetrics->ntmTm;
    LOGFONTW logFont = lpelfe->elfLogFont;

    
    if (logFont.lfFaceName[0] == L'@') {
        return 1;
    }

#ifdef DEBUG_pavlov
    printf("%s %d %d %d\n", NS_ConvertUTF16toUTF8(nsDependentString(logFont.lfFaceName)).get(),
           logFont.lfCharSet, logFont.lfItalic, logFont.lfWeight);
#endif

    nsString name(logFont.lfFaceName);
    ToLowerCase(name);

    nsRefPtr<FontFamily> ff;
    if (!ht->Get(name, &ff)) {
        ff = new FontFamily(nsDependentString(logFont.lfFaceName));
        ht->Put(name, ff);
    }

    nsRefPtr<FontEntry> fe;
    for (PRUint32 i = 0; i < ff->mVariations.Length(); ++i) {
        fe = ff->mVariations[i];
        if (fe->mWeight == logFont.lfWeight &&
            fe->mItalic == (logFont.lfItalic == 0xFF)) {
            return 1; 
        }
    }

    fe = new FontEntry(ff);
    

    fe->mItalic = (logFont.lfItalic == 0xFF);
    fe->mWeight = logFont.lfWeight;

    if (metrics.ntmFlags & NTM_TYPE1)
        fe->mIsType1 = PR_TRUE;
    if (metrics.ntmFlags & (NTM_PS_OPENTYPE | NTM_TT_OPENTYPE))
        fe->mTrueType = PR_TRUE;

    
    fe->mCharset[metrics.tmCharSet] = 1;

    fe->mWindowsFamily = logFont.lfPitchAndFamily & 0xF0;
    fe->mWindowsPitch = logFont.lfPitchAndFamily & 0x0F;

    if (nmetrics->ntmFontSig.fsUsb[0] == 0x00000000 &&
        nmetrics->ntmFontSig.fsUsb[1] == 0x00000000 &&
        nmetrics->ntmFontSig.fsUsb[2] == 0x00000000 &&
        nmetrics->ntmFontSig.fsUsb[3] == 0x00000000) {
        
        fe->mUnicodeFont = PR_FALSE;
    } else {
        fe->mUnicodeFont = PR_TRUE;

        
        PRUint32 x = 0;
        for (PRUint32 i = 0; i < 4; ++i) {
            DWORD range = nmetrics->ntmFontSig.fsUsb[i];
            for (PRUint32 k = 0; k < 32; ++k) {
                fe->mUnicodeRanges[x++] = (range & (1 << k)) != 0;
            }
        }
    }

    
    HDC hdc = GetDC(nsnull);
    logFont.lfCharSet = DEFAULT_CHARSET;
    HFONT font = CreateFontIndirectW(&logFont);

    NS_ASSERTION(font, "This font creation should never ever ever fail");
    if (font) {
        HFONT oldFont = (HFONT)SelectObject(hdc, font);

        TEXTMETRIC metrics;
        GetTextMetrics(hdc, &metrics);
        if (metrics.tmPitchAndFamily & TMPF_TRUETYPE)
            fe->mTrueType = PR_TRUE;

        if (NS_FAILED(ReadCMAP(hdc, fe))) {
            
            
            if (fe->mIsType1)
                fe->mUnicodeFont = PR_TRUE;
            else
                fe->mUnicodeFont = PR_FALSE;

            
        }

        SelectObject(hdc, oldFont);
        DeleteObject(font);
    }

    ReleaseDC(nsnull, hdc);

    if (!fe->mUnicodeFont) {
        






        fe->mCharacterMap.SetRange(0x20, 0xFF);
    }

    
    ff->mVariations.AppendElement(fe);

    return 1;
}



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
    
    return gfxFontUtils::ReadCMAP(buf, len, aFontEntry->mCharacterMap, aFontEntry->mUnicodeRanges,
                                  aFontEntry->mUnicodeFont, aFontEntry->mSymbolFont);
}

PLDHashOperator PR_CALLBACK
gfxWindowsPlatform::FontGetStylesProc(nsStringHashKey::KeyType aKey,
                                      nsRefPtr<FontFamily>& aFontFamily,
                                      void* userArg)
{
    NS_ASSERTION(aFontFamily->mVariations.Length() == 1, "We should only have 1 variation here");
    nsRefPtr<FontEntry> aFontEntry = aFontFamily->mVariations[0];

    HDC hdc = GetDC(nsnull);

    LOGFONTW logFont;
    memset(&logFont, 0, sizeof(LOGFONTW));
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfPitchAndFamily = 0;
    PRUint32 l = PR_MIN(aFontEntry->GetName().Length(), LF_FACESIZE - 1);
    memcpy(logFont.lfFaceName,
           nsPromiseFlatString(aFontEntry->GetName()).get(),
           l * sizeof(PRUnichar));
    logFont.lfFaceName[l] = 0;

    EnumFontFamiliesExW(hdc, &logFont, (FONTENUMPROCW)gfxWindowsPlatform::FontEnumProc, (LPARAM)userArg, 0);

    ReleaseDC(nsnull, hdc);

    return PL_DHASH_NEXT;
}

struct FontListData {
    FontListData(const nsACString& aLangGroup, const nsACString& aGenericFamily, nsStringArray& aListOfFonts) :
        mLangGroup(aLangGroup), mGenericFamily(aGenericFamily), mStringArray(aListOfFonts) {}
    const nsACString& mLangGroup;
    const nsACString& mGenericFamily;
    nsStringArray& mStringArray;
};

PLDHashOperator PR_CALLBACK
gfxWindowsPlatform::HashEnumFunc(nsStringHashKey::KeyType aKey,
                                 nsRefPtr<FontFamily>& aFontFamily,
                                 void* userArg)
{
    FontListData *data = (FontListData*)userArg;

    
    
    
    nsRefPtr<FontEntry> aFontEntry = aFontFamily->mVariations[0];

    
    if (aFontEntry->mSymbolFont)
        return PL_DHASH_NEXT;

    if (aFontEntry->SupportsLangGroup(data->mLangGroup) &&
        aFontEntry->MatchesGenericFamily(data->mGenericFamily))
        data->mStringArray.AppendString(aFontFamily->mName);

    return PL_DHASH_NEXT;
}

nsresult
gfxWindowsPlatform::GetFontList(const nsACString& aLangGroup,
                                const nsACString& aGenericFamily,
                                nsStringArray& aListOfFonts)
{
    FontListData data(aLangGroup, aGenericFamily, aListOfFonts);

    mFonts.Enumerate(gfxWindowsPlatform::HashEnumFunc, &data);

    aListOfFonts.Sort();
    aListOfFonts.Compact();

    return NS_OK;
}

static void
RemoveCharsetFromFontSubstitute(nsAString &aName)
{
    PRInt32 comma = aName.FindChar(PRUnichar(','));
    if (comma >= 0)
        aName.Truncate(comma);
}

static void
BuildKeyNameFromFontName(nsAString &aName)
{
    if (aName.Length() >= LF_FACESIZE)
        aName.Truncate(LF_FACESIZE - 1);
    ToLowerCase(aName);
}

nsresult
gfxWindowsPlatform::UpdateFontList()
{
    mFonts.Clear();
    mFontAliases.Clear();
    mNonExistingFonts.Clear();
    mFontSubstitutes.Clear();
    mPrefFonts.Clear();
    mCodepointsWithNoFonts.reset();

    LOGFONTW logFont;
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfFaceName[0] = 0;
    logFont.lfPitchAndFamily = 0;

    
    HDC dc = ::GetDC(nsnull);
    EnumFontFamiliesExW(dc, &logFont, (FONTENUMPROCW)gfxWindowsPlatform::FontEnumProc, (LPARAM)&mFonts, 0);
    ::ReleaseDC(nsnull, dc);

    
    mFonts.Enumerate(gfxWindowsPlatform::FontGetStylesProc, &mFonts);

    
    nsCOMPtr<nsIWindowsRegKey> regKey = do_CreateInstance("@mozilla.org/windows-registry-key;1");
    if (!regKey)
        return NS_ERROR_FAILURE;
     NS_NAMED_LITERAL_STRING(kFontSubstitutesKey, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes");

    nsresult rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_LOCAL_MACHINE,
                               kFontSubstitutesKey, nsIWindowsRegKey::ACCESS_READ);
    if (NS_FAILED(rv))
        return rv;

    PRUint32 count;
    rv = regKey->GetValueCount(&count);
    if (NS_FAILED(rv) || count == 0)
        return rv;
    for (PRUint32 i = 0; i < count; i++) {
        nsAutoString substituteName;
        rv = regKey->GetValueName(i, substituteName);
        if (NS_FAILED(rv) || substituteName.IsEmpty() ||
            substituteName.CharAt(1) == PRUnichar('@'))
            continue;
        PRUint32 valueType;
        rv = regKey->GetValueType(substituteName, &valueType);
        if (NS_FAILED(rv) || valueType != nsIWindowsRegKey::TYPE_STRING)
            continue;
        nsAutoString actualFontName;
        rv = regKey->ReadStringValue(substituteName, actualFontName);
        if (NS_FAILED(rv))
            continue;

        RemoveCharsetFromFontSubstitute(substituteName);
        BuildKeyNameFromFontName(substituteName);
        RemoveCharsetFromFontSubstitute(actualFontName);
        BuildKeyNameFromFontName(actualFontName);
        nsRefPtr<FontFamily> ff;
        if (!actualFontName.IsEmpty() && mFonts.Get(actualFontName, &ff))
            mFontSubstitutes.Put(substituteName, ff);
        else
            mNonExistingFonts.AppendString(substituteName);
    }

    
    mCodepointsWithNoFonts.SetRange(0,0x1f);     
    mCodepointsWithNoFonts.SetRange(0x7f,0x9f);  

    InitBadUnderlineList();

    return NS_OK;
}

static PRBool SimpleResolverCallback(const nsAString& aName, void* aClosure)
{
    nsString* result = static_cast<nsString*>(aClosure);
    result->Assign(aName);
    return PR_FALSE;
}

void
gfxWindowsPlatform::InitBadUnderlineList()
{
    nsAutoTArray<nsAutoString, 10> blacklist;
    gfxFontUtils::GetPrefsFontList("font.blacklist.underline_offset", blacklist);
    PRUint32 numFonts = blacklist.Length();
    for (PRUint32 i = 0; i < numFonts; i++) {
        PRBool aborted;
        nsAutoString resolved;
        ResolveFontName(blacklist[i], SimpleResolverCallback, &resolved, aborted);
        if (resolved.IsEmpty())
            continue;
        FontFamily *ff = FindFontFamily(resolved);
        if (!ff)
            continue;
        for (PRUint32 j = 0; j < ff->mVariations.Length(); ++j) {
            nsRefPtr<FontEntry> fe = ff->mVariations[j];
            fe->mIsBadUnderlineFont = 1;
        }
    }
}

nsresult
gfxWindowsPlatform::GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName)
{
    aFamilyName.Truncate();
    PRBool aborted;
    return ResolveFontName(aFontName, SimpleResolverCallback, &aFamilyName, aborted);
}

struct ResolveData {
    ResolveData(gfxPlatform::FontResolverCallback aCallback,
                gfxWindowsPlatform *aCaller, const nsAString *aFontName,
                void *aClosure) :
        mFoundCount(0), mCallback(aCallback), mCaller(aCaller),
        mFontName(aFontName), mClosure(aClosure) {}
    PRUint32 mFoundCount;
    gfxPlatform::FontResolverCallback mCallback;
    gfxWindowsPlatform *mCaller;
    const nsAString *mFontName;
    void *mClosure;
};

nsresult
gfxWindowsPlatform::ResolveFontName(const nsAString& aFontName,
                                    FontResolverCallback aCallback,
                                    void *aClosure,
                                    PRBool& aAborted)
{
    if (aFontName.IsEmpty())
        return NS_ERROR_FAILURE;

    nsAutoString keyName(aFontName);
    BuildKeyNameFromFontName(keyName);

    nsRefPtr<FontFamily> ff;
    if (mFonts.Get(keyName, &ff) ||
        mFontSubstitutes.Get(keyName, &ff) ||
        mFontAliases.Get(keyName, &ff)) {
        aAborted = !(*aCallback)(ff->mName, aClosure);
        
        return NS_OK;
    }

    if (mNonExistingFonts.IndexOf(keyName) >= 0) {
        aAborted = PR_FALSE;
        return NS_OK;
    }

    LOGFONTW logFont;
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfPitchAndFamily = 0;
    PRInt32 len = aFontName.Length();
    if (len >= LF_FACESIZE)
        len = LF_FACESIZE - 1;
    memcpy(logFont.lfFaceName,
           nsPromiseFlatString(aFontName).get(), len * sizeof(PRUnichar));
    logFont.lfFaceName[len] = 0;

    HDC dc = ::GetDC(nsnull);
    ResolveData data(aCallback, this, &keyName, aClosure);
    aAborted = !EnumFontFamiliesExW(dc, &logFont,
                                    (FONTENUMPROCW)gfxWindowsPlatform::FontResolveProc,
                                    (LPARAM)&data, 0);
    if (data.mFoundCount == 0)
        mNonExistingFonts.AppendString(keyName);
    ::ReleaseDC(nsnull, dc);

    return NS_OK;
}

int CALLBACK 
gfxWindowsPlatform::FontResolveProc(const ENUMLOGFONTEXW *lpelfe,
                                    const NEWTEXTMETRICEXW *nmetrics,
                                    DWORD fontType, LPARAM data)
{
    const LOGFONTW& logFont = lpelfe->elfLogFont;
    
    if (logFont.lfFaceName[0] == L'@' || logFont.lfFaceName[0] == 0)
        return 1;

    ResolveData *rData = reinterpret_cast<ResolveData*>(data);

    nsAutoString name(logFont.lfFaceName);

    
    nsRefPtr<FontFamily> ff;
    nsAutoString keyName(name);
    BuildKeyNameFromFontName(keyName);
    if (!rData->mCaller->mFonts.Get(keyName, &ff)) {
        
        
        
        
        NS_WARNING("Cannot find actual font");
        return 1;
    }

    rData->mFoundCount++;
    rData->mCaller->mFontAliases.Put(*(rData->mFontName), ff);

    return (rData->mCallback)(name, rData->mClosure);

    
}

struct FontSearch {
    FontSearch(PRUint32 aCh, gfxWindowsFont *aFont) :
        ch(aCh), fontToMatch(aFont), matchRank(0) {
    }
    PRUint32 ch;
    nsRefPtr<gfxWindowsFont> fontToMatch;
    PRInt32 matchRank;
    nsRefPtr<FontEntry> bestMatch;
};

PLDHashOperator PR_CALLBACK
gfxWindowsPlatform::FindFontForCharProc(nsStringHashKey::KeyType aKey,
                                        nsRefPtr<FontFamily>& aFontFamily,
                                        void* userArg)
{
    FontSearch *data = (FontSearch*)userArg;

    const PRUint32 ch = data->ch;

    nsRefPtr<FontEntry> fe = GetPlatform()->FindFontEntry(aFontFamily, data->fontToMatch->GetStyle());

    
    
    if (fe->IsCrappyFont() || !fe->mCharacterMap.test(ch))
        return PL_DHASH_NEXT;

    PRInt32 rank = 0;
    
    
    if (fe->SupportsRange(gfxFontUtils::CharRangeBit(ch)))
        rank += 1;

    if (fe->SupportsLangGroup(data->fontToMatch->GetStyle()->langGroup))
        rank += 2;

    if (fe->mWindowsFamily == data->fontToMatch->GetFontEntry()->mWindowsFamily)
        rank += 3;
    if (fe->mWindowsPitch == data->fontToMatch->GetFontEntry()->mWindowsFamily)
        rank += 3;

    
    const PRBool italic = (data->fontToMatch->GetStyle()->style & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE)) ? PR_TRUE : PR_FALSE;
    if (fe->mItalic != italic)
        rank += 3;

    
    PRInt8 baseWeight, weightDistance;
    data->fontToMatch->GetStyle()->ComputeWeightAndOffset(&baseWeight, &weightDistance);
    if (fe->mWeight == (baseWeight * 100) + (weightDistance * 100))
        rank += 2;
    else if (fe->mWeight == data->fontToMatch->GetFontEntry()->mWeight)
        rank += 1;

    if (rank > data->matchRank ||
        (rank == data->matchRank && Compare(fe->GetName(), data->bestMatch->GetName()) > 0)) {
        data->bestMatch = fe;
        data->matchRank = rank;
    }

    return PL_DHASH_NEXT;
}


FontEntry *
gfxWindowsPlatform::FindFontForChar(PRUint32 aCh, gfxWindowsFont *aFont)
{
    
    if (mCodepointsWithNoFonts.test(aCh)) {
        return nsnull;
    }

    FontSearch data(aCh, aFont);

    
    mFonts.Enumerate(gfxWindowsPlatform::FindFontForCharProc, &data);

    
    if (!data.bestMatch) {
        mCodepointsWithNoFonts.set(aCh);
    }
    
    return data.bestMatch;
}

gfxFontGroup *
gfxWindowsPlatform::CreateFontGroup(const nsAString &aFamilies,
                                    const gfxFontStyle *aStyle)
{
    return new gfxWindowsFontGroup(aFamilies, aStyle);
}

FontFamily *
gfxWindowsPlatform::FindFontFamily(const nsAString& aName)
{
    nsString name(aName);
    ToLowerCase(name);

    nsRefPtr<FontFamily> ff;
    if (!mFonts.Get(name, &ff) &&
        !mFontSubstitutes.Get(name, &ff) &&
        !mFontAliases.Get(name, &ff)) {
        return nsnull;
    }
    return ff.get();
}

FontEntry *
gfxWindowsPlatform::FindFontEntry(const nsAString& aName, const gfxFontStyle *aFontStyle)
{
    nsRefPtr<FontFamily> ff = FindFontFamily(aName);
    if (!ff)
        return nsnull;

    return FindFontEntry(ff, aFontStyle);
}

FontEntry *
gfxWindowsPlatform::FindFontEntry(FontFamily *aFontFamily, const gfxFontStyle *aFontStyle)
{
    PRUint8 bestMatch = 0;
    nsRefPtr<FontEntry> matchFE;
    const PRBool italic = (aFontStyle->style & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE)) ? PR_TRUE : PR_FALSE;

    
    nsAutoTArray<nsRefPtr<FontEntry>, 10> weightList;
    weightList.AppendElements(10);
    for (PRInt32 i = 0; i < aFontFamily->mVariations.Length(); i++) {
        nsRefPtr<FontEntry> fe = aFontFamily->mVariations[i];
        const PRUint8 weight = (fe->mWeight / 100) - 1;
        if (fe->mItalic == italic)
            weightList[weight] = fe;
    }

    PRInt8 baseWeight, weightDistance;
    aFontStyle->ComputeWeightAndOffset(&baseWeight, &weightDistance);

    PRUint32 chosenWeight = 0;
    PRUint8 direction = (weightDistance >= 0) ? 1 : -1;

 WEIGHT_SELECTION:
    for (PRUint8 i = baseWeight, k = 0; i < 10 && i >= 1; i+=direction) {
        if (weightList[i - 1]) {
            k++;
            chosenWeight = i * 100;
        }
        if (k > abs(weightDistance)) {
            chosenWeight = i * 100;
            break;
        }
    }

    
    
    if (baseWeight == 5 && weightDistance == 0 && chosenWeight > 500) {
        baseWeight = 4;
        goto WEIGHT_SELECTION;
    }

    
    if (chosenWeight == 0)
        chosenWeight = baseWeight * 100;

    
    
    const PRUint32 index = (chosenWeight / 100) - 1;
    for (PRInt32 j = index; j >= 0; --j) {
        if (matchFE = weightList[j])
            break;
    }

    if (!matchFE) {
        
        
        
        
        gfxFontStyle style(*aFontStyle);
        if (aFontStyle->style == FONT_STYLE_NORMAL)
            style.style = FONT_STYLE_ITALIC;
        else
            style.style = FONT_STYLE_NORMAL;

        matchFE = FindFontEntry(aFontFamily, &style);
    }
    return matchFE;
}

cmsHPROFILE
gfxWindowsPlatform::GetPlatformCMSOutputProfile()
{
    WCHAR str[1024+1];
    DWORD size = 1024;

    HDC dc = GetDC(nsnull);
    GetICMProfileW(dc, &size, (LPWSTR)&str);
    ReleaseDC(nsnull, dc);

    cmsHPROFILE profile =
        cmsOpenProfileFromFile(NS_ConvertUTF16toUTF8(str).get(), "r");
#ifdef DEBUG_tor
    if (profile)
        fprintf(stderr,
                "ICM profile read from %s successfully\n",
                NS_ConvertUTF16toUTF8(str).get());
#endif
    return profile;
}

PRBool
gfxWindowsPlatform::GetPrefFontEntries(const nsCString& aKey, nsTArray<nsRefPtr<FontEntry> > *array)
{
    return mPrefFonts.Get(aKey, array);
}

void
gfxWindowsPlatform::SetPrefFontEntries(const nsCString& aKey, nsTArray<nsRefPtr<FontEntry> >& array)
{
    mPrefFonts.Put(aKey, array);
}
