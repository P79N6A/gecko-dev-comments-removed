






































#ifndef GFXQUARTZFONTCACHE_H_
#define GFXQUARTZFONTCACHE_H_

#include "nsDataHashtable.h"
#include "nsRefPtrHashtable.h"

#include "gfxFontUtils.h"
#include "gfxAtsuiFonts.h"
#include "gfxPlatform.h"

#include <Carbon/Carbon.h>

#include "nsUnicharUtils.h"
#include "nsTArray.h"


struct FontSearch {
    FontSearch(const PRUint32 aCharacter, gfxFont *aFont) :
        ch(aCharacter), fontToMatch(aFont), matchRank(0) {
    }
    const PRUint32 ch;
    gfxFont *fontToMatch;
    PRInt32 matchRank;
    nsRefPtr<MacOSFontEntry> bestMatch;
};

class MacOSFamilyEntry;
class gfxQuartzFontCache;
class FontEntryStandardFaceComparator;


class MacOSFontEntry : public gfxFontEntry
{
public:
    friend class gfxQuartzFontCache;
    friend class FontEntryStandardFaceComparator;

    
    MacOSFontEntry(const nsAString& aPostscriptName, PRInt32 aAppleWeight, PRUint32 aTraits, 
                   PRBool aIsStandardFace = PR_FALSE);

    PRUint32 Traits() { return mTraits; }

    ATSFontRef GetFontRef();
    nsresult ReadCMAP();

protected:
    
    MacOSFontEntry(const nsAString& aPostscriptName, ATSFontRef aFontRef,
                   PRUint16 aWeight, PRUint16 aStretch, PRUint32 aItalicStyle,
                   gfxUserFontData *aUserFontData);

    PRUint32 mTraits;

    ATSFontRef mATSFontRef;
    PRPackedBool mATSFontRefInitialized;
    PRPackedBool mStandardFace;
};


class AddOtherFamilyNameFunctor;


class MacOSFamilyEntry : public gfxFontFamily
{
public:

    friend class gfxQuartzFontCache;

    
    MacOSFamilyEntry(nsAString &aName) :
        gfxFontFamily(aName), mOtherFamilyNamesInitialized(PR_FALSE), mHasOtherFamilyNames(PR_FALSE)
    {}
  
    virtual ~MacOSFamilyEntry() {}
        
    virtual void LocalizedName(nsAString& aLocalizedName);
    virtual PRBool HasOtherFamilyNames();
    
    nsTArray<nsRefPtr<MacOSFontEntry> >& GetFontList() { return mAvailableFonts; }
    
    void AddFontEntry(nsRefPtr<MacOSFontEntry> aFontEntry) {
        mAvailableFonts.AppendElement(aFontEntry);
    }
    
    
    
    
    MacOSFontEntry* FindFont(const gfxFontStyle* aStyle, PRBool& aNeedsBold);
    
    
    
    void FindFontForChar(FontSearch *aMatchData);
    
    
    virtual void ReadOtherFamilyNames(AddOtherFamilyNameFunctor& aOtherFamilyFunctor);
    
    
    MacOSFontEntry* FindFont(const nsAString& aPostscriptName);

    
    void ReadCMAP() {
        PRUint32 i, numFonts = mAvailableFonts.Length();
        for (i = 0; i < numFonts; i++)
            mAvailableFonts[i]->ReadCMAP();
    }

    
    void SetBadUnderlineFont(PRBool aIsBadUnderlineFont) {
        PRUint32 i, numFonts = mAvailableFonts.Length();
        for (i = 0; i < numFonts; i++)
            mAvailableFonts[i]->mIsBadUnderlineFont = aIsBadUnderlineFont;
    }

    
    void SortAvailableFonts();

protected:
    
    
    
    
    PRBool FindFontsWithTraits(gfxFontEntry* aFontsForWeights[], PRUint32 aPosTraitsMask, 
                                PRUint32 aNegTraitsMask);

    PRBool FindWeightsForStyle(gfxFontEntry* aFontsForWeights[], const gfxFontStyle& aFontStyle);

    nsTArray<nsRefPtr<MacOSFontEntry> >  mAvailableFonts;
    PRPackedBool mOtherFamilyNamesInitialized;
    PRPackedBool mHasOtherFamilyNames;
};


class SingleFaceFamily : public MacOSFamilyEntry
{
public:
    SingleFaceFamily(nsAString &aName) :
        MacOSFamilyEntry(aName)
    {}
    
    virtual ~SingleFaceFamily() {}
    
    virtual void LocalizedName(nsAString& aLocalizedName);
    
    
    virtual void ReadOtherFamilyNames(AddOtherFamilyNameFunctor& aOtherFamilyFunctor);
};

class gfxQuartzFontCache : private gfxFontInfoLoader {
public:
    static gfxQuartzFontCache* SharedFontCache() {
        return sSharedFontCache;
    }

    static nsresult Init() {
        NS_ASSERTION(!sSharedFontCache, "What's this doing here?");
        sSharedFontCache = new gfxQuartzFontCache();
        return sSharedFontCache ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
    }
    
    
    static void Shutdown() {
        delete sSharedFontCache;
        sSharedFontCache = nsnull;
    }

    
    
    void GetFontList (const nsACString& aLangGroup,
                      const nsACString& aGenericFamily,
                      nsTArray<nsString>& aListOfFonts);
    PRBool ResolveFontName(const nsAString& aFontName,
                           nsAString& aResolvedFontName);
    PRBool GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName);
    void UpdateFontList() { InitFontList(); }

    void GetFontFamilyList(nsTArray<nsRefPtr<MacOSFamilyEntry> >& aFamilyArray);

    MacOSFontEntry* FindFontForChar(const PRUint32 aCh, gfxFont *aPrevFont);

    MacOSFamilyEntry* FindFamily(const nsAString& aFamily);
    
    MacOSFontEntry* FindFontForFamily(const nsAString& aFamily, const gfxFontStyle* aStyle, PRBool& aNeedsBold);
    
    MacOSFontEntry* GetDefaultFont(const gfxFontStyle* aStyle, PRBool& aNeedsBold);

    static PRInt32 AppleWeightToCSSWeight(PRInt32 aAppleWeight);
    
    PRBool GetPrefFontFamilyEntries(eFontPrefLang aLangGroup, nsTArray<nsRefPtr<MacOSFamilyEntry> > *array);
    void SetPrefFontFamilyEntries(eFontPrefLang aLangGroup, nsTArray<nsRefPtr<MacOSFamilyEntry> >& array);
    
    void AddOtherFamilyName(MacOSFamilyEntry *aFamilyEntry, nsAString& aOtherFamilyName);

    gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                  const nsAString& aFontName);
    
    gfxFontEntry* MakePlatformFont(const gfxFontEntry *aProxyEntry, const PRUint8 *aFontData, PRUint32 aLength);

private:
    static PLDHashOperator FindFontForCharProc(nsStringHashKey::KeyType aKey,
                                               nsRefPtr<MacOSFamilyEntry>& aFamilyEntry,
                                               void* userArg);

    static gfxQuartzFontCache *sSharedFontCache;

    gfxQuartzFontCache();

    
    void InitFontList();
    void ReadOtherFamilyNamesForFamily(const nsAString& aFamilyName);
    
    
    void InitOtherFamilyNames();
    
    
    void InitSingleFaceList();
    
    
    void PreloadNamesList();

    
    void InitBadUnderlineList();

    
    void EliminateDuplicateFaces(const nsAString& aFamilyName);
                                                             
    
    void SetFixedPitch(const nsAString& aFamilyName);
                                                             
    static PLDHashOperator InitOtherFamilyNamesProc(nsStringHashKey::KeyType aKey,
                                                    nsRefPtr<MacOSFamilyEntry>& aFamilyEntry,
                                                    void* userArg);

    void GenerateFontListKey(const nsAString& aKeyName, nsAString& aResult);
    static void ATSNotification(ATSFontNotificationInfoRef aInfo, void* aUserArg);
    static int PrefChangedCallback(const char *aPrefName, void *closure);

    static PLDHashOperator
        HashEnumFuncForFamilies(nsStringHashKey::KeyType aKey,
                                nsRefPtr<MacOSFamilyEntry>& aFamilyEntry,
                                void* aUserArg);

    
    virtual void InitLoader();
    virtual PRBool RunLoader();
    virtual void FinishLoader();

    
    nsRefPtrHashtable<nsStringHashKey, MacOSFamilyEntry> mFontFamilies;    

    
    
    nsRefPtrHashtable<nsStringHashKey, MacOSFamilyEntry> mOtherFamilyNames;    

    
    
    nsDataHashtable<nsUint32HashKey, nsTArray<nsRefPtr<MacOSFamilyEntry> > > mPrefFonts;

    
    gfxSparseBitSet mCodepointsWithNoFonts;

    
    PRPackedBool mOtherFamilyNamesInitialized;

    
    nsTArray<nsRefPtr<MacOSFamilyEntry> > mFontFamiliesToLoad;
    PRUint32 mStartIndex;
    PRUint32 mIncrement;
    PRUint32 mNumFamilies;
    
    
    PRUint32 mATSGeneration;
    
    enum {
        kATSGenerationInitial = -1
    };
};

#endif 
