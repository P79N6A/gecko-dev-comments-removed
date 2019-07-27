




#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif
#include "prlog.h"

#include "gfxPlatformFontList.h"
#include "gfxTextRun.h"
#include "gfxUserFontSet.h"

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
#define LOG_FONTINIT(args) PR_LOG(gfxPlatform::GetLog(eGfxLog_fontinit), \
                               PR_LOG_DEBUG, args)
#define LOG_FONTINIT_ENABLED() PR_LOG_TEST( \
                                   gfxPlatform::GetLog(eGfxLog_fontinit), \
                                   PR_LOG_DEBUG)

#endif 

gfxPlatformFontList *gfxPlatformFontList::sPlatformFontList = nullptr;






const gfxFontEntry::ScriptRange gfxPlatformFontList::sComplexScriptRanges[] = {
    
    
    
    
    
    { 0x0600, 0x06FF, { TRUETYPE_TAG('a','r','a','b'), 0, 0 } },
    { 0x0700, 0x074F, { TRUETYPE_TAG('s','y','r','c'), 0, 0 } },
    { 0x0750, 0x077F, { TRUETYPE_TAG('a','r','a','b'), 0, 0 } },
    { 0x08A0, 0x08FF, { TRUETYPE_TAG('a','r','a','b'), 0, 0 } },
    { 0x0900, 0x097F, { TRUETYPE_TAG('d','e','v','2'),
                        TRUETYPE_TAG('d','e','v','a'), 0 } },
    { 0x0980, 0x09FF, { TRUETYPE_TAG('b','n','g','2'),
                        TRUETYPE_TAG('b','e','n','g'), 0 } },
    { 0x0A00, 0x0A7F, { TRUETYPE_TAG('g','u','r','2'),
                        TRUETYPE_TAG('g','u','r','u'), 0 } },
    { 0x0A80, 0x0AFF, { TRUETYPE_TAG('g','j','r','2'),
                        TRUETYPE_TAG('g','u','j','r'), 0 } },
    { 0x0B00, 0x0B7F, { TRUETYPE_TAG('o','r','y','2'),
                        TRUETYPE_TAG('o','r','y','a'), 0 } },
    { 0x0B80, 0x0BFF, { TRUETYPE_TAG('t','m','l','2'),
                        TRUETYPE_TAG('t','a','m','l'), 0 } },
    { 0x0C00, 0x0C7F, { TRUETYPE_TAG('t','e','l','2'),
                        TRUETYPE_TAG('t','e','l','u'), 0 } },
    { 0x0C80, 0x0CFF, { TRUETYPE_TAG('k','n','d','2'),
                        TRUETYPE_TAG('k','n','d','a'), 0 } },
    { 0x0D00, 0x0D7F, { TRUETYPE_TAG('m','l','m','2'),
                        TRUETYPE_TAG('m','l','y','m'), 0 } },
    { 0x0D80, 0x0DFF, { TRUETYPE_TAG('s','i','n','h'), 0, 0 } },
    { 0x0E80, 0x0EFF, { TRUETYPE_TAG('l','a','o',' '), 0, 0 } },
    { 0x0F00, 0x0FFF, { TRUETYPE_TAG('t','i','b','t'), 0, 0 } },
    { 0x1000, 0x109f, { TRUETYPE_TAG('m','y','m','r'),
                        TRUETYPE_TAG('m','y','m','2'), 0 } },
    { 0x1780, 0x17ff, { TRUETYPE_TAG('k','h','m','r'), 0, 0 } },
    
    { 0xaa60, 0xaa7f, { TRUETYPE_TAG('m','y','m','r'),
                        TRUETYPE_TAG('m','y','m','2'), 0 } },
    
    { 0, 0, { 0, 0, 0 } } 
};


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
    ~gfxFontListPrefObserver() {}
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
};

static gfxFontListPrefObserver* gFontListPrefObserver = nullptr;

NS_IMPL_ISUPPORTS(gfxFontListPrefObserver, nsIObserver)

NS_IMETHODIMP
gfxFontListPrefObserver::Observe(nsISupports     *aSubject,
                                 const char      *aTopic,
                                 const char16_t *aData)
{
    NS_ASSERTION(!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID), "invalid topic");
    
    
    gfxPlatformFontList::PlatformFontList()->ClearPrefFonts();
    gfxFontCache::GetCache()->AgeAllGenerations();
    return NS_OK;
}

MOZ_DEFINE_MALLOC_SIZE_OF(FontListMallocSizeOf)

NS_IMPL_ISUPPORTS(gfxPlatformFontList::MemoryReporter, nsIMemoryReporter)

NS_IMETHODIMP
gfxPlatformFontList::MemoryReporter::CollectReports(
    nsIMemoryReporterCallback* aCb, nsISupports* aClosure, bool aAnonymize)
{
    FontListSizes sizes;
    sizes.mFontListSize = 0;
    sizes.mFontTableCacheSize = 0;
    sizes.mCharMapsSize = 0;

    gfxPlatformFontList::PlatformFontList()->AddSizeOfIncludingThis(&FontListMallocSizeOf,
                                                                    &sizes);

    nsresult rv;
    rv = aCb->Callback(EmptyCString(),
                       NS_LITERAL_CSTRING("explicit/gfx/font-list"),
                       KIND_HEAP, UNITS_BYTES, sizes.mFontListSize,
                       NS_LITERAL_CSTRING("Memory used to manage the list of font families and faces."),
                       aClosure);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aCb->Callback(EmptyCString(),
                       NS_LITERAL_CSTRING("explicit/gfx/font-charmaps"),
                       KIND_HEAP, UNITS_BYTES, sizes.mCharMapsSize,
                       NS_LITERAL_CSTRING("Memory used to record the character coverage of individual fonts."),
                       aClosure);
    NS_ENSURE_SUCCESS(rv, rv);

    if (sizes.mFontTableCacheSize) {
        aCb->Callback(EmptyCString(),
                      NS_LITERAL_CSTRING("explicit/gfx/font-tables"),
                      KIND_HEAP, UNITS_BYTES, sizes.mFontTableCacheSize,
                      NS_LITERAL_CSTRING("Memory used for cached font metrics and layout tables."),
                      aClosure);
    }

    return NS_OK;
}

gfxPlatformFontList::gfxPlatformFontList(bool aNeedFullnamePostscriptNames)
    : mFontFamilies(64), mOtherFamilyNames(16),
      mPrefFonts(8), mBadUnderlineFamilyNames(8), mSharedCmaps(8),
      mStartIndex(0), mIncrement(1), mNumFamilies(0)
{
    mOtherFamilyNamesInitialized = false;

    if (aNeedFullnamePostscriptNames) {
        mExtraNames = new ExtraNames();
    }
    mFaceNameListsInitialized = false;

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
    mFaceNameListsInitialized = false;
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

struct InitOtherNamesData {
    InitOtherNamesData(gfxPlatformFontList *aFontList,
                       TimeStamp aStartTime)
        : mFontList(aFontList), mStartTime(aStartTime), mTimedOut(false)
    {}

    gfxPlatformFontList *mFontList;
    TimeStamp mStartTime;
    bool mTimedOut;
};

void 
gfxPlatformFontList::InitOtherFamilyNames()
{
    if (mOtherFamilyNamesInitialized) {
        return;
    }

    TimeStamp start = TimeStamp::Now();

    
    InitOtherNamesData otherNamesData(this, start);

    mFontFamilies.Enumerate(gfxPlatformFontList::InitOtherFamilyNamesProc,
                            &otherNamesData);

    if (!otherNamesData.mTimedOut) {
        mOtherFamilyNamesInitialized = true;
    }
    TimeStamp end = TimeStamp::Now();
    Telemetry::AccumulateTimeDelta(Telemetry::FONTLIST_INITOTHERFAMILYNAMES,
                                   start, end);

#ifdef PR_LOGGING
    if (LOG_FONTINIT_ENABLED()) {
        TimeDuration elapsed = end - start;
        LOG_FONTINIT(("(fontinit) InitOtherFamilyNames took %8.2f ms %s",
                      elapsed.ToMilliseconds(),
                      (otherNamesData.mTimedOut ? "timeout" : "")));
    }
#endif
}

#define OTHERNAMES_TIMEOUT 200

PLDHashOperator
gfxPlatformFontList::InitOtherFamilyNamesProc(nsStringHashKey::KeyType aKey,
                                              nsRefPtr<gfxFontFamily>& aFamilyEntry,
                                              void* userArg)
{
    InitOtherNamesData *data = static_cast<InitOtherNamesData*>(userArg);

    aFamilyEntry->ReadOtherFamilyNames(data->mFontList);
    TimeDuration elapsed = TimeStamp::Now() - data->mStartTime;
    if (elapsed.ToMilliseconds() > OTHERNAMES_TIMEOUT) {
        data->mTimedOut = true;
        return PL_DHASH_STOP;
    }
    return PL_DHASH_NEXT;
}
 
struct ReadFaceNamesData {
    ReadFaceNamesData(gfxPlatformFontList *aFontList, TimeStamp aStartTime)
        : mFontList(aFontList), mStartTime(aStartTime), mTimedOut(false),
          mFirstChar(0)
    {}

    gfxPlatformFontList *mFontList;
    TimeStamp mStartTime;
    bool mTimedOut;

    
    
    char16_t mFirstChar;
};

gfxFontEntry*
gfxPlatformFontList::SearchFamiliesForFaceName(const nsAString& aFaceName)
{
    TimeStamp start = TimeStamp::Now();
    gfxFontEntry *lookup = nullptr;

    ReadFaceNamesData faceNameListsData(this, start);

    
    faceNameListsData.mFirstChar = ToLowerCase(aFaceName.CharAt(0));
    mFontFamilies.Enumerate(gfxPlatformFontList::ReadFaceNamesProc,
                            &faceNameListsData);
    lookup = FindFaceName(aFaceName);

    TimeStamp end = TimeStamp::Now();
    Telemetry::AccumulateTimeDelta(Telemetry::FONTLIST_INITFACENAMELISTS,
                                   start, end);
#ifdef PR_LOGGING
    if (LOG_FONTINIT_ENABLED()) {
        TimeDuration elapsed = end - start;
        LOG_FONTINIT(("(fontinit) SearchFamiliesForFaceName took %8.2f ms %s %s",
                      elapsed.ToMilliseconds(),
                      (lookup ? "found name" : ""),
                      (faceNameListsData.mTimedOut ? "timeout" : "")));
    }
#endif

    return lookup;
}


#define NAMELIST_TIMEOUT  200

PLDHashOperator
gfxPlatformFontList::ReadFaceNamesProc(nsStringHashKey::KeyType aKey,
                                       nsRefPtr<gfxFontFamily>& aFamilyEntry,
                                       void* userArg)
{
    ReadFaceNamesData *data = static_cast<ReadFaceNamesData*>(userArg);
    gfxPlatformFontList *fc = data->mFontList;

    
    if (data->mFirstChar && ToLowerCase(aKey.CharAt(0)) != data->mFirstChar) {
        return PL_DHASH_NEXT;
    }

    aFamilyEntry->ReadFaceNames(fc, fc->NeedFullnamePostscriptNames());

    TimeDuration elapsed = TimeStamp::Now() - data->mStartTime;
    if (elapsed.ToMilliseconds() > NAMELIST_TIMEOUT) {
        data->mTimedOut = true;
        return PL_DHASH_STOP;
    }
    return PL_DHASH_NEXT;
}

gfxFontEntry*
gfxPlatformFontList::FindFaceName(const nsAString& aFaceName)
{
    gfxFontEntry *lookup;

    
    if (mExtraNames &&
        ((lookup = mExtraNames->mPostscriptNames.GetWeak(aFaceName)) ||
         (lookup = mExtraNames->mFullnames.GetWeak(aFaceName)))) {
        return lookup;
    }

    return nullptr;
}

gfxFontEntry*
gfxPlatformFontList::LookupInFaceNameLists(const nsAString& aFaceName)
{
    gfxFontEntry *lookup = nullptr;

    
    
    
    if (!mFaceNameListsInitialized) {
        lookup = SearchFamiliesForFaceName(aFaceName);
        if (lookup) {
            return lookup;
        }
    }

    
    if (!(lookup = FindFaceName(aFaceName))) {
        
        if (!mFaceNameListsInitialized) {
            if (!mFaceNamesMissed) {
                mFaceNamesMissed = new nsTHashtable<nsStringHashKey>(2);
            }
            mFaceNamesMissed->PutEntry(aFaceName);
        }
    }

    return lookup;
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

static PLDHashOperator
RebuildLocalFonts(nsPtrHashKey<gfxUserFontSet>* aKey,
                  void* aClosure)
{
    aKey->GetKey()->RebuildLocalRules();
    return PL_DHASH_NEXT;
}

void
gfxPlatformFontList::UpdateFontList()
{
    InitFontList();
    mUserFontSetList.EnumerateEntries(RebuildLocalFonts, nullptr);
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
    explicit FontFamilyListData(nsTArray<nsRefPtr<gfxFontFamily> >& aFamilyArray) 
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

    if (MOZ_UNLIKELY(PR_LOG_TEST(log, PR_LOG_WARNING))) {
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
gfxPlatformFontList::CheckFamily(gfxFontFamily *aFamily)
{
    if (aFamily && !aFamily->HasStyles()) {
        aFamily->FindStyleVariations();
        aFamily->CheckForSimpleFamily();
    }

    if (aFamily && aFamily->GetFontList().Length() == 0) {
        
        nsAutoString key;
        GenerateFontListKey(aFamily->Name(), key);
        mFontFamilies.Remove(key);
        return nullptr;
    }

    return aFamily;
}

gfxFontFamily* 
gfxPlatformFontList::FindFamily(const nsAString& aFamily, bool aUseSystemFonts)
{
    nsAutoString key;
    gfxFontFamily *familyEntry;
    GenerateFontListKey(aFamily, key);

    NS_ASSERTION(mFontFamilies.Count() != 0, "system font list was not initialized correctly");

    
    if ((familyEntry = mFontFamilies.GetWeak(key))) {
        return CheckFamily(familyEntry);
    }

#if defined(XP_MACOSX)
    
    if (aUseSystemFonts) {
        if ((familyEntry = mSystemFontFamilies.GetWeak(key)) != nullptr) {
            return CheckFamily(familyEntry);
        }
    }
#endif

    
    if ((familyEntry = mOtherFamilyNames.GetWeak(key)) != nullptr) {
        return CheckFamily(familyEntry);
    }

    
    
    
    
    
    if (!mOtherFamilyNamesInitialized && !IsASCII(aFamily)) {
        InitOtherFamilyNames();
        if ((familyEntry = mOtherFamilyNames.GetWeak(key)) != nullptr) {
            return CheckFamily(familyEntry);
        } else if (!mOtherFamilyNamesInitialized) {
            
            
            if (!mOtherNamesMissed) {
                mOtherNamesMissed = new nsTHashtable<nsStringHashKey>(2);
            }
            mOtherNamesMissed->PutEntry(key);
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
    gfxFontFamily *ff = FindFamily(aFontName);
    if (!ff) {
        return false;
    }
    aFamilyName.Assign(ff->Name());
    return true;
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

static PLDHashOperator AppendFamilyToList(nsStringHashKey::KeyType aKey,
                                          nsRefPtr<gfxFontFamily>& aFamilyEntry,
                                          void *aUserArg)
{
    nsTArray<nsString> *familyNames = static_cast<nsTArray<nsString> *>(aUserArg);
    familyNames->AppendElement(aFamilyEntry->Name());
    return PL_DHASH_NEXT;
}

void
gfxPlatformFontList::GetFontFamilyNames(nsTArray<nsString>& aFontFamilyNames)
{
    mFontFamilies.Enumerate(AppendFamilyToList, &aFontFamilyNames);
}

void 
gfxPlatformFontList::InitLoader()
{
    GetFontFamilyNames(mFontInfo->mFontFamiliesToLoad);
    mStartIndex = 0;
    mNumFamilies = mFontInfo->mFontFamiliesToLoad.Length();
    memset(&(mFontInfo->mLoadStats), 0, sizeof(mFontInfo->mLoadStats));
}

#define FONT_LOADER_MAX_TIMESLICE 100  // max time for one pass through RunLoader = 100ms

bool
gfxPlatformFontList::LoadFontInfo()
{
    TimeStamp start = TimeStamp::Now();
    uint32_t i, endIndex = mNumFamilies;
    bool loadCmaps = !UsesSystemFallback() ||
        gfxPlatform::GetPlatform()->UseCmapsDuringSystemFallback();

    
    for (i = mStartIndex; i < endIndex; i++) {
        nsAutoString key;
        gfxFontFamily *familyEntry;
        GenerateFontListKey(mFontInfo->mFontFamiliesToLoad[i], key);

        
        if (!(familyEntry = mFontFamilies.GetWeak(key))) {
            continue;
        }

        
        familyEntry->ReadFaceNames(this, NeedFullnamePostscriptNames(), mFontInfo);

        
        if (loadCmaps) {
            familyEntry->ReadAllCMAPs(mFontInfo);
        }

        
        TimeDuration elapsed = TimeStamp::Now() - start;
        if (elapsed.ToMilliseconds() > FONT_LOADER_MAX_TIMESLICE &&
                i + 1 != endIndex) {
            endIndex = i + 1;
            break;
        }
    }

    mStartIndex = endIndex;
    bool done = mStartIndex >= mNumFamilies;

#ifdef PR_LOGGING
    if (LOG_FONTINIT_ENABLED()) {
        TimeDuration elapsed = TimeStamp::Now() - start;
        LOG_FONTINIT(("(fontinit) fontloader load pass %8.2f ms done %s\n",
                      elapsed.ToMilliseconds(), (done ? "true" : "false")));
    }
#endif

    if (done) {
        mOtherFamilyNamesInitialized = true;
        mFaceNameListsInitialized = true;
    }

    return done;
}

struct LookupMissedFaceNamesData {
    explicit LookupMissedFaceNamesData(gfxPlatformFontList *aFontList)
        : mFontList(aFontList), mFoundName(false) {}

    gfxPlatformFontList *mFontList;
    bool mFoundName;
};

 PLDHashOperator
gfxPlatformFontList::LookupMissedFaceNamesProc(nsStringHashKey *aKey,
                                               void *aUserArg)
{
    LookupMissedFaceNamesData *data =
        reinterpret_cast<LookupMissedFaceNamesData*>(aUserArg);

    if (data->mFontList->FindFaceName(aKey->GetKey())) {
        data->mFoundName = true;
        return PL_DHASH_STOP;
    }
    return PL_DHASH_NEXT;
}

struct LookupMissedOtherNamesData {
    explicit LookupMissedOtherNamesData(gfxPlatformFontList *aFontList)
        : mFontList(aFontList), mFoundName(false) {}

    gfxPlatformFontList *mFontList;
    bool mFoundName;
};

 PLDHashOperator
gfxPlatformFontList::LookupMissedOtherNamesProc(nsStringHashKey *aKey,
                                                void *aUserArg)
{
    LookupMissedOtherNamesData *data =
        reinterpret_cast<LookupMissedOtherNamesData*>(aUserArg);

    if (data->mFontList->FindFamily(aKey->GetKey())) {
        data->mFoundName = true;
        return PL_DHASH_STOP;
    }
    return PL_DHASH_NEXT;
}

void 
gfxPlatformFontList::CleanupLoader()
{
    mFontFamiliesToLoad.Clear();
    mNumFamilies = 0;
    bool rebuilt = false, forceReflow = false;

    
    if (mFaceNamesMissed &&
        mFaceNamesMissed->Count() != 0) {
        LookupMissedFaceNamesData namedata(this);
        mFaceNamesMissed->EnumerateEntries(LookupMissedFaceNamesProc, &namedata);
        if (namedata.mFoundName) {
            rebuilt = true;
            mUserFontSetList.EnumerateEntries(RebuildLocalFonts, nullptr);
        }
        mFaceNamesMissed = nullptr;
    }

    if (mOtherNamesMissed) {
        LookupMissedOtherNamesData othernamesdata(this);
        mOtherNamesMissed->EnumerateEntries(LookupMissedOtherNamesProc,
                                            &othernamesdata);
        mOtherNamesMissed = nullptr;
        if (othernamesdata.mFoundName) {
            forceReflow = true;
            ForceGlobalReflow();
        }
    }

#ifdef PR_LOGGING
    if (LOG_FONTINIT_ENABLED() && mFontInfo) {
        LOG_FONTINIT(("(fontinit) fontloader load thread took %8.2f ms "
                      "%d families %d fonts %d cmaps "
                      "%d facenames %d othernames %s %s",
                      mLoadTime.ToMilliseconds(),
                      mFontInfo->mLoadStats.families,
                      mFontInfo->mLoadStats.fonts,
                      mFontInfo->mLoadStats.cmaps,
                      mFontInfo->mLoadStats.facenames,
                      mFontInfo->mLoadStats.othernames,
                      (rebuilt ? "(userfont sets rebuilt)" : ""),
                      (forceReflow ? "(global reflow)" : "")));
    }
#endif

    gfxFontInfoLoader::CleanupLoader();
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

void
gfxPlatformFontList::ForceGlobalReflow()
{
    
    static const char kPrefName[] = "font.internaluseonly.changed";
    bool fontInternalChange = Preferences::GetBool(kPrefName, false);
    Preferences::SetBool(kPrefName, !fontInternalChange);
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
        mBadUnderlineFamilyNames.SizeOfExcludingThis(aMallocSizeOf);

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
