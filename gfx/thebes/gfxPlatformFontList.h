




































#ifndef GFXPLATFORMFONTLIST_H_
#define GFXPLATFORMFONTLIST_H_

#include "nsDataHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsHashSets.h"

#include "gfxFontUtils.h"
#include "gfxFont.h"
#include "gfxPlatform.h"

#include "mozilla/FunctionTimer.h"









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

    virtual PRBool ResolveFontName(const nsAString& aFontName,
                                   nsAString& aResolvedFontName);

    void UpdateFontList() { InitFontList(); }

    void ClearPrefFonts() { mPrefFonts.Clear(); }

    virtual void GetFontFamilyList(nsTArray<nsRefPtr<gfxFontFamily> >& aFamilyArray);

    gfxFontEntry* FindFontForChar(const PRUint32 aCh, gfxFont *aPrevFont);

    
    virtual gfxFontFamily* FindFamily(const nsAString& aFamily);

    gfxFontEntry* FindFontForFamily(const nsAString& aFamily, const gfxFontStyle* aStyle, PRBool& aNeedsBold);

    PRBool GetPrefFontFamilyEntries(eFontPrefLang aLangGroup, nsTArray<nsRefPtr<gfxFontFamily> > *array);
    void SetPrefFontFamilyEntries(eFontPrefLang aLangGroup, nsTArray<nsRefPtr<gfxFontFamily> >& array);

    

    void AddOtherFamilyName(gfxFontFamily *aFamilyEntry, nsAString& aOtherFamilyName);

    void AddFullname(gfxFontEntry *aFontEntry, nsAString& aFullname);

    void AddPostscriptName(gfxFontEntry *aFontEntry, nsAString& aPostscriptName);

    PRBool NeedFullnamePostscriptNames() { return mNeedFullnamePostscriptNames; }

    

    
    virtual gfxFontEntry* GetDefaultFont(const gfxFontStyle* aStyle,
                                         PRBool& aNeedsBold) = 0;

    
    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName) = 0;

    
    
    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const PRUint8 *aFontData,
                                           PRUint32 aLength) = 0;

    
    
    virtual PRBool GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName);

protected:
    gfxPlatformFontList(PRBool aNeedFullnamePostscriptNames = PR_TRUE);

    static gfxPlatformFontList *sPlatformFontList;

    static PLDHashOperator FindFontForCharProc(nsStringHashKey::KeyType aKey,
                                               nsRefPtr<gfxFontFamily>& aFamilyEntry,
                                               void* userArg);

    
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
    virtual PRBool RunLoader();
    virtual void FinishLoader();

    
    nsRefPtrHashtable<nsStringHashKey, gfxFontFamily> mFontFamilies;

    
    
    nsRefPtrHashtable<nsStringHashKey, gfxFontFamily> mOtherFamilyNames;

    
    PRPackedBool mOtherFamilyNamesInitialized;

    
    PRPackedBool mFaceNamesInitialized;

    
    PRPackedBool mNeedFullnamePostscriptNames;

    
    nsRefPtrHashtable<nsStringHashKey, gfxFontEntry> mFullnames;

    
    nsRefPtrHashtable<nsStringHashKey, gfxFontEntry> mPostscriptNames;

    
    
    nsDataHashtable<nsUint32HashKey, nsTArray<nsRefPtr<gfxFontFamily> > > mPrefFonts;

    
    gfxSparseBitSet mCodepointsWithNoFonts;

    
    
    nsString mReplacementCharFallbackFamily;

    nsStringHashSet mBadUnderlineFamilyNames;

    
    nsTArray<nsRefPtr<gfxFontFamily> > mFontFamiliesToLoad;
    PRUint32 mStartIndex;
    PRUint32 mIncrement;
    PRUint32 mNumFamilies;
};

#endif 
