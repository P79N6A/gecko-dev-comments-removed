




#ifndef GFXPLATFORMFONTLIST_H_
#define GFXPLATFORMFONTLIST_H_

#include "nsDataHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsTHashtable.h"

#include "gfxFontUtils.h"
#include "gfxFontInfoLoader.h"
#include "gfxFont.h"
#include "gfxPlatform.h"

#include "nsIMemoryReporter.h"
#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"

class CharMapHashKey : public PLDHashEntryHdr
{
public:
    typedef gfxCharacterMap* KeyType;
    typedef const gfxCharacterMap* KeyTypePointer;

    explicit CharMapHashKey(const gfxCharacterMap *aCharMap) :
        mCharMap(const_cast<gfxCharacterMap*>(aCharMap))
    {
        MOZ_COUNT_CTOR(CharMapHashKey);
    }
    CharMapHashKey(const CharMapHashKey& toCopy) :
        mCharMap(toCopy.mCharMap)
    {
        MOZ_COUNT_CTOR(CharMapHashKey);
    }
    ~CharMapHashKey()
    {
        MOZ_COUNT_DTOR(CharMapHashKey);
    }

    gfxCharacterMap* GetKey() const { return mCharMap; }

    bool KeyEquals(const gfxCharacterMap *aCharMap) const {
        NS_ASSERTION(!aCharMap->mBuildOnTheFly && !mCharMap->mBuildOnTheFly,
                     "custom cmap used in shared cmap hashtable");
        
        if (aCharMap->mHash != mCharMap->mHash)
        {
            return false;
        }
        return mCharMap->Equals(aCharMap);
    }

    static const gfxCharacterMap* KeyToPointer(gfxCharacterMap *aCharMap) {
        return aCharMap;
    }
    static PLDHashNumber HashKey(const gfxCharacterMap *aCharMap) {
        return aCharMap->mHash;
    }

    enum { ALLOW_MEMMOVE = true };

protected:
    gfxCharacterMap *mCharMap;
};









struct FontListSizes {
    uint32_t mFontListSize; 
                            
                            
    uint32_t mFontTableCacheSize; 
    uint32_t mCharMapsSize; 
};

class gfxUserFontSet;

class gfxPlatformFontList : public gfxFontInfoLoader
{
public:
    static gfxPlatformFontList* PlatformFontList() {
        return sPlatformFontList;
    }

    static nsresult Init() {
        NS_ASSERTION(!sPlatformFontList, "What's this doing here?");
        gfxPlatform::GetPlatform()->CreatePlatformFontList();
        if (!sPlatformFontList) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        return NS_OK;
    }

    static void Shutdown() {
        delete sPlatformFontList;
        sPlatformFontList = nullptr;
    }

    virtual ~gfxPlatformFontList();

    
    virtual nsresult InitFontList();

    void GetFontList (nsIAtom *aLangGroup,
                      const nsACString& aGenericFamily,
                      nsTArray<nsString>& aListOfFonts);

    void UpdateFontList();

    void ClearPrefFonts() { mPrefFonts.Clear(); }

    virtual void GetFontFamilyList(nsTArray<nsRefPtr<gfxFontFamily> >& aFamilyArray);

    virtual gfxFontEntry*
    SystemFindFontForChar(const uint32_t aCh,
                          int32_t aRunScript,
                          const gfxFontStyle* aStyle);

    virtual gfxFontFamily* FindFamily(const nsAString& aFamily,
                                      bool aUseSystemFonts = false);

    gfxFontEntry* FindFontForFamily(const nsAString& aFamily, const gfxFontStyle* aStyle, bool& aNeedsBold);

    bool GetPrefFontFamilyEntries(eFontPrefLang aLangGroup, nsTArray<nsRefPtr<gfxFontFamily> > *array);
    void SetPrefFontFamilyEntries(eFontPrefLang aLangGroup, nsTArray<nsRefPtr<gfxFontFamily> >& array);

    

    void AddOtherFamilyName(gfxFontFamily *aFamilyEntry, nsAString& aOtherFamilyName);

    void AddFullname(gfxFontEntry *aFontEntry, nsAString& aFullname);

    void AddPostscriptName(gfxFontEntry *aFontEntry, nsAString& aPostscriptName);

    bool NeedFullnamePostscriptNames() { return mExtraNames != nullptr; }

    

    
    virtual gfxFontFamily* GetDefaultFont(const gfxFontStyle* aStyle) = 0;

    
    virtual gfxFontEntry* LookupLocalFont(const nsAString& aFontName,
                                          uint16_t aWeight,
                                          int16_t aStretch,
                                          bool aItalic) = 0;

    
    
    virtual gfxFontEntry* MakePlatformFont(const nsAString& aFontName,
                                           uint16_t aWeight,
                                           int16_t aStretch,
                                           bool aItalic,
                                           const uint8_t* aFontData,
                                           uint32_t aLength) = 0;

    
    
    virtual bool GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName);

    virtual void AddSizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontListSizes* aSizes) const;
    virtual void AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontListSizes* aSizes) const;

    
    
    gfxCharacterMap* FindCharMap(gfxCharacterMap *aCmap);

    
    gfxCharacterMap* AddCmap(const gfxCharacterMap *aCharMap);

    
    void RemoveCmap(const gfxCharacterMap *aCharMap);

    
    void AddUserFontSet(gfxUserFontSet *aUserFontSet) {
        mUserFontSetList.PutEntry(aUserFontSet);
    }

    void RemoveUserFontSet(gfxUserFontSet *aUserFontSet) {
        mUserFontSetList.RemoveEntry(aUserFontSet);
    }

    static const gfxFontEntry::ScriptRange sComplexScriptRanges[];

protected:
    class MemoryReporter MOZ_FINAL : public nsIMemoryReporter
    {
        ~MemoryReporter() {}
    public:
        NS_DECL_ISUPPORTS
        NS_DECL_NSIMEMORYREPORTER
    };

    explicit gfxPlatformFontList(bool aNeedFullnamePostscriptNames = true);

    static gfxPlatformFontList *sPlatformFontList;

    static PLDHashOperator FindFontForCharProc(nsStringHashKey::KeyType aKey,
                                               nsRefPtr<gfxFontFamily>& aFamilyEntry,
                                               void* userArg);

    
    gfxFontEntry* CommonFontFallback(const uint32_t aCh,
                                     int32_t aRunScript,
                                     const gfxFontStyle* aMatchStyle,
                                     gfxFontFamily** aMatchedFamily);

    
    virtual gfxFontEntry* GlobalFontFallback(const uint32_t aCh,
                                             int32_t aRunScript,
                                             const gfxFontStyle* aMatchStyle,
                                             uint32_t& aCmapCount,
                                             gfxFontFamily** aMatchedFamily);

    
    
    virtual bool UsesSystemFallback() { return false; }

    
    gfxFontFamily* CheckFamily(gfxFontFamily *aFamily);

    
    void InitOtherFamilyNames();

    static PLDHashOperator
    InitOtherFamilyNamesProc(nsStringHashKey::KeyType aKey,
                             nsRefPtr<gfxFontFamily>& aFamilyEntry,
                             void* userArg);

    
    
    
    gfxFontEntry* SearchFamiliesForFaceName(const nsAString& aFaceName);

    static PLDHashOperator
    ReadFaceNamesProc(nsStringHashKey::KeyType aKey,
                      nsRefPtr<gfxFontFamily>& aFamilyEntry,
                      void* userArg);

    
    gfxFontEntry* FindFaceName(const nsAString& aFaceName);

    
    
    virtual gfxFontEntry* LookupInFaceNameLists(const nsAString& aFontName);

    static PLDHashOperator LookupMissedFaceNamesProc(nsStringHashKey *aKey,
                                                     void *aUserArg);

    static PLDHashOperator LookupMissedOtherNamesProc(nsStringHashKey *aKey,
                                                      void *aUserArg);

    
    virtual void PreloadNamesList();

    
    void LoadBadUnderlineList();

    void GenerateFontListKey(const nsAString& aKeyName, nsAString& aResult);

    static PLDHashOperator
        HashEnumFuncForFamilies(nsStringHashKey::KeyType aKey,
                                nsRefPtr<gfxFontFamily>& aFamilyEntry,
                                void* aUserArg);

    virtual void GetFontFamilyNames(nsTArray<nsString>& aFontFamilyNames);

    
    virtual void InitLoader();
    virtual bool LoadFontInfo();
    virtual void CleanupLoader();

    
    void GetPrefsAndStartLoader();

    
    void ForceGlobalReflow();

    
    static size_t
    SizeOfFamilyNameEntryExcludingThis(const nsAString&               aKey,
                                       const nsRefPtr<gfxFontFamily>& aFamily,
                                       mozilla::MallocSizeOf          aMallocSizeOf,
                                       void*                          aUserArg);

    
    nsRefPtrHashtable<nsStringHashKey, gfxFontFamily> mFontFamilies;

#if defined(XP_MACOSX)
    
    nsRefPtrHashtable<nsStringHashKey, gfxFontFamily> mSystemFontFamilies;
#endif

    
    
    nsRefPtrHashtable<nsStringHashKey, gfxFontFamily> mOtherFamilyNames;

    
    bool mOtherFamilyNamesInitialized;

    
    bool mFaceNameListsInitialized;

    struct ExtraNames {
      ExtraNames() : mFullnames(64), mPostscriptNames(64) {}
      
      nsRefPtrHashtable<nsStringHashKey, gfxFontEntry> mFullnames;
      
      nsRefPtrHashtable<nsStringHashKey, gfxFontEntry> mPostscriptNames;
    };
    nsAutoPtr<ExtraNames> mExtraNames;

    
    nsAutoPtr<nsTHashtable<nsStringHashKey> > mFaceNamesMissed;

    
    nsAutoPtr<nsTHashtable<nsStringHashKey> > mOtherNamesMissed;

    
    
    nsDataHashtable<nsUint32HashKey, nsTArray<nsRefPtr<gfxFontFamily> > > mPrefFonts;

    
    gfxSparseBitSet mCodepointsWithNoFonts;

    
    
    nsRefPtr<gfxFontFamily> mReplacementCharFallbackFamily;

    nsTHashtable<nsStringHashKey> mBadUnderlineFamilyNames;

    
    
    nsTHashtable<CharMapHashKey> mSharedCmaps;

    
    nsTArray<nsRefPtr<gfxFontFamily> > mFontFamiliesToLoad;
    uint32_t mStartIndex;
    uint32_t mIncrement;
    uint32_t mNumFamilies;

    nsTHashtable<nsPtrHashKey<gfxUserFontSet> > mUserFontSetList;
};

#endif
