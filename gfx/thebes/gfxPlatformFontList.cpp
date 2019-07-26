




#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif
#include "prlog.h"

#include "gfxPlatformFontList.h"

#include "nsUnicharUtils.h"
#include "nsUnicodeRange.h"
#include "nsUnicodeProperties.h"

#include "mozilla/Attributes.h"
#include "mozilla/Likely.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/gfx/2D.h"

using namespace mozilla;

#ifdef PR_LOGGING

#define LOG_FONTLIST(args) PR_LOG(gfxPlatform::GetLog(eGfxLog_fontlist), \
                               PR_LOG_DEBUG, args)
#define LOG_FONTLIST_ENABLED() PR_LOG_TEST( \
                                   gfxPlatform::GetLog(eGfxLog_fontlist), \
                                   PR_LOG_DEBUG)

#endif 

gfxPlatformFontList *gfxPlatformFontList::sPlatformFontList = nullptr;


#define FONT_LOADER_FAMILIES_PER_SLICE_PREF "gfx.font_loader.families_per_slice"
#define FONT_LOADER_DELAY_PREF              "gfx.font_loader.delay"
#define FONT_LOADER_INTERVAL_PREF           "gfx.font_loader.interval"

static const char* kObservedPrefs[] = {
    "font.",
    "font.name-list.",
    "intl.accept_languages",  
    nullptr
};

class gfxFontListPrefObserver MOZ_FINAL : public nsIObserver {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
};

static gfxFontListPrefObserver* gFontListPrefObserver = nullptr;

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

MOZ_DEFINE_MALLOC_SIZE_OF(FontListMallocSizeOf)

gfxPlatformFontList::MemoryReporter::MemoryReporter()
{}

NS_IMETHODIMP
gfxPlatformFontList::MemoryReporter::CollectReports
    (nsIMemoryReporterCallback* aCb,
     nsISupports* aClosure)
{
    FontListSizes sizes;
    sizes.mFontListSize = 0;
    sizes.mFontTableCacheSize = 0;
    sizes.mCharMapsSize = 0;

    gfxPlatformFontList::PlatformFontList()->AddSizeOfIncludingThis(&FontListMallocSizeOf,
                                                                    &sizes);

    aCb->Callback(EmptyCString(),
                  NS_LITERAL_CSTRING("explicit/gfx/font-list"),
                  nsIMemoryReporter::KIND_HEAP, nsIMemoryReporter::UNITS_BYTES,
                  sizes.mFontListSize,
                  NS_LITERAL_CSTRING("Memory used to manage the list of font families and faces."),
                  aClosure);

    aCb->Callback(EmptyCString(),
                  NS_LITERAL_CSTRING("explicit/gfx/font-charmaps"),
                  nsIMemoryReporter::KIND_HEAP, nsIMemoryReporter::UNITS_BYTES,
                  sizes.mCharMapsSize,
                  NS_LITERAL_CSTRING("Memory used to record the character coverage of individual fonts."),
                  aClosure);

    if (sizes.mFontTableCacheSize) {
        aCb->Callback(EmptyCString(),
                      NS_LITERAL_CSTRING("explicit/gfx/font-tables"),
                      nsIMemoryReporter::KIND_HEAP, nsIMemoryReporter::UNITS_BYTES,
                      sizes.mFontTableCacheSize,
                      NS_LITERAL_CSTRING("Memory used for cached font metrics and layout tables."),
                      aClosure);
    }

    return NS_OK;
}

gfxPlatformFontList::gfxPlatformFontList(bool aNeedFullnamePostscriptNames)
    : mFontFamilies(100), mOtherFamilyNames(30),
      mPrefFonts(10), mBadUnderlineFamilyNames(10), mSharedCmaps(16),
      mStartIndex(0), mIncrement(1), mNumFamilies(0)
{
    mOtherFamilyNamesInitialized = false;

    if (aNeedFullnamePostscriptNames) {
        mExtraNames = new ExtraNames();
    }
    mFaceNamesInitialized = false;

    LoadBadUnderlineList();

    
    NS_ASSERTION(!gFontListPrefObserver,
                 "There has been font list pref observer already");
    gFontListPrefObserver = new gfxFontListPrefObserver();
    NS_ADDREF(gFontListPrefObserver);
    Preferences::AddStrongObservers(gFontListPrefObserver, kObservedPrefs);

    RegisterStrongMemoryReporter(new MemoryReporter());
}

gfxPlatformFontList::~gfxPlatformFontList()
{
    mSharedCmaps.Clear();
    NS_ASSERTION(gFontListPrefObserver, "There is no font list pref observer");
    Preferences::RemoveObservers(gFontListPrefObserver, kObservedPrefs);
    NS_RELEASE(gFontListPrefObserver);
}

nsresult
gfxPlatformFontList::InitFontList()
{
    mFontFamilies.Clear();
    mOtherFamilyNames.Clear();
    mOtherFamilyNamesInitialized = false;
    if (mExtraNames) {
        mExtraNames->mFullnames.Clear();
        mExtraNames->mPostscriptNames.Clear();
    }
    mFaceNamesInitialized = false;
    mPrefFonts.Clear();
    mReplacementCharFallbackFamily = nullptr;
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
    mOtherFamilyNamesInitialized = true;

    Telemetry::AutoTimer<Telemetry::FONTLIST_INITOTHERFAMILYNAMES> timer;
    
    mFontFamilies.Enumerate(gfxPlatformFontList::InitOtherFamilyNamesProc, this);
}
                                                         
PLDHashOperator
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
    mFaceNamesInitialized = true;

    
    Telemetry::AutoTimer<Telemetry::FONTLIST_INITFACENAMELISTS> timer;
    mFontFamilies.Enumerate(gfxPlatformFontList::InitFaceNameListsProc, this);
}

PLDHashOperator
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

    uint32_t numFonts = preloadFonts.Length();
    for (uint32_t i = 0; i < numFonts; i++) {
        nsAutoString key;
        GenerateFontListKey(preloadFonts[i], key);
        
        
        gfxFontFamily *familyEntry = mFontFamilies.GetWeak(key);
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

    uint32_t i, numFonts = fontlist.Length();

    for (i = 0; i < numFonts; i++) {
        fontlist[i]->mFixedPitch = 1;
    }
}

void
gfxPlatformFontList::LoadBadUnderlineList()
{
    nsAutoTArray<nsString, 10> blacklist;
    gfxFontUtils::GetPrefsFontList("font.blacklist.underline_offset", blacklist);
    uint32_t numFonts = blacklist.Length();
    for (uint32_t i = 0; i < numFonts; i++) {
        nsAutoString key;
        GenerateFontListKey(blacklist[i], key);
        mBadUnderlineFamilyNames.PutEntry(key);
    }
}

bool 
gfxPlatformFontList::ResolveFontName(const nsAString& aFontName, nsAString& aResolvedFontName)
{
    gfxFontFamily *family = FindFamily(aFontName);
    if (family) {
        aResolvedFontName = family->Name();
        return true;
    }
    return false;
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

PLDHashOperator
gfxPlatformFontList::HashEnumFuncForFamilies(nsStringHashKey::KeyType aKey,
                                             nsRefPtr<gfxFontFamily>& aFamilyEntry,
                                             void *aUserArg)
{
    FontListData *data = static_cast<FontListData*>(aUserArg);

    
    
    
    gfxFontStyle style;
    style.language = data->mLangGroup;
    bool needsBold;
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

    static PLDHashOperator AppendFamily(nsStringHashKey::KeyType aKey,
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
gfxPlatformFontList::SystemFindFontForChar(const uint32_t aCh,
                                           int32_t aRunScript,
                                           const gfxFontStyle* aStyle)
 {
    gfxFontEntry* fontEntry = nullptr;

    
    if (mCodepointsWithNoFonts.test(aCh)) {
        return nullptr;
    }

    
    
    
    
    if (aCh == 0xFFFD && mReplacementCharFallbackFamily) {
        bool needsBold;  

        fontEntry =
            mReplacementCharFallbackFamily->FindFontForStyle(*aStyle,
                                                             needsBold);

        
        
        if (fontEntry && fontEntry->TestCharacterMap(aCh)) {
            return fontEntry;
        }
    }

    TimeStamp start = TimeStamp::Now();

    
    bool common = true;
    gfxFontFamily *fallbackFamily = nullptr;
    fontEntry = CommonFontFallback(aCh, aRunScript, aStyle, &fallbackFamily);
 
    
    uint32_t cmapCount = 0;
    if (!fontEntry) {
        common = false;
        fontEntry = GlobalFontFallback(aCh, aRunScript, aStyle, cmapCount,
                                       &fallbackFamily);
    }
    TimeDuration elapsed = TimeStamp::Now() - start;

#ifdef PR_LOGGING
    PRLogModuleInfo *log = gfxPlatform::GetLog(eGfxLog_textrun);

    if (MOZ_UNLIKELY(log)) {
        uint32_t unicodeRange = FindCharUnicodeRange(aCh);
        int32_t script = mozilla::unicode::GetScriptCode(aCh);
        PR_LOG(log, PR_LOG_WARNING,\
               ("(textrun-systemfallback-%s) char: u+%6.6x "
                 "unicode-range: %d script: %d match: [%s]"
                " time: %dus cmaps: %d\n",
                (common ? "common" : "global"), aCh,
                 unicodeRange, script,
                (fontEntry ? NS_ConvertUTF16toUTF8(fontEntry->Name()).get() :
                    "<none>"),
                int32_t(elapsed.ToMicroseconds()),
                cmapCount));
    }
#endif

    
    if (!fontEntry) {
        mCodepointsWithNoFonts.set(aCh);
    } else if (aCh == 0xFFFD && fontEntry && fallbackFamily) {
        mReplacementCharFallbackFamily = fallbackFamily;
    }
 
    
    static bool first = true;
    int32_t intElapsed = int32_t(first ? elapsed.ToMilliseconds() :
                                         elapsed.ToMicroseconds());
    Telemetry::Accumulate((first ? Telemetry::SYSTEM_FONT_FALLBACK_FIRST :
                                   Telemetry::SYSTEM_FONT_FALLBACK),
                          intElapsed);
    first = false;

    
    
    Telemetry::Accumulate(Telemetry::SYSTEM_FONT_FALLBACK_SCRIPT, aRunScript + 1);

    return fontEntry;
}

PLDHashOperator 
gfxPlatformFontList::FindFontForCharProc(nsStringHashKey::KeyType aKey, nsRefPtr<gfxFontFamily>& aFamilyEntry,
     void *userArg)
{
    GlobalFontMatch *data = static_cast<GlobalFontMatch*>(userArg);

    
    aFamilyEntry->FindFontForChar(data);

    return PL_DHASH_NEXT;
}

#define NUM_FALLBACK_FONTS        8

gfxFontEntry*
gfxPlatformFontList::CommonFontFallback(const uint32_t aCh,
                                        int32_t aRunScript,
                                        const gfxFontStyle* aMatchStyle,
                                        gfxFontFamily** aMatchedFamily)
{
    nsAutoTArray<const char*,NUM_FALLBACK_FONTS> defaultFallbacks;
    uint32_t i, numFallbacks;

    gfxPlatform::GetPlatform()->GetCommonFallbackFonts(aCh, aRunScript,
                                                       defaultFallbacks);
    numFallbacks = defaultFallbacks.Length();
    for (i = 0; i < numFallbacks; i++) {
        nsAutoString familyName;
        const char *fallbackFamily = defaultFallbacks[i];

        familyName.AppendASCII(fallbackFamily);
        gfxFontFamily *fallback =
                gfxPlatformFontList::PlatformFontList()->FindFamily(familyName);
        if (!fallback)
            continue;

        gfxFontEntry *fontEntry;
        bool needsBold;  

        
        fontEntry = fallback->FindFontForStyle(*aMatchStyle, needsBold);
        if (fontEntry && fontEntry->TestCharacterMap(aCh)) {
            *aMatchedFamily = fallback;
            return fontEntry;
        }
    }

    return nullptr;
}

gfxFontEntry*
gfxPlatformFontList::GlobalFontFallback(const uint32_t aCh,
                                        int32_t aRunScript,
                                        const gfxFontStyle* aMatchStyle,
                                        uint32_t& aCmapCount,
                                        gfxFontFamily** aMatchedFamily)
{
    
    GlobalFontMatch data(aCh, aRunScript, aMatchStyle);

    
    mFontFamilies.Enumerate(gfxPlatformFontList::FindFontForCharProc, &data);

    aCmapCount = data.mCmapsTested;
    *aMatchedFamily = data.mMatchedFamily;

    return data.mBestMatch;
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
    GenerateFontListKey(aFamily, key);

    NS_ASSERTION(mFontFamilies.Count() != 0, "system font list was not initialized correctly");

    
    if ((familyEntry = mFontFamilies.GetWeak(key))) {
        return familyEntry;
    }

    
    if ((familyEntry = mOtherFamilyNames.GetWeak(key)) != nullptr) {
        return familyEntry;
    }

    
    
    
    
    
    if (!mOtherFamilyNamesInitialized && !IsASCII(aFamily)) {
        InitOtherFamilyNames();
        if ((familyEntry = mOtherFamilyNames.GetWeak(key)) != nullptr) {
            return familyEntry;
        }
    }

    return nullptr;
}

gfxFontEntry*
gfxPlatformFontList::FindFontForFamily(const nsAString& aFamily, const gfxFontStyle* aStyle, bool& aNeedsBold)
{
    gfxFontFamily *familyEntry = FindFamily(aFamily);

    aNeedsBold = false;

    if (familyEntry)
        return familyEntry->FindFontForStyle(*aStyle, aNeedsBold);

    return nullptr;
}

bool
gfxPlatformFontList::GetPrefFontFamilyEntries(eFontPrefLang aLangGroup, nsTArray<nsRefPtr<gfxFontFamily> > *array)
{
    return mPrefFonts.Get(uint32_t(aLangGroup), array);
}

void
gfxPlatformFontList::SetPrefFontFamilyEntries(eFontPrefLang aLangGroup, nsTArray<nsRefPtr<gfxFontFamily> >& array)
{
    mPrefFonts.Put(uint32_t(aLangGroup), array);
}

void 
gfxPlatformFontList::AddOtherFamilyName(gfxFontFamily *aFamilyEntry, nsAString& aOtherFamilyName)
{
    nsAutoString key;
    GenerateFontListKey(aOtherFamilyName, key);

    if (!mOtherFamilyNames.GetWeak(key)) {
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
    if (!mExtraNames->mFullnames.GetWeak(aFullname)) {
        mExtraNames->mFullnames.Put(aFullname, aFontEntry);
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
    if (!mExtraNames->mPostscriptNames.GetWeak(aPostscriptName)) {
        mExtraNames->mPostscriptNames.Put(aPostscriptName, aFontEntry);
#ifdef PR_LOGGING
        LOG_FONTLIST(("(fontlist-postscript) name: %s, psname: %s\n",
                      NS_ConvertUTF16toUTF8(aFontEntry->Name()).get(),
                      NS_ConvertUTF16toUTF8(aPostscriptName).get()));
#endif
    }
}

bool
gfxPlatformFontList::GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName)
{
    aFamilyName.Truncate();
    ResolveFontName(aFontName, aFamilyName);
    return !aFamilyName.IsEmpty();
}

gfxCharacterMap*
gfxPlatformFontList::FindCharMap(gfxCharacterMap *aCmap)
{
    aCmap->CalcHash();
    gfxCharacterMap *cmap = AddCmap(aCmap);
    cmap->mShared = true;
    return cmap;
}


gfxCharacterMap*
gfxPlatformFontList::AddCmap(const gfxCharacterMap* aCharMap)
{
    CharMapHashKey *found =
        mSharedCmaps.PutEntry(const_cast<gfxCharacterMap*>(aCharMap));
    return found->GetKey();
}


void
gfxPlatformFontList::RemoveCmap(const gfxCharacterMap* aCharMap)
{
    
    if (mSharedCmaps.Count() == 0) {
        return;
    }

    
    CharMapHashKey *found =
        mSharedCmaps.GetEntry(const_cast<gfxCharacterMap*>(aCharMap));
    if (found && found->GetKey() == aCharMap) {
        mSharedCmaps.RemoveEntry(const_cast<gfxCharacterMap*>(aCharMap));
    }
}

void 
gfxPlatformFontList::InitLoader()
{
    GetFontFamilyList(mFontFamiliesToLoad);
    mStartIndex = 0;
    mNumFamilies = mFontFamiliesToLoad.Length();
}

#define FONT_LOADER_MAX_TIMESLICE 100  // max time for one pass through RunLoader = 100ms

bool
gfxPlatformFontList::RunLoader()
{
    TimeStamp start = TimeStamp::Now();
    uint32_t i, endIndex = (mStartIndex + mIncrement < mNumFamilies ? mStartIndex + mIncrement : mNumFamilies);
    bool loadCmaps = !UsesSystemFallback() ||
        gfxPlatform::GetPlatform()->UseCmapsDuringSystemFallback();

    
    for (i = mStartIndex; i < endIndex; i++) {
        gfxFontFamily* familyEntry = mFontFamiliesToLoad[i];

        
        familyEntry->FindStyleVariations();
        if (familyEntry->GetFontList().Length() == 0) {
            
            nsAutoString key;
            GenerateFontListKey(familyEntry->Name(), key);
            mFontFamilies.Remove(key);
            continue;
        }

        
        if (loadCmaps) {
            familyEntry->ReadAllCMAPs();
        }

        
        familyEntry->ReadFaceNames(this, NeedFullnamePostscriptNames());

        
        familyEntry->CheckForSimpleFamily();

        
        TimeDuration elapsed = TimeStamp::Now() - start;
        if (elapsed.ToMilliseconds() > FONT_LOADER_MAX_TIMESLICE &&
                i + 1 != endIndex) {
            endIndex = i + 1;
            break;
        }
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

void
gfxPlatformFontList::GetPrefsAndStartLoader()
{
    mIncrement =
        std::max(1u, Preferences::GetUint(FONT_LOADER_FAMILIES_PER_SLICE_PREF));

    uint32_t delay =
        std::max(1u, Preferences::GetUint(FONT_LOADER_DELAY_PREF));
    uint32_t interval =
        std::max(1u, Preferences::GetUint(FONT_LOADER_INTERVAL_PREF));

    StartLoader(delay, interval);
}



static size_t
SizeOfFamilyEntryExcludingThis(const nsAString&               aKey,
                               const nsRefPtr<gfxFontFamily>& aFamily,
                               MallocSizeOf                   aMallocSizeOf,
                               void*                          aUserArg)
{
    FontListSizes *sizes = static_cast<FontListSizes*>(aUserArg);
    aFamily->AddSizeOfExcludingThis(aMallocSizeOf, sizes);

    sizes->mFontListSize += aKey.SizeOfExcludingThisIfUnshared(aMallocSizeOf);

    
    
    return 0;
}


 size_t
gfxPlatformFontList::SizeOfFamilyNameEntryExcludingThis
    (const nsAString&               aKey,
     const nsRefPtr<gfxFontFamily>& aFamily,
     MallocSizeOf                   aMallocSizeOf,
     void*                          aUserArg)
{
    
    
    return aKey.SizeOfExcludingThisIfUnshared(aMallocSizeOf);
}

static size_t
SizeOfFontNameEntryExcludingThis(const nsAString&              aKey,
                                 const nsRefPtr<gfxFontEntry>& aFont,
                                 MallocSizeOf                  aMallocSizeOf,
                                 void*                         aUserArg)
{
    
    
    return aKey.SizeOfExcludingThisIfUnshared(aMallocSizeOf);
}

static size_t
SizeOfPrefFontEntryExcludingThis
    (const uint32_t&                           aKey,
     const nsTArray<nsRefPtr<gfxFontFamily> >& aList,
     MallocSizeOf                              aMallocSizeOf,
     void*                                     aUserArg)
{
    
    
    
    return aList.SizeOfExcludingThis(aMallocSizeOf);
}

static size_t
SizeOfStringEntryExcludingThis(nsStringHashKey*  aHashEntry,
                               MallocSizeOf      aMallocSizeOf,
                               void*             aUserArg)
{
    return aHashEntry->GetKey().SizeOfExcludingThisIfUnshared(aMallocSizeOf);
}

static size_t
SizeOfSharedCmapExcludingThis(CharMapHashKey*   aHashEntry,
                              MallocSizeOf      aMallocSizeOf,
                              void*             aUserArg)
{
    FontListSizes *sizes = static_cast<FontListSizes*>(aUserArg);

    uint32_t size = aHashEntry->GetKey()->SizeOfIncludingThis(aMallocSizeOf);
    sizes->mCharMapsSize += size;

    
    
    return 0;
}

void
gfxPlatformFontList::AddSizeOfExcludingThis(MallocSizeOf aMallocSizeOf,
                                            FontListSizes* aSizes) const
{
    aSizes->mFontListSize +=
        mFontFamilies.SizeOfExcludingThis(SizeOfFamilyEntryExcludingThis,
                                          aMallocSizeOf, aSizes);

    aSizes->mFontListSize +=
        mOtherFamilyNames.SizeOfExcludingThis(SizeOfFamilyNameEntryExcludingThis,
                                              aMallocSizeOf);

    if (mExtraNames) {
        aSizes->mFontListSize +=
            mExtraNames->mFullnames.SizeOfExcludingThis(SizeOfFontNameEntryExcludingThis,
                                                        aMallocSizeOf);
        aSizes->mFontListSize +=
            mExtraNames->mPostscriptNames.SizeOfExcludingThis(SizeOfFontNameEntryExcludingThis,
                                                              aMallocSizeOf);
    }

    aSizes->mFontListSize +=
        mCodepointsWithNoFonts.SizeOfExcludingThis(aMallocSizeOf);
    aSizes->mFontListSize +=
        mFontFamiliesToLoad.SizeOfExcludingThis(aMallocSizeOf);

    aSizes->mFontListSize +=
        mPrefFonts.SizeOfExcludingThis(SizeOfPrefFontEntryExcludingThis,
                                       aMallocSizeOf);

    aSizes->mFontListSize +=
        mBadUnderlineFamilyNames.SizeOfExcludingThis(SizeOfStringEntryExcludingThis,
                                                     aMallocSizeOf);

    aSizes->mFontListSize +=
        mSharedCmaps.SizeOfExcludingThis(SizeOfSharedCmapExcludingThis,
                                         aMallocSizeOf, aSizes);
}

void
gfxPlatformFontList::AddSizeOfIncludingThis(MallocSizeOf aMallocSizeOf,
                                            FontListSizes* aSizes) const
{
    aSizes->mFontListSize += aMallocSizeOf(this);
    AddSizeOfExcludingThis(aMallocSizeOf, aSizes);
}
