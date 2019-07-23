





































#ifndef GFXQUARTZFONTCACHE_H_
#define GFXQUARTZFONTCACHE_H_

#include "nsDataHashtable.h"

#include "gfxAtsuiFonts.h"

#include "nsUnicharUtils.h"
#include "nsVoidArray.h"

class NSFontManager;
class NSString;
class NSFont;

class FamilyEntry
{
public:
    THEBES_INLINE_DECL_REFCOUNTING(FamilyEntry)

    FamilyEntry(nsString &aName) :
        mName(aName)
    {
    }

    const nsString& Name() { return mName; }
protected:
    nsString mName;
    
};

class FontEntry
{
public:
    THEBES_INLINE_DECL_REFCOUNTING(FontEntry)

    FontEntry(nsString &aName) :
        mName(aName), mWeight(0), mTraits(0)
    {
    }

    const nsString& Name() { return mName; }
    PRInt32 Weight() {
        if (!mWeight)
            RealizeWeightAndTraits();
        return mWeight;
    }
    PRUint32 Traits() {
        if (!mWeight)
            RealizeWeightAndTraits();
        return mTraits;
    }
    PRBool IsFixedPitch();
    PRBool IsItalicStyle();
    PRBool IsBold();
    NSFont* GetNSFont(float aSize);

protected:
    void RealizeWeightAndTraits();
    void GetStringForNSString(const NSString *aSrc, nsAString& aDist);
    NSString* GetNSStringForString(const nsAString& aSrc);

    nsString mName;
    PRInt32 mWeight;
    PRUint32 mTraits;
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

    ATSUFontID FindATSUFontIDForFamilyAndStyle (const nsAString& aFamily,
                                                const gfxFontStyle* aStyle);

    ATSUFontID GetDefaultATSUFontID (const gfxFontStyle* aStyle);

    void GetFontList (const nsACString& aLangGroup,
                      const nsACString& aGenericFamily,
                      nsStringArray& aListOfFonts);
    PRBool ResolveFontName(const nsAString& aFontName,
                           nsAString& aResolvedFontName);
    void UpdateFontList() { InitFontList(); }

    const nsString& GetPostscriptNameForFontID(ATSUFontID fid);

private:
    static gfxQuartzFontCache *sSharedFontCache;

    gfxQuartzFontCache();

    void InitFontList();
    PRBool AppendFontFamily(NSFontManager *aFontManager,
                            NSString *aName, PRBool aNameIsPostscriptName);
    NSFont* FindFontWeight(NSFontManager *aFontManager,
                           FontEntry *aOriginalFont,
                           NSFont *aFont,
                           const gfxFontStyle *aStyle);
    NSFont* FindAnotherWeightMemberFont(NSFontManager *aFontManager,
                                        FontEntry *aOriginalFont,
                                        NSFont *aFont,
                                        const gfxFontStyle *aStyle,
                                        PRBool aBolder);
    void GenerateFontListKey(const nsAString& aKeyName, nsAString& aResult);
    static void ATSNotification(ATSFontNotificationInfoRef aInfo,
                                void* aUserArg);

    static PLDHashOperator PR_CALLBACK
        HashEnumFuncForFamilies(nsStringHashKey::KeyType aKey,
                                nsRefPtr<FamilyEntry>& aFamilyEntry,
                                void* aUserArg);

    ATSUFontID FindFromSystem (const nsAString& aFamily,
                               const gfxFontStyle* aStyle);

    struct FontAndFamilyContainer {
        FontAndFamilyContainer (const nsAString& family, const gfxFontStyle& style)
            : mFamily(family), mStyle(style)
        {
            ToLowerCase(mFamily);
        }

        FontAndFamilyContainer (const FontAndFamilyContainer& other)
            : mFamily(other.mFamily), mStyle(other.mStyle)
        { }

        nsString mFamily;
        gfxFontStyle mStyle;
    };

    struct FontAndFamilyKey : public PLDHashEntryHdr {
        typedef const FontAndFamilyContainer& KeyType;
        typedef const FontAndFamilyContainer* KeyTypePointer;

        FontAndFamilyKey(KeyTypePointer aObj) : mObj(*aObj) { }
        FontAndFamilyKey(const FontAndFamilyKey& other) : mObj(other.mObj) { }
        ~FontAndFamilyKey() { }

        KeyType GetKey() const { return mObj; }

        PRBool KeyEquals(KeyTypePointer aKey) const {
            return
                aKey->mFamily.Equals(mObj.mFamily) &&
                aKey->mStyle.Equals(mObj.mStyle);
        }

        static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
        static PLDHashNumber HashKey(KeyTypePointer aKey) {
            return HashString(aKey->mFamily);
        }
        enum { ALLOW_MEMMOVE = PR_FALSE };
    private:
        const FontAndFamilyContainer mObj;
    };

    nsDataHashtable<FontAndFamilyKey, ATSUFontID> mCache;
    
    nsDataHashtable<nsStringHashKey, nsRefPtr<FamilyEntry> > mFamilies;
    
    nsDataHashtable<nsStringHashKey, nsRefPtr<FontEntry> > mPostscriptFonts;
    
    nsDataHashtable<nsUint32HashKey, nsRefPtr<FontEntry> > mFontIDTable;
    
    
    
    
    
    nsDataHashtable<nsStringHashKey, nsString> mAppleFamilyNames;
    
    
    
    nsDataHashtable<nsStringHashKey, nsString> mAllFamilyNames;
    
    
    
    nsDataHashtable<nsStringHashKey, nsString> mAllFontNames;
    
    
    
    
    
    

    
    nsStringArray mNonExistingFonts;
};

#endif 
