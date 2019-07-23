






































#ifndef GFXQUARTZFONTCACHE_H_
#define GFXQUARTZFONTCACHE_H_

#include "nsDataHashtable.h"

#include "gfxFontUtils.h"
#include "gfxAtsuiFonts.h"
#include "gfxPlatform.h"

#include <Carbon/Carbon.h>

#include "nsUnicharUtils.h"
#include "nsVoidArray.h"


struct FontSearch {
    FontSearch(const PRUint32 aCharacter, gfxAtsuiFont *aFont) :
        ch(aCharacter), fontToMatch(aFont), matchRank(0) {
    }
    const PRUint32 ch;
    gfxAtsuiFont *fontToMatch;
    PRInt32 matchRank;
    nsRefPtr<MacOSFontEntry> bestMatch;
};

class MacOSFamilyEntry;


class MacOSFontEntry
{
public:
    THEBES_INLINE_DECL_REFCOUNTING(MacOSFontEntry)

    
    MacOSFontEntry(const nsAString& aPostscriptName, PRInt32 aAppleWeight, PRUint32 aTraits, 
                    MacOSFamilyEntry *aFamily);  

    const nsString& Name() { return mPostscriptName; }
    const nsString& FamilyName();
    PRInt32 Weight() { return mWeight; }
    PRUint32 Traits() { return mTraits; }
    
    PRBool IsFixedPitch();
    PRBool IsItalicStyle();
    PRBool IsBold();

    ATSUFontID GetFontID();                   
    nsresult ReadCMAP();
    inline PRBool TestCharacterMap(PRUint32 aCh) {
        if ( !mCmapInitialized ) ReadCMAP();
        return mCharacterMap.test(aCh);
    }
        
protected:
    nsString mPostscriptName;
    PRInt32 mWeight; 
    PRUint32 mTraits;
    MacOSFamilyEntry *mFamily;

    ATSUFontID mATSUFontID;
    std::bitset<128> mUnicodeRanges;
    gfxSparseBitSet mCharacterMap;
    
    PRPackedBool mCmapInitialized;
    PRPackedBool mATSUIDInitialized;
};


class MacOSFamilyEntry
{
public:
    THEBES_INLINE_DECL_REFCOUNTING(MacOSFamilyEntry)

    MacOSFamilyEntry(nsString &aName) :
        mName(aName)
    {
    }

    const nsString& Name() { return mName; }
    void AddFontEntry(nsRefPtr<MacOSFontEntry> aFontEntry) {
        mAvailableFonts.AppendElement(aFontEntry);
    }
    
    
    
    MacOSFontEntry* FindFont(const gfxFontStyle* aStyle);
    
    
    
    void FindFontForChar(FontSearch *aMatchData);
    
protected:
    
    
    
    
    PRBool FindFontsWithTraits(MacOSFontEntry* aFontsForWeights[], PRUint32 aPosTraitsMask, 
                                PRUint32 aNegTraitsMask);

    
    MacOSFontEntry* FindFontWeight(MacOSFontEntry* aFontsForWeights[], const gfxFontStyle* aStyle);
    
    nsString mName;  
    nsTArray<nsRefPtr<MacOSFontEntry> >  mAvailableFonts;
};


class gfxQuartzFontCache {
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
                      nsStringArray& aListOfFonts);
    PRBool ResolveFontName(const nsAString& aFontName,
                           nsAString& aResolvedFontName);
    void UpdateFontList() { InitFontList(); }


    MacOSFontEntry* FindFontForChar(const PRUint32 aCh, gfxAtsuiFont *aPrevFont);

    MacOSFamilyEntry* FindFamily(const nsAString& aFamily);
    
    MacOSFontEntry* FindFontForFamily(const nsAString& aFamily, const gfxFontStyle* aStyle);
    
    MacOSFontEntry* GetDefaultFont(const gfxFontStyle* aStyle);

    static PRInt32 AppleWeightToCSSWeight(PRInt32 aAppleWeight);
    
    PRBool GetPrefFontFamilyEntries(eFontPrefLang aLangGroup, nsTArray<nsRefPtr<MacOSFamilyEntry> > *array);
    void SetPrefFontFamilyEntries(eFontPrefLang aLangGroup, nsTArray<nsRefPtr<MacOSFamilyEntry> >& array);

private:
    static PLDHashOperator PR_CALLBACK FindFontForCharProc(nsStringHashKey::KeyType aKey,
                                                             nsRefPtr<MacOSFamilyEntry>& aFamilyEntry,
                                                             void* userArg);
    static gfxQuartzFontCache *sSharedFontCache;

    gfxQuartzFontCache();

    void InitFontList();
    
    void GenerateFontListKey(const nsAString& aKeyName, nsAString& aResult);
    static void ATSNotification(ATSFontNotificationInfoRef aInfo, void* aUserArg);
    static int PR_CALLBACK PrefChangedCallback(const char *aPrefName, void *closure);

    static PLDHashOperator PR_CALLBACK
        HashEnumFuncForFamilies(nsStringHashKey::KeyType aKey,
                                nsRefPtr<MacOSFamilyEntry>& aFamilyEntry,
                                void* aUserArg);

    
    nsDataHashtable<nsStringHashKey, nsRefPtr<MacOSFamilyEntry> > mFontFamilies;    

    
    
    nsDataHashtable<nsStringHashKey, nsRefPtr<MacOSFamilyEntry> > mLocalizedFamilies;    

    
    
    nsDataHashtable<nsUint32HashKey, nsTArray<nsRefPtr<MacOSFamilyEntry> > > mPrefFonts;

};

#endif 
