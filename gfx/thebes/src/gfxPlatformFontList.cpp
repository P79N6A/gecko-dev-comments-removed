

































































#include "gfxPlatformFontList.h"

#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"  
#include "nsServiceManagerUtils.h"
#include "nsUnicharUtils.h"


static const PRUint32 kDelayBeforeLoadingCmaps = 8 * 1000; 
static const PRUint32 kIntervalBetweenLoadingCmaps = 150; 
static const PRUint32 kNumFontsPerSlice = 10; 

static PRLogModuleInfo *gFontListLog = PR_NewLogModule("fontListLog");


gfxPlatformFontList *gfxPlatformFontList::sPlatformFontList = nsnull;


class gfxFontListPrefObserver : public nsIObserver {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
};

NS_IMPL_ISUPPORTS1(gfxFontListPrefObserver, nsIObserver)

NS_IMETHODIMP
gfxFontListPrefObserver::Observe(nsISupports     *aSubject,
                                 const char      *aTopic,
                                 const PRUnichar *aData)
{
    NS_ASSERTION(!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID), "invalid topic");
    
    
    gfxPlatformFontList::PlatformFontList()->ClearPrefFonts();
    return NS_OK;
}


gfxPlatformFontList::gfxPlatformFontList()
    : mStartIndex(0), mIncrement(kNumFontsPerSlice), mNumFamilies(0)
{
    mFontFamilies.Init(100);
    mOtherFamilyNames.Init(30);
    mOtherFamilyNamesInitialized = PR_FALSE;
    mPrefFonts.Init(10);

    
    gfxFontListPrefObserver *observer = new gfxFontListPrefObserver();
    if (observer) {
        nsCOMPtr<nsIPrefBranch2> pref = do_GetService(NS_PREFSERVICE_CONTRACTID);
        if (pref) {
            pref->AddObserver("font.", observer, PR_FALSE);
            pref->AddObserver("font.name-list.", observer, PR_FALSE);
            pref->AddObserver("intl.accept_languages", observer, PR_FALSE);  
        } else {
            delete observer;
        }
    }
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
    AddOtherFamilyNameFunctor addOtherNames(fc);
    aFamilyEntry->ReadOtherFamilyNames(addOtherNames);
    return PL_DHASH_NEXT;
}

void
gfxPlatformFontList::ReadOtherFamilyNamesForFamily(const nsAString& aFamilyName)
{
    gfxFontFamily *familyEntry = FindFamily(aFamilyName);

    if (familyEntry) {
        AddOtherFamilyNameFunctor addOtherNames(this);
        familyEntry->ReadOtherFamilyNames(addOtherNames);
    }
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
            AddOtherFamilyNameFunctor addOtherNames(this);
            familyEntry->ReadOtherFamilyNames(addOtherNames);
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
gfxPlatformFontList::InitBadUnderlineList()
{
    nsAutoTArray<nsString, 10> blacklist;
    gfxFontUtils::GetPrefsFontList("font.blacklist.underline_offset", blacklist);
    PRUint32 numFonts = blacklist.Length();
    for (PRUint32 i = 0; i < numFonts; i++) {
        PRBool found;
        nsAutoString key;
        GenerateFontListKey(blacklist[i], key);

        gfxFontFamily *familyEntry = mFontFamilies.GetWeak(key, &found);
        if (familyEntry)
            familyEntry->SetBadUnderlineFamily();
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
    FontListData(const nsACString& aLangGroup,
                 const nsACString& aGenericFamily,
                 nsTArray<nsString>& aListOfFonts) :
        mLangGroup(aLangGroup), mGenericFamily(aGenericFamily),
        mListOfFonts(aListOfFonts) {}
    const nsACString& mLangGroup;
    const nsACString& mGenericFamily;
    nsTArray<nsString>& mListOfFonts;
};

PLDHashOperator PR_CALLBACK
gfxPlatformFontList::HashEnumFuncForFamilies(nsStringHashKey::KeyType aKey,
                                             nsRefPtr<gfxFontFamily>& aFamilyEntry,
                                             void *aUserArg)
{
    FontListData *data = static_cast<FontListData*>(aUserArg);

    nsAutoString localizedFamilyName;
    aFamilyEntry->LocalizedName(localizedFamilyName);
    data->mListOfFonts.AppendElement(localizedFamilyName);
    return PL_DHASH_NEXT;
}

void
gfxPlatformFontList::GetFontList (const nsACString& aLangGroup,
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

gfxFontFamily* 
gfxPlatformFontList::FindFamily(const nsAString& aFamily)
{
    nsAutoString key;
    gfxFontFamily *familyEntry;
    PRBool found;
    GenerateFontListKey(aFamily, key);

    
    if ((familyEntry = mFontFamilies.GetWeak(key, &found))) {
        return familyEntry;
    }

    
    if ((familyEntry = mOtherFamilyNames.GetWeak(key, &found))) {
        return familyEntry;
    }

    
    
    
    if (!mOtherFamilyNamesInitialized) {
        InitOtherFamilyNames();
        if ((familyEntry = mOtherFamilyNames.GetWeak(key, &found))) {
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
        PR_LOG(gFontListLog, PR_LOG_DEBUG, ("(fontlist-otherfamily) canonical family: %s, other family: %s\n", 
                                            NS_ConvertUTF16toUTF8(aFamilyEntry->Name()).get(), 
                                            NS_ConvertUTF16toUTF8(aOtherFamilyName).get()));
    }
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

    
    AddOtherFamilyNameFunctor addOtherNames(this);
    for (i = mStartIndex; i < endIndex; i++) {
        gfxFontFamily* familyEntry = mFontFamiliesToLoad[i];

        
        familyEntry->FindStyleVariations();

        
        familyEntry->ReadCMAP();

        
        familyEntry->ReadOtherFamilyNames(addOtherNames);

        
        familyEntry->CheckForSimpleFamily();
    }

    mStartIndex += mIncrement;
    if (mStartIndex < mNumFamilies)
        return PR_FALSE;
    return PR_TRUE;
}

void 
gfxPlatformFontList::FinishLoader()
{
    mFontFamiliesToLoad.Clear();
    mNumFamilies = 0;
}

