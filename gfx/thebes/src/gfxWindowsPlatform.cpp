







































#include "gfxWindowsPlatform.h"

#include "gfxImageSurface.h"
#include "gfxWindowsSurface.h"

#include "nsUnicharUtils.h"

#include "nsIPref.h"
#include "nsServiceManagerUtils.h"

#include "nsIWindowsRegKey.h"
#include "nsILocalFile.h"
#include "plbase64.h"

#include "gfxWindowsFonts.h"
#include "gfxUserFontSet.h"

#include <string>

#include "lcms.h"

static void InitializeFontEmbeddingProcs();


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

int
gfxWindowsPlatform::PrefChangedCallback(const char *aPrefName, void *closure)
{
    
    
    gfxWindowsPlatform *plat = static_cast<gfxWindowsPlatform *>(closure);
    plat->mPrefFonts.Clear();
    return 0;
}

gfxWindowsPlatform::gfxWindowsPlatform()
    : mStartIndex(0), mIncrement(kNumFontsPerSlice), mNumFamilies(0)
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
    

    InitializeFontEmbeddingProcs();
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
    FontListData(const nsACString& aLangGroup, const nsACString& aGenericFamily, nsStringArray& aListOfFonts) :
        mLangGroup(aLangGroup), mGenericFamily(aGenericFamily), mStringArray(aListOfFonts) {}
    const nsACString& mLangGroup;
    const nsACString& mGenericFamily;
    nsStringArray& mStringArray;
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
    
    LOGFONTW logFont;
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfFaceName[0] = 0;
    logFont.lfPitchAndFamily = 0;

    
    HDC dc = ::GetDC(nsnull);
    EnumFontFamiliesExW(dc, &logFont, (FONTENUMPROCW)gfxWindowsPlatform::FontEnumProc, (LPARAM)&mFonts, 0);
    ::ReleaseDC(nsnull, dc);

    
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
            mNonExistingFonts.AppendString(substituteName);
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
        ch(aCh), fontToMatch(aFont), matchRank(-1) {
    }
    PRUint32 ch;
    nsRefPtr<gfxWindowsFont> fontToMatch;
    PRInt32 matchRank;
    nsRefPtr<FontEntry> bestMatch;
};

PLDHashOperator
gfxWindowsPlatform::FindFontForCharProc(nsStringHashKey::KeyType aKey,
                                        nsRefPtr<FontFamily>& aFontFamily,
                                        void* userArg)
{
    FontSearch *data = (FontSearch*)userArg;

    const PRUint32 ch = data->ch;

    nsRefPtr<FontEntry> fe = aFontFamily->FindFontEntry(*data->fontToMatch->GetStyle());

    
    
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

    
    const PRBool italic = (data->fontToMatch->GetStyle()->style != FONT_STYLE_NORMAL);
    if (fe->mItalic != italic)
        rank += 3;

    
    PRInt8 baseWeight, weightDistance;
    data->fontToMatch->GetStyle()->ComputeWeightAndOffset(&baseWeight, &weightDistance);
    if (fe->mWeight == (baseWeight * 100) + (weightDistance * 100))
        rank += 2;
    else if (fe->mWeight == data->fontToMatch->GetFontEntry()->mWeight)
        rank += 1;

    if (rank > data->matchRank ||
        (rank == data->matchRank && Compare(fe->Name(), data->bestMatch->Name()) > 0)) {
        data->bestMatch = fe;
        data->matchRank = rank;
    }

    return PL_DHASH_NEXT;
}

already_AddRefed<gfxWindowsFont>
gfxWindowsPlatform::FindFontForChar(PRUint32 aCh, gfxWindowsFont *aFont)
{
    
    if (mCodepointsWithNoFonts.test(aCh)) {
        return nsnull;
    }

    FontSearch data(aCh, aFont);

    
    mFonts.Enumerate(gfxWindowsPlatform::FindFontForCharProc, &data);

    if (data.bestMatch) {
        nsRefPtr<gfxWindowsFont> font =
            gfxWindowsFont::GetOrMakeFont(data.bestMatch, aFont->GetStyle());
        if (font->IsValid())
            return font.forget();
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
    return new gfxWindowsFontGroup(aFamilies, aStyle, aUserFontSet);
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
    }

    if (data->mFound)
        return PL_DHASH_STOP;

    return PL_DHASH_NEXT;
}

gfxFontEntry* 
gfxWindowsPlatform::LookupLocalFont(const nsAString& aFontName)
{
    
    FullFontNameSearch data(aFontName);

    
    mFonts.Enumerate(FindFullName, &data);

    if (data.mDC)
        ReleaseDC(nsnull, data.mDC);
    
    return data.mFontEntry;
}


static void MakeUniqueFontName(PRUnichar aName[LF_FACESIZE])
{
    static PRUint32 fontCount = 0;
    ++fontCount;

    char buf[LF_FACESIZE];

    sprintf(buf, "mozfont%8.8x%8.8x", ::GetTickCount(), fontCount);  

    nsCAutoString fontName(buf);

    PRUint32 nameLen = PR_MIN(fontName.Length(), LF_FACESIZE - 1);
    memcpy(aName, nsPromiseFlatString(NS_ConvertUTF8toUTF16(fontName)).get(), nameLen * 2);
    aName[nameLen] = 0;
}



#ifndef __t2embapi__

#define TTLOAD_PRIVATE                  0x00000001
#define LICENSE_PREVIEWPRINT            0x0004
#define E_NONE                          0x0000L

typedef unsigned long( WINAPIV *READEMBEDPROC ) ( void*, void*, const unsigned long );

typedef struct
{
    unsigned short usStructSize;    
    unsigned short usRefStrSize;    
    unsigned short *pusRefStr;      
}TTLOADINFO;

LONG WINAPI TTLoadEmbeddedFont
(
    HANDLE*  phFontReference,           
                                        
    ULONG    ulFlags,                   
    ULONG*   pulPrivStatus,             
    ULONG    ulPrivs,                   
    ULONG*   pulStatus,                 
    READEMBEDPROC lpfnReadFromStream,   
    LPVOID   lpvReadStream,             
    LPWSTR   szWinFamilyName,           
    LPSTR    szMacFamilyName,           
    TTLOADINFO* pTTLoadInfo             
);

#endif 

typedef LONG( WINAPI *TTLoadEmbeddedFontProc ) (HANDLE* phFontReference, ULONG ulFlags, ULONG* pulPrivStatus, ULONG ulPrivs, ULONG* pulStatus, 
                                             READEMBEDPROC lpfnReadFromStream, LPVOID lpvReadStream, LPWSTR szWinFamilyName, 
                                             LPSTR szMacFamilyName, TTLOADINFO* pTTLoadInfo);

typedef LONG( WINAPI *TTDeleteEmbeddedFontProc ) (HANDLE hFontReference, ULONG ulFlags, ULONG* pulStatus);


static TTLoadEmbeddedFontProc TTLoadEmbeddedFontPtr = nsnull;
static TTDeleteEmbeddedFontProc TTDeleteEmbeddedFontPtr = nsnull;

static void InitializeFontEmbeddingProcs()
{
    HMODULE fontlib = LoadLibraryW(L"t2embed.dll");
    if (!fontlib)
        return;
    TTLoadEmbeddedFontPtr = (TTLoadEmbeddedFontProc) GetProcAddress(fontlib, "TTLoadEmbeddedFont");
    TTDeleteEmbeddedFontPtr = (TTDeleteEmbeddedFontProc) GetProcAddress(fontlib, "TTDeleteEmbeddedFont");
}

class WinUserFontData : public gfxUserFontData {
public:
    WinUserFontData(HANDLE aFontRef)
        : mFontRef(aFontRef)
    { }

    virtual ~WinUserFontData()
    {
        ULONG pulStatus;
        TTDeleteEmbeddedFontPtr(mFontRef, 0, &pulStatus);
    }
    
    HANDLE mFontRef;
};



class EOTFontStreamReader {
public:
    EOTFontStreamReader(nsILocalFile *aFontFile, PRUint8 *aEOTHeader, 
                        PRUint32 aEOTHeaderLen)
        : mFontFile(aFontFile), mFd(nsnull), mOpenError(PR_FALSE), 
          mInHeader(PR_TRUE), mHeaderOffset(0), mEOTHeader(aEOTHeader), 
          mEOTHeaderLen(aEOTHeaderLen)
    {
    
    }

    ~EOTFontStreamReader() 
    { 
        if (mFd) {
            PR_Close(mFd);
        }

        mFontFile->Remove(PR_FALSE);
    }

    nsCOMPtr<nsILocalFile>  mFontFile;
    PRFileDesc              *mFd;
    PRPackedBool            mOpenError;
    PRPackedBool            mInHeader;
    PRUint32                mHeaderOffset;
    PRUint8                 *mEOTHeader;
    PRUint32                mEOTHeaderLen;

    PRBool OpenFontFile()
    {
        nsresult rv;

        rv = mFontFile->OpenNSPRFileDesc(PR_RDONLY, 0, &mFd);
        if (NS_FAILED(rv) || !mFd)
            return PR_FALSE;

        return PR_TRUE;
    }

    unsigned long Read(void *outBuffer, const unsigned long aBytesToRead)
    {
        PRUint32 bytesLeft = aBytesToRead;
        PRUint8 *out = static_cast<PRUint8*> (outBuffer);

        if (mOpenError)
            return 0;

        if (!mFd) {
            if (!OpenFontFile()) {
                mOpenError = PR_TRUE;
                return 0;
            }
        }

        
        if (mInHeader) {
            PRUint32 toCopy = PR_MIN(aBytesToRead, mEOTHeaderLen - mHeaderOffset);
            memcpy(out, mEOTHeader + mHeaderOffset, toCopy);
            bytesLeft -= toCopy;
            mHeaderOffset += toCopy;
            out += toCopy;
            if (mHeaderOffset == mEOTHeaderLen)
                mInHeader = PR_FALSE;
        }

        if (bytesLeft) {
            PRInt32 bytesRead = PR_Read(mFd, out, bytesLeft);
            if (bytesRead > 0)
                bytesLeft -= bytesRead;
        }

        return aBytesToRead - bytesLeft;
    }

    static unsigned long ReadEOTStream(void *aReadStream, void *outBuffer, 
                                       const unsigned long aBytesToRead) 
    {
        EOTFontStreamReader *eotReader = 
                               static_cast<EOTFontStreamReader*> (aReadStream);
        return eotReader->Read(outBuffer, aBytesToRead);
    }        
        
};

gfxFontEntry* 
gfxWindowsPlatform::MakePlatformFont(const gfxFontEntry *aProxyEntry, 
                                     const gfxDownloadedFontData* aFontData)
{
    
    if (!TTLoadEmbeddedFontPtr || !TTDeleteEmbeddedFontPtr)
        return nsnull;

    
    nsAutoTArray<PRUint8,2048> eotHeader;
    PRUint8 *buffer;
    PRUint32 eotlen;
    PRUnichar fontName[LF_FACESIZE];
    PRBool isCFF;
    
    nsresult rv;
    HANDLE fontRef;
    PRInt32 ret;

    {
        nsCOMPtr<nsILocalFile> fontFile(do_QueryInterface(aFontData->mFontFile, &rv));
        if (NS_FAILED(rv))
            return nsnull;

        rv = gfxFontUtils::MakeEOTHeader(fontFile, &eotHeader, &isCFF);
        if (NS_FAILED(rv))
            return nsnull;

        
        eotlen = eotHeader.Length();
        buffer = reinterpret_cast<PRUint8*> (eotHeader.Elements());
        
        ULONG privStatus, pulStatus;
        MakeUniqueFontName(fontName);
        EOTFontStreamReader eotReader(fontFile, buffer, eotlen);

        ret = TTLoadEmbeddedFontPtr(&fontRef, TTLOAD_PRIVATE, &privStatus, 
                                   LICENSE_PREVIEWPRINT, &pulStatus, 
                                   EOTFontStreamReader::ReadEOTStream, 
                                   &eotReader, fontName, 0, 0);
    }

    if (ret != E_NONE)
        return nsnull;
    
    
    WinUserFontData *winUserFontData = new WinUserFontData(fontRef);
    PRUint16 w = (aProxyEntry->mWeight == 0 ? 400 : aProxyEntry->mWeight);

    return FontEntry::CreateFontEntry(nsDependentString(fontName), 
        gfxWindowsFontType(isCFF ? GFX_FONT_TYPE_PS_OPENTYPE : GFX_FONT_TYPE_TRUETYPE) , 
        PRUint32(aProxyEntry->mItalic ? FONT_STYLE_ITALIC : FONT_STYLE_NORMAL), 
        w, winUserFontData);
}

PRBool
gfxWindowsPlatform::IsFontFormatSupported(nsIURI *aFontURI, PRUint32 aFormatFlags)
{
    
    if (aFormatFlags & (gfxUserFontSet::FLAG_FORMAT_EOT | gfxUserFontSet::FLAG_FORMAT_SVG)) {
        return PR_FALSE;
    }

    
    
    
    if ((aFormatFlags & gfxUserFontSet::FLAG_FORMAT_TRUETYPE_AAT) 
         && !(aFormatFlags & (gfxUserFontSet::FLAG_FORMAT_OPENTYPE | gfxUserFontSet::FLAG_FORMAT_TRUETYPE))) {
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

    
    for (i = mStartIndex; i < endIndex; i++) {
        
        mFontFamilies[i]->FindStyleVariations();
    }

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

