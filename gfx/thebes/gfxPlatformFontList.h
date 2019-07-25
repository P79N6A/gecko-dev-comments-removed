





































#ifndef GFXPLATFORMFONTLIST_H_
#define GFXPLATFORMFONTLIST_H_

#include "nsDataHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsTHashtable.h"

#include "gfxFontUtils.h"
#include "gfxFont.h"
#include "gfxPlatform.h"

#include "nsIMemoryReporter.h"
#include "mozilla/FunctionTimer.h"









struct FontListSizes {
    FontListSizes()
        : mFontListSize(0), mFontTableCacheSize(0), mCharMapsSize(0)
    { }

    size_t mFontListSize; 
                          
                          
    size_t mFontTableCacheSize; 
    size_t mCharMapsSize; 
};

class gfxPlatformFontList : protected gfxFontInfoLoader
{
public:
    static gfxPlatformFontList* PlatformFontList() {
        return sPlatformFontList;
    }

    static nsresult Init() {
        NS_TIME_FUNCTION;

        NS_ASSERTION(!sPlatformFontList, "What's this doing here?");
        gfxPlatform::GetPlatform()->CreatePlatformFontList();
        if (!sPlatformFontList) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        return NS_OK;
    }

    static void Shutdown() {
        delete sPlatformFontList;
        sPlatformFontList = nsnull;
    }

    virtual ~gfxPlatformFontList();

    
    virtual nsresult InitFontList();

    void GetFontList (nsIAtom *aLangGroup,
                      const nsACString& aGenericFamily,
                      nsTArray<nsString>& aListOfFonts);

    virtual bool ResolveFontName(const nsAString& aFontName,
                                   nsAString& aResolvedFontName);

    void UpdateFontList() { InitFontList(); }

    void ClearPrefFonts() { mPrefFonts.Clear(); }

    virtual void GetFontFamilyList(nsTArray<nsRefPtr<gfxFontFamily> >& aFamilyArray);

    virtual gfxFontEntry*
    SystemFindFontForChar(const PRUint32 aCh,
                          PRInt32 aRunScript,
                          const gfxFontStyle* aStyle);

    
    virtual gfxFontFamily* FindFamily(const nsAString& aFamily);

    gfxFontEntry* FindFontForFamily(const nsAString& aFamily, const gfxFontStyle* aStyle, bool& aNeedsBold);

    bool GetPrefFontFamilyEntries(eFontPrefLang aLangGroup, nsTArray<nsRefPtr<gfxFontFamily> > *array);
    void SetPrefFontFamilyEntries(eFontPrefLang aLangGroup, nsTArray<nsRefPtr<gfxFontFamily> >& array);

    

    void AddOtherFamilyName(gfxFontFamily *aFamilyEntry, nsAString& aOtherFamilyName);

    void AddFullname(gfxFontEntry *aFontEntry, nsAString& aFullname);

    void AddPostscriptName(gfxFontEntry *aFontEntry, nsAString& aPostscriptName);

    bool NeedFullnamePostscriptNames() { return mNeedFullnamePostscriptNames; }

    

    
    virtual gfxFontEntry* GetDefaultFont(const gfxFontStyle* aStyle,
                                         bool& aNeedsBold) = 0;

    
    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName) = 0;

    
    
    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const PRUint8 *aFontData,
                                           PRUint32 aLength) = 0;

    
    
    virtual bool GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName);

    virtual void SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontListSizes*    aSizes) const;
    virtual void SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontListSizes*    aSizes) const;

protected:
    class MemoryReporter
        : public nsIMemoryMultiReporter
    {
    public:
        NS_DECL_ISUPPORTS
        NS_DECL_NSIMEMORYMULTIREPORTER
    };

    gfxPlatformFontList(bool aNeedFullnamePostscriptNames = true);

    static gfxPlatformFontList *sPlatformFontList;

    static PLDHashOperator FindFontForCharProc(nsStringHashKey::KeyType aKey,
                                               nsRefPtr<gfxFontFamily>& aFamilyEntry,
                                               void* userArg);

    
    virtual gfxFontEntry* CommonFontFallback(const PRUint32 aCh,
                                             PRInt32 aRunScript,
                                             const gfxFontStyle* aMatchStyle);

    
    virtual gfxFontEntry* GlobalFontFallback(const PRUint32 aCh,
                                             PRInt32 aRunScript,
                                             const gfxFontStyle* aMatchStyle,
                                             PRUint32& aCmapCount);

    
    
    virtual bool UsesSystemFallback() { return false; }

    
    void InitOtherFamilyNames();

    static PLDHashOperator InitOtherFamilyNamesProc(nsStringHashKey::KeyType aKey,
                                                    nsRefPtr<gfxFontFamily>& aFamilyEntry,
                                                    void* userArg);

    
    void InitFaceNameLists();

    static PLDHashOperator InitFaceNameListsProc(nsStringHashKey::KeyType aKey,
                                                 nsRefPtr<gfxFontFamily>& aFamilyEntry,
                                                 void* userArg);

    
    virtual void PreloadNamesList();

    
    void LoadBadUnderlineList();

    
    void SetFixedPitch(const nsAString& aFamilyName);

    void GenerateFontListKey(const nsAString& aKeyName, nsAString& aResult);

    static PLDHashOperator
        HashEnumFuncForFamilies(nsStringHashKey::KeyType aKey,
                                nsRefPtr<gfxFontFamily>& aFamilyEntry,
                                void* aUserArg);

    
    virtual void InitLoader();
    virtual bool RunLoader();
    virtual void FinishLoader();

    
    static size_t
    SizeOfFamilyNameEntryExcludingThis(const nsAString&               aKey,
                                       const nsRefPtr<gfxFontFamily>& aFamily,
                                       nsMallocSizeOfFun              aMallocSizeOf,
                                       void*                          aUserArg);

    
    nsRefPtrHashtable<nsStringHashKey, gfxFontFamily> mFontFamilies;

    
    
    nsRefPtrHashtable<nsStringHashKey, gfxFontFamily> mOtherFamilyNames;

    
    bool mOtherFamilyNamesInitialized;

    
    bool mFaceNamesInitialized;

    
    bool mNeedFullnamePostscriptNames;

    
    nsRefPtrHashtable<nsStringHashKey, gfxFontEntry> mFullnames;

    
    nsRefPtrHashtable<nsStringHashKey, gfxFontEntry> mPostscriptNames;

    
    
    nsDataHashtable<nsUint32HashKey, nsTArray<nsRefPtr<gfxFontFamily> > > mPrefFonts;

    
    gfxSparseBitSet mCodepointsWithNoFonts;

    
    
    nsString mReplacementCharFallbackFamily;

    nsTHashtable<nsStringHashKey> mBadUnderlineFamilyNames;

    
    nsTArray<nsRefPtr<gfxFontFamily> > mFontFamiliesToLoad;
    PRUint32 mStartIndex;
    PRUint32 mIncrement;
    PRUint32 mNumFamilies;
};

#endif
