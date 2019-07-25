

































































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif
#include "prlog.h"

#include "gfxPlatformFontList.h"
#include "gfxTextRunWordCache.h"

#include "nsUnicharUtils.h"
#include "nsUnicodeRange.h"
#include "gfxUnicodeProperties.h"

#include "mozilla/Preferences.h"

using namespace mozilla;


static const PRUint32 kDelayBeforeLoadingCmaps = 8 * 1000; 
static const PRUint32 kIntervalBetweenLoadingCmaps = 150; 
static const PRUint32 kNumFontsPerSlice = 10; 

#ifdef PR_LOGGING

#define LOG_FONTLIST(args) PR_LOG(gfxPlatform::GetLog(eGfxLog_fontlist), \
                               PR_LOG_DEBUG, args)
#define LOG_FONTLIST_ENABLED() PR_LOG_TEST( \
                                   gfxPlatform::GetLog(eGfxLog_fontlist), \
                                   PR_LOG_DEBUG)

#endif 

gfxPlatformFontList *gfxPlatformFontList::sPlatformFontList = nsnull;


static const char* kObservedPrefs[] = {
    "font.",
    "font.name-list.",
    "intl.accept_languages",  
    nsnull
};

class gfxFontListPrefObserver : public nsIObserver {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
};

static gfxFontListPrefObserver* gFontListPrefObserver = nsnull;

NS_IMPL_ISUPPORTS1(gfxFontListPrefObserver, nsIObserver)

NS_IMETHODIMP
gfxFontListPrefObserver::Observe(nsISupports     *aSubject,
                                 const char      *aTopic,
                                 const PRUnichar *aData)
{
    NS_ASSERTION(!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID), "invalid topic");
    
    
    gfxPlatformFontList::PlatformFontList()->ClearPrefFonts();
    gfxFontCache::GetCache()->AgeAllGenerations();
    return NS_OK;
}


gfxPlatformFontList::gfxPlatformFontList(PRBool aNeedFullnamePostscriptNames)
    : mNeedFullnamePostscriptNames(aNeedFullnamePostscriptNames),
      mStartIndex(0), mIncrement(kNumFontsPerSlice), mNumFamilies(0)
{
    mFontFamilies.Init(100);
    mOtherFamilyNames.Init(30);
    mOtherFamilyNamesInitialized = PR_FALSE;

    if (mNeedFullnamePostscriptNames) {
        mFullnames.Init(100);
        mPostscriptNames.Init(100);
    }
    mFaceNamesInitialized = PR_FALSE;

    mPrefFonts.Init(10);

    mBadUnderlineFamilyNames.Init(10);
    LoadBadUnderlineList();

    
    NS_ASSERTION(!gFontListPrefObserver,
                 "There has been font list pref observer already");
    gFontListPrefObserver = new gfxFontListPrefObserver();
    NS_ADDREF(gFontListPrefObserver);
    Preferences::AddStrongObservers(gFontListPrefObserver, kObservedPrefs);
}

gfxPlatformFontList::~gfxPlatformFontList()
{
    NS_ASSERTION(gFontListPrefObserver, "There is no font list pref observer");
    Preferences::RemoveObservers(gFontListPrefObserver, kObservedPrefs);
    NS_RELEASE(gFontListPrefObserver);
}

nsresult
gfxPlatformFontList::InitFontList()
{
    mFontFamilies.Clear();
    mOtherFamilyNames.Clear();
    mOtherFamilyNamesInitialized = PR_FALSE;
    if (mNeedFullnamePostscriptNames) {
        mFullnames.Clear();
        mPostscriptNames.Clear();
    }
    mFaceNamesInitialized = PR_FALSE;
    mPrefFonts.Clear();
    CancelLoader();

    
    mCodepointsWithNoFonts.reset();
    mCodepointsWithNoFonts.SetRange(0,0x1f);     
    mCodepointsWithNoFonts.SetRange(0x7f,0x9f);  

    sPlatformFontList = this;

    return NS_OK;
}

void
gfxPlatformFontList::GenerateFontListKey(const nsAString& aKeyName, nsAString& aResult)
{
    aResult = aKeyName;
    ToLowerCase(aResult);
}

void 
gfxPlatformFontList::InitOtherFamilyNames()
{
    mOtherFamilyNamesInitialized = PR_TRUE;

    
    mFontFamilies.Enumerate(gfxPlatformFontList::InitOtherFamilyNamesProc, this);
}
                                                         
PLDHashOperator PR_CALLBACK
gfxPlatformFontList::InitOtherFamilyNamesProc(nsStringHashKey::KeyType aKey,
                                              nsRefPtr<gfxFontFamily>& aFamilyEntry,
                                              void* userArg)
{
    gfxPlatformFontList *fc = static_cast<gfxPlatformFontList*>(userArg);
    aFamilyEntry->ReadOtherFamilyNames(fc);
    return PL_DHASH_NEXT;
}

void
gfxPlatformFontList::InitFaceNameLists()
{
    mFaceNamesInitialized = PR_TRUE;

    
    mFontFamilies.Enumerate(gfxPlatformFontList::InitFaceNameListsProc, this);
}

PLDHashOperator PR_CALLBACK
gfxPlatformFontList::InitFaceNameListsProc(nsStringHashKey::KeyType aKey,
                                           nsRefPtr<gfxFontFamily>& aFamilyEntry,
                                           void* userArg)
{
    gfxPlatformFontList *fc = static_cast<gfxPlatformFontList*>(userArg);
    aFamilyEntry->ReadFaceNames(fc, fc->NeedFullnamePostscriptNames());
    return PL_DHASH_NEXT;
}

void
gfxPlatformFontList::PreloadNamesList()
{
    nsAutoTArray<nsString, 10> preloadFonts;
    gfxFontUtils::GetPrefsFontList("font.preload-names-list", preloadFonts);

    PRUint32 numFonts = preloadFonts.Length();
    for (PRUint32 i = 0; i < numFonts; i++) {
        PRBool found;
        nsAutoString key;
        GenerateFontListKey(preloadFonts[i], key);
        
        
        gfxFontFamily *familyEntry = mFontFamilies.GetWeak(key, &found);
        if (familyEntry) {
            familyEntry->ReadOtherFamilyNames(this);
        }
    }

}

void 
gfxPlatformFontList::SetFixedPitch(const nsAString& aFamilyName)
{
    gfxFontFamily *family = FindFamily(aFamilyName);
    if (!family) return;

    family->FindStyleVariations();
    nsTArray<nsRefPtr<gfxFontEntry> >& fontlist = family->GetFontList();

    PRUint32 i, numFonts = fontlist.Length();

    for (i = 0; i < numFonts; i++) {
        fontlist[i]->mFixedPitch = 1;
    }
}

void
gfxPlatformFontList::LoadBadUnderlineList()
{
    nsAutoTArray<nsString, 10> blacklist;
    gfxFontUtils::GetPrefsFontList("font.blacklist.underline_offset", blacklist);
    PRUint32 numFonts = blacklist.Length();
    for (PRUint32 i = 0; i < numFonts; i++) {
        nsAutoString key;
        GenerateFontListKey(blacklist[i], key);
        mBadUnderlineFamilyNames.Put(key);
    }
}

PRBool 
gfxPlatformFontList::ResolveFontName(const nsAString& aFontName, nsAString& aResolvedFontName)
{
    gfxFontFamily *family = FindFamily(aFontName);
    if (family) {
        aResolvedFontName = family->Name();
        return PR_TRUE;
    }
    return PR_FALSE;
}

struct FontListData {
    FontListData(nsIAtom *aLangGroup,
                 const nsACString& aGenericFamily,
                 nsTArray<nsString>& aListOfFonts) :
        mLangGroup(aLangGroup), mGenericFamily(aGenericFamily),
        mListOfFonts(aListOfFonts) {}
    nsIAtom *mLangGroup;
    const nsACString& mGenericFamily;
    nsTArray<nsString>& mListOfFonts;
};

PLDHashOperator PR_CALLBACK
gfxPlatformFontList::HashEnumFuncForFamilies(nsStringHashKey::KeyType aKey,
                                             nsRefPtr<gfxFontFamily>& aFamilyEntry,
                                             void *aUserArg)
{
    FontListData *data = static_cast<FontListData*>(aUserArg);

    
    
    
    gfxFontStyle style;
    style.language = data->mLangGroup;
    PRBool needsBold;
    nsRefPtr<gfxFontEntry> aFontEntry = aFamilyEntry->FindFontForStyle(style, needsBold);
    NS_ASSERTION(aFontEntry, "couldn't find any font entry in family");
    if (!aFontEntry)
        return PL_DHASH_NEXT;

    
    if (aFontEntry->IsSymbolFont())
        return PL_DHASH_NEXT;

    if (aFontEntry->SupportsLangGroup(data->mLangGroup) &&
        aFontEntry->MatchesGenericFamily(data->mGenericFamily)) {
        nsAutoString localizedFamilyName;
        aFamilyEntry->LocalizedName(localizedFamilyName);
        data->mListOfFonts.AppendElement(localizedFamilyName);
    }

    return PL_DHASH_NEXT;
}

void
gfxPlatformFontList::GetFontList(nsIAtom *aLangGroup,
                                 const nsACString& aGenericFamily,
                                 nsTArray<nsString>& aListOfFonts)
{
    FontListData data(aLangGroup, aGenericFamily, aListOfFonts);

    mFontFamilies.Enumerate(gfxPlatformFontList::HashEnumFuncForFamilies, &data);

    aListOfFonts.Sort();
    aListOfFonts.Compact();
}

struct FontFamilyListData {
    FontFamilyListData(nsTArray<nsRefPtr<gfxFontFamily> >& aFamilyArray) 
        : mFamilyArray(aFamilyArray)
    {}

    static PLDHashOperator PR_CALLBACK AppendFamily(nsStringHashKey::KeyType aKey,
                                                    nsRefPtr<gfxFontFamily>& aFamilyEntry,
                                                    void *aUserArg)
    {
        FontFamilyListData *data = static_cast<FontFamilyListData*>(aUserArg);
        data->mFamilyArray.AppendElement(aFamilyEntry);
        return PL_DHASH_NEXT;
    }

    nsTArray<nsRefPtr<gfxFontFamily> >& mFamilyArray;
};

void
gfxPlatformFontList::GetFontFamilyList(nsTArray<nsRefPtr<gfxFontFamily> >& aFamilyArray)
{
    FontFamilyListData data(aFamilyArray);
    mFontFamilies.Enumerate(FontFamilyListData::AppendFamily, &data);
}

gfxFontEntry*  
gfxPlatformFontList::FindFontForChar(const PRUint32 aCh, gfxFont *aPrevFont)
{
    
    if (mCodepointsWithNoFonts.test(aCh)) {
        return nsnull;
    }

    
    

    
    
    
    
    if (aCh == 0xFFFD && mReplacementCharFallbackFamily.Length() > 0) {
        gfxFontEntry* fontEntry = nsnull;
        PRBool needsBold;  

        if (aPrevFont) {
            fontEntry = FindFontForFamily(mReplacementCharFallbackFamily, aPrevFont->GetStyle(), needsBold);
        } else {
            gfxFontStyle normalStyle;
            fontEntry = FindFontForFamily(mReplacementCharFallbackFamily, &normalStyle, needsBold);
        }

        if (fontEntry && fontEntry->TestCharacterMap(aCh))
            return fontEntry;
    }

    FontSearch data(aCh, aPrevFont);

    
    mFontFamilies.Enumerate(gfxPlatformFontList::FindFontForCharProc, &data);

#ifdef PR_LOGGING
    PRLogModuleInfo *log = gfxPlatform::GetLog(eGfxLog_textrun);

    if (NS_UNLIKELY(log)) {
        PRUint32 charRange = gfxFontUtils::CharRangeBit(aCh);
        PRUint32 unicodeRange = FindCharUnicodeRange(aCh);
        PRUint32 hbscript = gfxUnicodeProperties::GetScriptCode(aCh);
        PR_LOG(log, PR_LOG_DEBUG,\
               ("(textrun-systemfallback) char: u+%6.6x "
                "char-range: %d unicode-range: %d script: %d match: [%s] count: %d\n",
                aCh,
                charRange, unicodeRange, hbscript,
                (data.mBestMatch ?
                 NS_ConvertUTF16toUTF8(data.mBestMatch->Name()).get() :
                 "<none>"),
                data.mCount));
    }
#endif

    
    if (!data.mBestMatch) {
        mCodepointsWithNoFonts.set(aCh);
    } else if (aCh == 0xFFFD) {
        mReplacementCharFallbackFamily = data.mBestMatch->FamilyName();
    }

    return data.mBestMatch;
}

PLDHashOperator PR_CALLBACK 
gfxPlatformFontList::FindFontForCharProc(nsStringHashKey::KeyType aKey, nsRefPtr<gfxFontFamily>& aFamilyEntry,
     void *userArg)
{
    FontSearch *data = static_cast<FontSearch*>(userArg);

    
    aFamilyEntry->FindFontForChar(data);
    return PL_DHASH_NEXT;
}

#ifdef XP_WIN
#include <windows.h>


static void LogRegistryEvent(const wchar_t *msg)
{
  HKEY dummyKey;
  HRESULT hr;
  wchar_t buf[512];

  wsprintfW(buf, L" log %s", msg);
  hr = RegOpenKeyExW(HKEY_LOCAL_MACHINE, buf, 0, KEY_READ, &dummyKey);
  if (SUCCEEDED(hr)) {
    RegCloseKey(dummyKey);
  }
}
#endif

gfxFontFamily* 
gfxPlatformFontList::FindFamily(const nsAString& aFamily)
{
    nsAutoString key;
    gfxFontFamily *familyEntry;
    PRBool found;
    GenerateFontListKey(aFamily, key);

    NS_ASSERTION(mFontFamilies.Count() != 0, "system font list was not initialized correctly");

    
    if ((familyEntry = mFontFamilies.GetWeak(key, &found))) {
        return familyEntry;
    }

    
    if ((familyEntry = mOtherFamilyNames.GetWeak(key, &found)) != nsnull) {
        return familyEntry;
    }

    
    
    
    
    
    if (!mOtherFamilyNamesInitialized && !IsASCII(aFamily)) {
        InitOtherFamilyNames();
        if ((familyEntry = mOtherFamilyNames.GetWeak(key, &found)) != nsnull) {
            return familyEntry;
        }
    }

    return nsnull;
}

gfxFontEntry*
gfxPlatformFontList::FindFontForFamily(const nsAString& aFamily, const gfxFontStyle* aStyle, PRBool& aNeedsBold)
{
    gfxFontFamily *familyEntry = FindFamily(aFamily);

    aNeedsBold = PR_FALSE;

    if (familyEntry)
        return familyEntry->FindFontForStyle(*aStyle, aNeedsBold);

    return nsnull;
}

PRBool
gfxPlatformFontList::GetPrefFontFamilyEntries(eFontPrefLang aLangGroup, nsTArray<nsRefPtr<gfxFontFamily> > *array)
{
    return mPrefFonts.Get(PRUint32(aLangGroup), array);
}

void
gfxPlatformFontList::SetPrefFontFamilyEntries(eFontPrefLang aLangGroup, nsTArray<nsRefPtr<gfxFontFamily> >& array)
{
    mPrefFonts.Put(PRUint32(aLangGroup), array);
}

void 
gfxPlatformFontList::AddOtherFamilyName(gfxFontFamily *aFamilyEntry, nsAString& aOtherFamilyName)
{
    nsAutoString key;
    PRBool found;
    GenerateFontListKey(aOtherFamilyName, key);

    if (!mOtherFamilyNames.GetWeak(key, &found)) {
        mOtherFamilyNames.Put(key, aFamilyEntry);
#ifdef PR_LOGGING
        LOG_FONTLIST(("(fontlist-otherfamily) canonical family: %s, "
                      "other family: %s\n",
                      NS_ConvertUTF16toUTF8(aFamilyEntry->Name()).get(),
                      NS_ConvertUTF16toUTF8(aOtherFamilyName).get()));
#endif
        if (mBadUnderlineFamilyNames.Contains(key))
            aFamilyEntry->SetBadUnderlineFamily();
    }
}

void
gfxPlatformFontList::AddFullname(gfxFontEntry *aFontEntry, nsAString& aFullname)
{
    PRBool found;

    if (!mFullnames.GetWeak(aFullname, &found)) {
        mFullnames.Put(aFullname, aFontEntry);
#ifdef PR_LOGGING
        LOG_FONTLIST(("(fontlist-fullname) name: %s, fullname: %s\n",
                      NS_ConvertUTF16toUTF8(aFontEntry->Name()).get(),
                      NS_ConvertUTF16toUTF8(aFullname).get()));
#endif
    }
}

void
gfxPlatformFontList::AddPostscriptName(gfxFontEntry *aFontEntry, nsAString& aPostscriptName)
{
    PRBool found;

    if (!mPostscriptNames.GetWeak(aPostscriptName, &found)) {
        mPostscriptNames.Put(aPostscriptName, aFontEntry);
#ifdef PR_LOGGING
        LOG_FONTLIST(("(fontlist-postscript) name: %s, psname: %s\n",
                      NS_ConvertUTF16toUTF8(aFontEntry->Name()).get(),
                      NS_ConvertUTF16toUTF8(aPostscriptName).get()));
#endif
    }
}

PRBool
gfxPlatformFontList::GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName)
{
    aFamilyName.Truncate();
    ResolveFontName(aFontName, aFamilyName);
    return !aFamilyName.IsEmpty();
}

void 
gfxPlatformFontList::InitLoader()
{
    GetFontFamilyList(mFontFamiliesToLoad);
    mStartIndex = 0;
    mNumFamilies = mFontFamiliesToLoad.Length();
}

PRBool 
gfxPlatformFontList::RunLoader()
{
    PRUint32 i, endIndex = (mStartIndex + mIncrement < mNumFamilies ? mStartIndex + mIncrement : mNumFamilies);

    
    for (i = mStartIndex; i < endIndex; i++) {
        gfxFontFamily* familyEntry = mFontFamiliesToLoad[i];

        
        familyEntry->FindStyleVariations();
        if (familyEntry->GetFontList().Length() == 0) {
            
            nsAutoString key;
            GenerateFontListKey(familyEntry->Name(), key);
            mFontFamilies.Remove(key);
            continue;
        }

        
        familyEntry->ReadCMAP();

        
        familyEntry->ReadFaceNames(this, mNeedFullnamePostscriptNames);

        
        familyEntry->CheckForSimpleFamily();
    }

    mStartIndex = endIndex;

    return (mStartIndex >= mNumFamilies);
}

void 
gfxPlatformFontList::FinishLoader()
{
    mFontFamiliesToLoad.Clear();
    mNumFamilies = 0;
}
