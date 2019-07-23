




































#ifndef GFXPLATFORMFONTLIST_H_
#define GFXPLATFORMFONTLIST_H_

#include "nsDataHashtable.h"
#include "nsRefPtrHashtable.h"

#include "gfxFontUtils.h"
#include "gfxFont.h"
#include "gfxPlatform.h"









class gfxPlatformFontList : protected gfxFontInfoLoader
{
public:
    static gfxPlatformFontList* PlatformFontList() {
        return sPlatformFontList;
    }

    static nsresult Init() {
        NS_ASSERTION(!sPlatformFontList, "What's this doing here?");
        sPlatformFontList = gfxPlatform::GetPlatform()->CreatePlatformFontList();
        if (!sPlatformFontList) return NS_ERROR_OUT_OF_MEMORY;
        sPlatformFontList->InitFontList();
        return NS_OK;
    }

    static void Shutdown() {
        delete sPlatformFontList;
        sPlatformFontList = nsnull;
    }

    void GetFontList (const nsACString& aLangGroup,
                      const nsACString& aGenericFamily,
                      nsTArray<nsString>& aListOfFonts);

    virtual PRBool ResolveFontName(const nsAString& aFontName,
                                   nsAString& aResolvedFontName);

    void UpdateFontList() { InitFontList(); }

    void ClearPrefFonts() { mPrefFonts.Clear(); }

    void GetFontFamilyList(nsTArray<nsRefPtr<gfxFontFamily> >& aFamilyArray);

    gfxFontEntry* FindFontForChar(const PRUint32 aCh, gfxFont *aPrevFont);

    gfxFontFamily* FindFamily(const nsAString& aFamily);

    gfxFontEntry* FindFontForFamily(const nsAString& aFamily, const gfxFontStyle* aStyle, PRBool& aNeedsBold);

    PRBool GetPrefFontFamilyEntries(eFontPrefLang aLangGroup, nsTArray<nsRefPtr<gfxFontFamily> > *array);
    void SetPrefFontFamilyEntries(eFontPrefLang aLangGroup, nsTArray<nsRefPtr<gfxFontFamily> >& array);

    void AddOtherFamilyName(gfxFontFamily *aFamilyEntry, nsAString& aOtherFamilyName);

    

    
    virtual gfxFontEntry* GetDefaultFont(const gfxFontStyle* aStyle,
                                         PRBool& aNeedsBold) = 0;

    
    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName) = 0;

    
    
    virtual gfxFontEntry* MakePlatformFont(const gfxFontEntry *aProxyEntry,
                                           const PRUint8 *aFontData,
                                           PRUint32 aLength) = 0;

    
    virtual PRBool GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName) = 0;

protected:
    gfxPlatformFontList();

    static gfxPlatformFontList *sPlatformFontList;

    static PLDHashOperator FindFontForCharProc(nsStringHashKey::KeyType aKey,
                                               nsRefPtr<gfxFontFamily>& aFamilyEntry,
                                               void* userArg);

    
    virtual void InitFontList() = 0;

    
    void ReadOtherFamilyNamesForFamily(const nsAString& aFamilyName);

    
    void InitOtherFamilyNames();

    
    virtual void PreloadNamesList();

    
    virtual void InitBadUnderlineList();

    
    void SetFixedPitch(const nsAString& aFamilyName);

    static PLDHashOperator InitOtherFamilyNamesProc(nsStringHashKey::KeyType aKey,
                                                    nsRefPtr<gfxFontFamily>& aFamilyEntry,
                                                    void* userArg);

    void GenerateFontListKey(const nsAString& aKeyName, nsAString& aResult);

    static PLDHashOperator
        HashEnumFuncForFamilies(nsStringHashKey::KeyType aKey,
                                nsRefPtr<gfxFontFamily>& aFamilyEntry,
                                void* aUserArg);

    
    virtual void InitLoader();
    virtual PRBool RunLoader();
    virtual void FinishLoader();

    
    nsRefPtrHashtable<nsStringHashKey, gfxFontFamily> mFontFamilies;    

    
    
    nsRefPtrHashtable<nsStringHashKey, gfxFontFamily> mOtherFamilyNames;    

    
    
    nsDataHashtable<nsUint32HashKey, nsTArray<nsRefPtr<gfxFontFamily> > > mPrefFonts;

    
    gfxSparseBitSet mCodepointsWithNoFonts;

    
    
    nsString mReplacementCharFallbackFamily;

    
    PRPackedBool mOtherFamilyNamesInitialized;

    
    nsTArray<nsRefPtr<gfxFontFamily> > mFontFamiliesToLoad;
    PRUint32 mStartIndex;
    PRUint32 mIncrement;
    PRUint32 mNumFamilies;
};



class AddOtherFamilyNameFunctor 
{
public:
    AddOtherFamilyNameFunctor(gfxPlatformFontList *aFontList) :
        mFontList(aFontList)
    {}

    void operator() (gfxFontFamily *aFamilyEntry, nsAString& aOtherName) {
        mFontList->AddOtherFamilyName(aFamilyEntry, aOtherName);
    }

    gfxPlatformFontList *mFontList;
};


#endif 
