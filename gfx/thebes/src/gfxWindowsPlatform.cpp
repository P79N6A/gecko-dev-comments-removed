







































#include "gfxWindowsPlatform.h"

#include "gfxImageSurface.h"
#include "gfxWindowsSurface.h"

#include "nsUnicharUtils.h"

#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"
#include "nsServiceManagerUtils.h"
#include "nsTArray.h"

#include "nsIWindowsRegKey.h"
#include "nsILocalFile.h"
#include "plbase64.h"

#ifdef MOZ_FT2_FONTS
#include "ft2build.h"
#include FT_FREETYPE_H
#include "gfxFT2Fonts.h"
#include "cairo-ft.h"
#include "nsAppDirectoryServiceDefs.h"
#else
#include "gfxWindowsFonts.h"
#endif


#include "cairo.h"

#ifdef WINCE
#include <shlwapi.h>

#ifdef CAIRO_HAS_DDRAW_SURFACE
#include "gfxDDrawSurface.h"
#endif
#endif

#include "gfxUserFontSet.h"

#include <string>

#ifdef MOZ_FT2_FONTS
static FT_Library gPlatformFTLibrary = NULL;
#endif


static const PRUint32 kDelayBeforeLoadingCmaps = 8 * 1000; 
static const PRUint32 kIntervalBetweenLoadingCmaps = 150; 
static const PRUint32 kNumFontsPerSlice = 10; 

static __inline void
BuildKeyNameFromFontName(nsAString &aName)
{
    if (aName.Length() >= LF_FACESIZE)
        aName.Truncate(LF_FACESIZE - 1);
    ToLowerCase(aName);
}

class gfxWindowsPlatformPrefObserver : public nsIObserver {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
};

NS_IMPL_ISUPPORTS1(gfxWindowsPlatformPrefObserver, nsIObserver)

NS_IMETHODIMP
gfxWindowsPlatformPrefObserver::Observe(nsISupports     *aSubject,
                                        const char      *aTopic,
                                        const PRUnichar *aData)
{
    NS_ASSERTION(!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID), "invalid topic");
    
    
    gfxWindowsPlatform::GetPlatform()->ClearPrefFonts();
    return NS_OK;
}

gfxWindowsPlatform::gfxWindowsPlatform()
    : mStartIndex(0), mIncrement(kNumFontsPerSlice), mNumFamilies(0)
{
    mFonts.Init(200);
    mFontAliases.Init(20);
    mFontSubstitutes.Init(50);
    mPrefFonts.Init(10);

#ifdef MOZ_FT2_FONTS
    FT_Init_FreeType(&gPlatformFTLibrary);
#else
    FontEntry::InitializeFontEmbeddingProcs();
#endif

    UpdateFontList();

    nsCOMPtr<nsIPrefBranch2> pref = do_GetService(NS_PREFSERVICE_CONTRACTID);

    if (pref) {
        gfxWindowsPlatformPrefObserver *observer = new gfxWindowsPlatformPrefObserver();
        if (observer) {
            pref->AddObserver("font.", observer, PR_FALSE);
            pref->AddObserver("font.name-list.", observer, PR_FALSE);
            pref->AddObserver("intl.accept_languages", observer, PR_FALSE);
            
        }
    }




#if defined(WINCE_WINDOWS_MOBILE)
    mRenderMode = RENDER_IMAGE_DDRAW16;
#elif defined(WINCE)
    mRenderMode = RENDER_DDRAW_GL;
#else
    mRenderMode = RENDER_GDI;
#endif

    PRInt32 rmode;
    if (NS_SUCCEEDED(pref->GetIntPref("mozilla.widget.render-mode", &rmode))) {
        if (rmode >= 0 || rmode < RENDER_MODE_MAX) {
#ifndef CAIRO_HAS_DDRAW_SURFACE
            if (rmode == RENDER_DDRAW || rmode == RENDER_DDRAW_GL)
                rmode = RENDER_IMAGE_STRETCH24;
#endif
            mRenderMode = (RenderMode) rmode;
        }
    }
}

gfxWindowsPlatform::~gfxWindowsPlatform()
{
    
    
}

already_AddRefed<gfxASurface>
gfxWindowsPlatform::CreateOffscreenSurface(const gfxIntSize& size,
                                           gfxASurface::gfxImageFormat imageFormat)
{
    gfxASurface *surf = nsnull;

#ifdef CAIRO_HAS_DDRAW_SURFACE
    if (mRenderMode == RENDER_DDRAW || mRenderMode == RENDER_DDRAW_GL)
        surf = new gfxDDrawSurface(NULL, size, imageFormat);
#endif

#ifdef CAIRO_HAS_WIN32_SURFACE
    if (mRenderMode == RENDER_GDI)
        surf = new gfxWindowsSurface(size, imageFormat);
#endif

    if (surf == nsnull)
        surf = new gfxImageSurface(size, imageFormat);

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
    const LOGFONTW& logFont = lpelfe->elfLogFont;

    
    if (logFont.lfFaceName[0] == L'@')
        return 1;

    nsAutoString name(logFont.lfFaceName);
    BuildKeyNameFromFontName(name);

    nsRefPtr<FontFamily> ff;
    if (!ht->Get(name, &ff)) {
        ff = new FontFamily(nsDependentString(logFont.lfFaceName));
        ht->Put(name, ff);
    }

    return 1;
}




struct FontListData {
    FontListData(const nsACString& aLangGroup, const nsACString& aGenericFamily, nsTArray<nsString>& aListOfFonts) :
        mLangGroup(aLangGroup), mGenericFamily(aGenericFamily), mStringArray(aListOfFonts) {}
    const nsACString& mLangGroup;
    const nsACString& mGenericFamily;
    nsTArray<nsString>& mStringArray;
};

PLDHashOperator
gfxWindowsPlatform::HashEnumFunc(nsStringHashKey::KeyType aKey,
                                 nsRefPtr<FontFamily>& aFontFamily,
                                 void* userArg)
{
    FontListData *data = (FontListData*)userArg;

    
    
    
    gfxFontStyle style;
    style.langGroup = data->mLangGroup;
    nsRefPtr<FontEntry> aFontEntry = aFontFamily->FindFontEntry(style);
    NS_ASSERTION(aFontEntry, "couldn't find any font entry in family");
    if (!aFontEntry)
        return PL_DHASH_NEXT;


#ifndef MOZ_FT2_FONTS
    
    if (aFontEntry->mSymbolFont)
        return PL_DHASH_NEXT;

    if (aFontEntry->SupportsLangGroup(data->mLangGroup) &&
        aFontEntry->MatchesGenericFamily(data->mGenericFamily))
#endif
    data->mStringArray.AppendElement(aFontFamily->Name());

    return PL_DHASH_NEXT;
}

nsresult
gfxWindowsPlatform::GetFontList(const nsACString& aLangGroup,
                                const nsACString& aGenericFamily,
                                nsTArray<nsString>& aListOfFonts)
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

#ifdef MOZ_FT2_FONTS
void gfxWindowsPlatform::AppendFacesFromFontFile(const PRUnichar *aFileName) {
    char fileName[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, aFileName, -1, fileName, MAX_PATH, NULL, NULL);
    FT_Face dummy;
    if (FT_Err_Ok == FT_New_Face(GetFTLibrary(), fileName, -1, &dummy)) {
        for (FT_Long i = 0; i < dummy->num_faces; i++) {
            FT_Face face;
            if (FT_Err_Ok != FT_New_Face(GetFTLibrary(), fileName, 
                                         i, &face))
                continue;

            FontEntry* fe = FontEntry::CreateFontEntryFromFace(face);
            if (fe) {
                NS_ConvertUTF8toUTF16 name(face->family_name);
                BuildKeyNameFromFontName(name);       
                nsRefPtr<FontFamily> ff;
                if (!mFonts.Get(name, &ff)) {
                    ff = new FontFamily(name);
                    mFonts.Put(name, ff);
                }
                ff->AddFontEntry(fe);
                ff->SetHasStyles(PR_TRUE);
            }
        }
        FT_Done_Face(dummy);
    }
}

void
gfxWindowsPlatform::FindFonts()
{
    nsTArray<nsString> searchPaths(3);
    nsTArray<nsString> fontPatterns(3);
    fontPatterns.AppendElement(NS_LITERAL_STRING("\\*.ttf"));
    fontPatterns.AppendElement(NS_LITERAL_STRING("\\*.ttc"));
    fontPatterns.AppendElement(NS_LITERAL_STRING("\\*.otf"));
    wchar_t pathBuf[256];
    SHGetSpecialFolderPathW(0, pathBuf, CSIDL_WINDOWS, 0);
    searchPaths.AppendElement(pathBuf);
    SHGetSpecialFolderPathW(0, pathBuf, CSIDL_FONTS, 0);
    searchPaths.AppendElement(pathBuf);
    nsCOMPtr<nsIFile> resDir;
    NS_GetSpecialDirectory(NS_APP_RES_DIR, getter_AddRefs(resDir));
    if (resDir) {
        resDir->Append(NS_LITERAL_STRING("fonts"));
        nsAutoString resPath;
        resDir->GetPath(resPath);
        searchPaths.AppendElement(resPath);
    }
    WIN32_FIND_DATAW results;
    for (PRUint32 i = 0;  i < searchPaths.Length(); i++) {
        const nsString& path(searchPaths[i]);
        for (PRUint32 j = 0; j < fontPatterns.Length(); j++) { 
            nsAutoString pattern(path);
            pattern.Append(fontPatterns[j]);
            HANDLE handle = FindFirstFileExW(pattern.get(),
                                             FindExInfoStandard,
                                             &results,
                                             FindExSearchNameMatch,
                                             NULL,
                                             0);
            PRBool moreFiles = handle != INVALID_HANDLE_VALUE;
            while (moreFiles) {
                nsAutoString filePath(path);
                filePath.AppendLiteral("\\");
                filePath.Append(results.cFileName);
                AppendFacesFromFontFile(static_cast<const PRUnichar*>(filePath.get()));
                moreFiles = FindNextFile(handle, &results);
            }
            if (handle != INVALID_HANDLE_VALUE)
                FindClose(handle);
        }
    }
}

#endif


nsresult
gfxWindowsPlatform::UpdateFontList()
{
    gfxFontCache *fc = gfxFontCache::GetCache();
    if (fc)
        fc->AgeAllGenerations();
    mFonts.Clear();
    mFontAliases.Clear();
    mNonExistingFonts.Clear();
    mFontSubstitutes.Clear();
    mPrefFonts.Clear();
    mCodepointsWithNoFonts.reset();
    CancelLoader();
#ifdef MOZ_FT2_FONTS
    FindFonts();
#else    
    LOGFONTW logFont;
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfFaceName[0] = 0;
    logFont.lfPitchAndFamily = 0;

    
    HDC dc = ::GetDC(nsnull);
    EnumFontFamiliesExW(dc, &logFont, (FONTENUMPROCW)gfxWindowsPlatform::FontEnumProc, (LPARAM)&mFonts, 0);
    ::ReleaseDC(nsnull, dc);
#endif
    
    StartLoader(kDelayBeforeLoadingCmaps, kIntervalBetweenLoadingCmaps); 

    
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
            mNonExistingFonts.AppendElement(substituteName);
    }

    
    mCodepointsWithNoFonts.SetRange(0,0x1f);     
    mCodepointsWithNoFonts.SetRange(0x7f,0x9f);  

    InitBadUnderlineList();

    return NS_OK;
}

struct FontFamilyListData {
    FontFamilyListData(nsTArray<nsRefPtr<FontFamily> >& aFamilyArray) 
        : mFamilyArray(aFamilyArray)
    {}

    static PLDHashOperator AppendFamily(nsStringHashKey::KeyType aKey,
                                        nsRefPtr<FontFamily>& aFamilyEntry,
                                        void *aUserArg)
    {
        FontFamilyListData *data = (FontFamilyListData*)aUserArg;
        data->mFamilyArray.AppendElement(aFamilyEntry);
        return PL_DHASH_NEXT;
    }

    nsTArray<nsRefPtr<FontFamily> >& mFamilyArray;
};

void
gfxWindowsPlatform::GetFontFamilyList(nsTArray<nsRefPtr<FontFamily> >& aFamilyArray)
{
    FontFamilyListData data(aFamilyArray);
    mFonts.Enumerate(FontFamilyListData::AppendFamily, &data);
}

static PRBool SimpleResolverCallback(const nsAString& aName, void* aClosure)
{
    nsString *result = static_cast<nsString*>(aClosure);
    result->Assign(aName);
    return PR_FALSE;
}

void
gfxWindowsPlatform::InitBadUnderlineList()
{

#ifndef MOZ_FT2_FONTS
    nsAutoTArray<nsString, 10> blacklist;
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
        ff->mIsBadUnderlineFontFamily = 1;
    }
#endif
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
        aAborted = !(*aCallback)(ff->Name(), aClosure);
        
        return NS_OK;
    }

    if (mNonExistingFonts.Contains(keyName)) {
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
        mNonExistingFonts.AppendElement(keyName);
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

PLDHashOperator
gfxWindowsPlatform::FindFontForCharProc(nsStringHashKey::KeyType aKey,
                                        nsRefPtr<FontFamily>& aFontFamily,
                                        void* userArg)
{
    FontSearch *data = (FontSearch*)userArg;

#ifdef MOZ_FT2_FONTS
    aFontFamily->FindFontForChar(data);
#else
    const PRUint32 ch = data->mCh;

    nsRefPtr<FontEntry> fe = aFontFamily->FindFontEntry(*data->mFontToMatch->GetStyle());
    NS_ASSERTION(fe, "couldn't find any font entry in family");
    if (!fe)
        return PL_DHASH_NEXT;

    
    
    
    
    PRInt32 rank = 1;

    
    
    if (fe->IsCrappyFont() || !fe->mCharacterMap.test(ch))
        return PL_DHASH_NEXT;

    
    
    if (fe->SupportsRange(gfxFontUtils::CharRangeBit(ch)))
        rank += 1;

    if (fe->SupportsLangGroup(data->mFontToMatch->GetStyle()->langGroup))
        rank += 2;

    FontEntry* mfe = static_cast<FontEntry*>(data->mFontToMatch->GetFontEntry());

    if (fe->mWindowsFamily == mfe->mWindowsFamily)
        rank += 3;
    if (fe->mWindowsPitch == mfe->mWindowsPitch)
        rank += 3;

    
    const PRBool italic = (data->mFontToMatch->GetStyle()->style != FONT_STYLE_NORMAL);
    if (fe->mItalic != italic)
        rank += 3;

    
    PRInt8 baseWeight, weightDistance;
    data->mFontToMatch->GetStyle()->ComputeWeightAndOffset(&baseWeight, &weightDistance);
    if (fe->mWeight == (baseWeight * 100) + (weightDistance * 100))
        rank += 2;
    else if (fe->mWeight == data->mFontToMatch->GetFontEntry()->mWeight)
        rank += 1;

    if (rank > data->mMatchRank ||
        (rank == data->mMatchRank && Compare(fe->Name(), data->mBestMatch->Name()) > 0)) {
        data->mBestMatch = fe;
        data->mMatchRank = rank;
    }
#endif

    return PL_DHASH_NEXT;
}

already_AddRefed<gfxFont>
gfxWindowsPlatform::FindFontForChar(PRUint32 aCh, gfxFont *aFont)
{
    
    if (mCodepointsWithNoFonts.test(aCh)) {
        return nsnull;
    }

    FontSearch data(aCh, aFont);

    
    mFonts.Enumerate(gfxWindowsPlatform::FindFontForCharProc, &data);

    if (data.mBestMatch) {
#ifdef MOZ_FT2_FONTS
        nsRefPtr<gfxFT2Font> font =
            gfxFT2Font::GetOrMakeFont(static_cast<FontEntry*>(data.mBestMatch.get()), 
                                      aFont->GetStyle()); 
        gfxFont* ret = font.forget().get();
        return already_AddRefed<gfxFont>(ret);
#else
        nsRefPtr<gfxWindowsFont> font =
            gfxWindowsFont::GetOrMakeFont(static_cast<FontEntry*>(data.mBestMatch.get()),
                                          aFont->GetStyle());
        if (font->IsValid()) {
            gfxFont* ret = font.forget().get();
            return already_AddRefed<gfxFont>(ret);
        }
#endif
        return nsnull;
    }
    
    mCodepointsWithNoFonts.set(aCh);
    return nsnull;
}

gfxFontGroup *
gfxWindowsPlatform::CreateFontGroup(const nsAString &aFamilies,
                                    const gfxFontStyle *aStyle,
                                    gfxUserFontSet *aUserFontSet)
{
#ifdef MOZ_FT2_FONTS
    return new gfxFT2FontGroup(aFamilies, aStyle);
#else
    return new gfxWindowsFontGroup(aFamilies, aStyle, aUserFontSet);
#endif
}


struct FullFontNameSearch {
    FullFontNameSearch(const nsAString& aFullName)
        : mFound(PR_FALSE), mFullName(aFullName), mDC(nsnull), mFontEntry(nsnull)
    { }

    PRPackedBool mFound;
    nsString     mFullName;
    nsString     mFamilyName;
    HDC          mDC;
    gfxFontEntry *mFontEntry;
};

#ifndef MOZ_FT2_FONTS



static int CALLBACK 
FindFullNameForFace(const ENUMLOGFONTEXW *lpelfe,
                    const NEWTEXTMETRICEXW *nmetrics,
                    DWORD fontType, LPARAM userArg)
{
    FullFontNameSearch *data = reinterpret_cast<FullFontNameSearch*>(userArg);

    
    if (!data->mFullName.Equals(nsDependentString(lpelfe->elfFullName)))
        return 1;  

    
    data->mFound = PR_TRUE;

    const NEWTEXTMETRICW& metrics = nmetrics->ntmTm;
    LOGFONTW logFont = lpelfe->elfLogFont;

    
    logFont.lfWeight = PR_MAX(PR_MIN(logFont.lfWeight, 900), 100);

    gfxWindowsFontType feType = FontEntry::DetermineFontType(metrics, fontType);

    data->mFontEntry = FontEntry::CreateFontEntry(data->mFamilyName, feType, (logFont.lfItalic == 0xFF), (PRUint16) (logFont.lfWeight), nsnull, data->mDC, &logFont);

    return 0;  
}
#endif




static PLDHashOperator
FindFullName(nsStringHashKey::KeyType aKey,
             nsRefPtr<FontFamily>& aFontFamily,
             void* userArg)
{
    FullFontNameSearch *data = reinterpret_cast<FullFontNameSearch*>(userArg);

    
    const nsString& family = aFontFamily->Name();
    
    nsString fullNameFamily;
    data->mFullName.Left(fullNameFamily, family.Length());

    
    if (family.Equals(fullNameFamily)) {
#ifdef MOZ_FT2_FONTS
        int len = aFontFamily->GetFontList().Length();
        int index = 0;
        for (; index < len && 
                 !aFontFamily->GetFontList()[index]->Name().Equals(data->mFullName); index++);
        if (index < len) {
            data->mFound = PR_TRUE;
            data->mFontEntry = aFontFamily->GetFontList()[index];
        }
#else
        HDC hdc;
        
        if (!data->mDC) {
            data->mDC= GetDC(nsnull);
            SetGraphicsMode(data->mDC, GM_ADVANCED);
        }
        hdc = data->mDC;

        LOGFONTW logFont;
        memset(&logFont, 0, sizeof(LOGFONTW));
        logFont.lfCharSet = DEFAULT_CHARSET;
        logFont.lfPitchAndFamily = 0;
        PRUint32 l = PR_MIN(family.Length(), LF_FACESIZE - 1);
        memcpy(logFont.lfFaceName,
               nsPromiseFlatString(family).get(),
               l * sizeof(PRUnichar));
        logFont.lfFaceName[l] = 0;
        data->mFamilyName.Assign(family);

        EnumFontFamiliesExW(hdc, &logFont, (FONTENUMPROCW)FindFullNameForFace, (LPARAM)data, 0);
#endif
    }

    if (data->mFound)
        return PL_DHASH_STOP;

    return PL_DHASH_NEXT;
}

gfxFontEntry* 
gfxWindowsPlatform::LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                    const nsAString& aFontName)
{
#ifdef MOZ_FT2_FONTS
    
    FullFontNameSearch data(aFontName);

    
    mFonts.Enumerate(FindFullName, &data);

    if (data.mDC)
        ReleaseDC(nsnull, data.mDC);
    
    return data.mFontEntry;
#else
    return FontEntry::LoadLocalFont(*aProxyEntry, aFontName);
#endif
}

gfxFontEntry* 
gfxWindowsPlatform::MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                     nsISupports *aLoader,
                                     const PRUint8 *aFontData, PRUint32 aLength)
{
#ifdef MOZ_FT2_FONTS
    return FontEntry::CreateFontEntry(*aProxyEntry, aLoader, aFontData, aLength);
#else
    return FontEntry::LoadFont(*aProxyEntry, aLoader, aFontData, aLength);
#endif    
}

PRBool
gfxWindowsPlatform::IsFontFormatSupported(nsIURI *aFontURI, PRUint32 aFormatFlags)
{
    
    NS_ASSERTION(!(aFormatFlags & gfxUserFontSet::FLAG_FORMAT_NOT_USED),
                 "strange font format hint set");

    
    if (aFormatFlags & (gfxUserFontSet::FLAG_FORMAT_OPENTYPE | 
                        gfxUserFontSet::FLAG_FORMAT_TRUETYPE)) {
        return PR_TRUE;
    }

    
    if (aFormatFlags != 0) {
        return PR_FALSE;
    }

    
    return PR_TRUE;
}

FontFamily *
gfxWindowsPlatform::FindFontFamily(const nsAString& aName)
{
    nsAutoString name(aName);
    BuildKeyNameFromFontName(name);

    nsRefPtr<FontFamily> ff;
    if (!mFonts.Get(name, &ff) &&
        !mFontSubstitutes.Get(name, &ff) &&
        !mFontAliases.Get(name, &ff)) {
        return nsnull;
    }
    return ff.get();
}

FontEntry *
gfxWindowsPlatform::FindFontEntry(const nsAString& aName, const gfxFontStyle& aFontStyle)
{
    nsRefPtr<FontFamily> ff = FindFontFamily(aName);
    if (!ff)
        return nsnull;

    return ff->FindFontEntry(aFontStyle);
}

qcms_profile*
gfxWindowsPlatform::GetPlatformCMSOutputProfile()
{
#ifndef MOZ_FT2_FONTS
    WCHAR str[1024+1];
    DWORD size = 1024;

    HDC dc = GetDC(nsnull);
    GetICMProfileW(dc, &size, (LPWSTR)&str);
    ReleaseDC(nsnull, dc);

    qcms_profile* profile =
        qcms_profile_from_path(NS_ConvertUTF16toUTF8(str).get());
#ifdef DEBUG_tor
    if (profile)
        fprintf(stderr,
                "ICM profile read from %s successfully\n",
                NS_ConvertUTF16toUTF8(str).get());
#endif
    return profile;
#else
    return nsnull;
#endif
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

void 
gfxWindowsPlatform::InitLoader()
{
    GetFontFamilyList(mFontFamilies);
    mStartIndex = 0;
    mNumFamilies = mFontFamilies.Length();
}

PRBool 
gfxWindowsPlatform::RunLoader()
{
    PRUint32 i, endIndex = ( mStartIndex + mIncrement < mNumFamilies ? mStartIndex + mIncrement : mNumFamilies );

#ifndef MOZ_FT2_FONTS
    
    for (i = mStartIndex; i < endIndex; i++) {
        
        mFontFamilies[i]->FindStyleVariations();
    }
#endif
    mStartIndex += mIncrement;
    if (mStartIndex < mNumFamilies)
        return PR_FALSE;
    return PR_TRUE;
}

void 
gfxWindowsPlatform::FinishLoader()
{
    mFontFamilies.Clear();
    mNumFamilies = 0;
}

#ifdef MOZ_FT2_FONTS
FT_Library
gfxWindowsPlatform::GetFTLibrary()
{
    return gPlatformFTLibrary;
}
#endif
