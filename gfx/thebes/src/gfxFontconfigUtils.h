





































#ifndef GFX_FONTCONFIG_UTILS_H
#define GFX_FONTCONFIG_UTILS_H

#include "gfxPlatform.h"

#include "nsAutoRef.h"
#include "nsTArray.h"
#include "nsTHashtable.h"

#include <fontconfig/fontconfig.h>


NS_SPECIALIZE_TEMPLATE
class nsAutoRefTraits<FcPattern> : public nsPointerRefTraits<FcPattern>
{
public:
    static void Release(FcPattern *ptr) { FcPatternDestroy(ptr); }
    static void AddRef(FcPattern *ptr) { FcPatternReference(ptr); }
};

NS_SPECIALIZE_TEMPLATE
class nsAutoRefTraits<FcFontSet> : public nsPointerRefTraits<FcFontSet>
{
public:
    static void Release(FcFontSet *ptr) { FcFontSetDestroy(ptr); }
};

NS_SPECIALIZE_TEMPLATE
class nsAutoRefTraits<FcCharSet> : public nsPointerRefTraits<FcCharSet>
{
public:
    static void Release(FcCharSet *ptr) { FcCharSetDestroy(ptr); }
};

class gfxIgnoreCaseCStringComparator
{
  public:
    PRBool Equals(const nsACString& a, const nsACString& b) const
    {
      return nsCString(a).Equals(b, nsCaseInsensitiveCStringComparator());
    }

    PRBool LessThan(const nsACString& a, const nsACString& b) const
    { 
      return a < b;
    }
};

class gfxFontNameList : public nsTArray<nsString>
{
public:
    THEBES_INLINE_DECL_REFCOUNTING(gfxFontNameList)
    PRBool Exists(nsAString& aName);
};

class gfxFontconfigUtils {
public:
    gfxFontconfigUtils();

    static gfxFontconfigUtils* GetFontconfigUtils() {
        if (!sUtils)
            sUtils = new gfxFontconfigUtils();
        return sUtils;
    }

    static void Shutdown();

    nsresult GetFontList(const nsACString& aLangGroup,
                         const nsACString& aGenericFamily,
                         nsTArray<nsString>& aListOfFonts);

    nsresult UpdateFontList();

    nsresult ResolveFontName(const nsAString& aFontName,
                             gfxPlatform::FontResolverCallback aCallback,
                             void *aClosure, PRBool& aAborted);

    nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName);

    const nsTArray< nsCountedRef<FcPattern> >&
    GetFontsForFamily(const FcChar8 *aFamilyName);

    const nsTArray< nsCountedRef<FcPattern> >&
    GetFontsForFullname(const FcChar8 *aFullname);

    
    FcLangResult GetBestLangSupport(const FcChar8 *aLang);
    
    const nsTArray< nsCountedRef<FcPattern> >&
    GetFontsForLang(const FcChar8 *aLang);

    
    static FcLangResult GetLangSupport(FcPattern *aFont, const FcChar8 *aLang);

    
    
    static const FcChar8 *ToFcChar8(const char *aCharPtr)
    {
        return reinterpret_cast<const FcChar8*>(aCharPtr);
    }
    static const FcChar8 *ToFcChar8(const nsCString& aCString)
    {
        return ToFcChar8(aCString.get());
    }
    static const char *ToCString(const FcChar8 *aChar8Ptr)
    {
        return reinterpret_cast<const char*>(aChar8Ptr);
    }

    static PRUint8 FcSlantToThebesStyle(int aFcSlant);
    static PRUint8 GetThebesStyle(FcPattern *aPattern); 
    static PRUint16 GetThebesWeight(FcPattern *aPattern);

    static int GetFcSlant(const gfxFontStyle& aFontStyle);
    
    
    static int FcWeightForBaseWeight(PRInt8 aBaseWeight);

    static PRBool GetFullnameFromFamilyAndStyle(FcPattern *aFont,
                                                nsACString *aFullname);

    
    
    static nsReturnRef<FcPattern>
    NewPattern(const nsTArray<nsString>& aFamilies,
               const gfxFontStyle& aFontStyle, const char *aLang);

    




    static void GetSampleLangForGroup(const nsACString& aLangGroup,
                                      nsACString *aFcLang);

protected:
    
    
    class FcStrEntryBase : public PLDHashEntryHdr {
    public:
        typedef const FcChar8 *KeyType;
        typedef const FcChar8 *KeyTypePointer;

        static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }
        
        
        
        
        
        
        
        
        
        
        static PLDHashNumber HashKey(const FcChar8 *aKey) {
            PRUint32 hash = 0;
            for (const FcChar8 *c = aKey; *c != '\0'; ++c) {
                hash = PR_ROTATE_LEFT32(hash, 3) ^ FcToLower(*c);
            }
            return hash;
        }
        enum { ALLOW_MEMMOVE = PR_TRUE };
    };

public:
    
    
    
    
    
    class DepFcStrEntry : public FcStrEntryBase {
    public:
        
        
        
        
        DepFcStrEntry(KeyTypePointer aName)
            : mKey(NULL) { }

        DepFcStrEntry(const DepFcStrEntry& toCopy)
            : mKey(toCopy.mKey) { }

        PRBool KeyEquals(KeyTypePointer aKey) const {
            return FcStrCmpIgnoreCase(aKey, mKey) == 0;
        }

        const FcChar8 *mKey;
    };

    
    
    
    class CopiedFcStrEntry : public FcStrEntryBase {
    public:
        
        
        
        
        CopiedFcStrEntry(KeyTypePointer aName) {
            mKey.SetIsVoid(PR_TRUE);
        }

        CopiedFcStrEntry(const CopiedFcStrEntry& toCopy)
            : mKey(toCopy.mKey) { }

        PRBool KeyEquals(KeyTypePointer aKey) const {
            return FcStrCmpIgnoreCase(aKey, ToFcChar8(mKey)) == 0;
        }

        PRBool IsKeyInitialized() { return !mKey.IsVoid(); }
        void InitKey(const FcChar8* aKey) { mKey.Assign(ToCString(aKey)); }

    private:
        nsCString mKey;
    };

protected:
    class FontsByFcStrEntry : public DepFcStrEntry {
    public:
        FontsByFcStrEntry(KeyTypePointer aName)
            : DepFcStrEntry(aName) { }

        FontsByFcStrEntry(const FontsByFcStrEntry& toCopy)
            : DepFcStrEntry(toCopy), mFonts(toCopy.mFonts) { }

        PRBool AddFont(FcPattern *aFont) {
            return mFonts.AppendElement(aFont) != nsnull;
        }
        const nsTArray< nsCountedRef<FcPattern> >& GetFonts() {
            return mFonts;
        }
    private:
        nsTArray< nsCountedRef<FcPattern> > mFonts;
    };

    
    
    
    
    
    
    
    class FontsByFullnameEntry : public DepFcStrEntry {
    public:
        
        
        
        
        FontsByFullnameEntry(KeyTypePointer aName)
            : DepFcStrEntry(aName) { }

        FontsByFullnameEntry(const FontsByFullnameEntry& toCopy)
            : DepFcStrEntry(toCopy), mFonts(toCopy.mFonts) { }

        PRBool KeyEquals(KeyTypePointer aKey) const;

        PRBool AddFont(FcPattern *aFont) {
            return mFonts.AppendElement(aFont) != nsnull;
        }
        const nsTArray< nsCountedRef<FcPattern> >& GetFonts() {
            return mFonts;
        }

        
        enum { ALLOW_MEMMOVE = PR_FALSE };
    private:
        
        nsAutoTArray<nsCountedRef<FcPattern>,1> mFonts;
    };

    class LangSupportEntry : public CopiedFcStrEntry {
    public:
        LangSupportEntry(KeyTypePointer aName)
            : CopiedFcStrEntry(aName) { }

        LangSupportEntry(const LangSupportEntry& toCopy)
            : CopiedFcStrEntry(toCopy), mSupport(toCopy.mSupport) { }

        FcLangResult mSupport;
        nsTArray< nsCountedRef<FcPattern> > mFonts;
    };

    static gfxFontconfigUtils* sUtils;

    PRBool IsExistingFamily(const nsCString& aFamilyName);

    nsresult GetFontListInternal(nsTArray<nsCString>& aListOfFonts,
                                 const nsACString& aLangGroup);
    nsresult UpdateFontListInternal(PRBool aForce = PR_FALSE);

    void AddFullnameEntries();

    LangSupportEntry *GetLangSupportEntry(const FcChar8 *aLang,
                                          PRBool aWithFonts);

    
    
    nsTHashtable<FontsByFcStrEntry> mFontsByFamily;
    nsTHashtable<FontsByFullnameEntry> mFontsByFullname;
    
    
    
    nsTHashtable<LangSupportEntry> mLangSupportTable;
    const nsTArray< nsCountedRef<FcPattern> > mEmptyPatternArray;

    nsTArray<nsCString> mAliasForMultiFonts;

    FcConfig *mLastConfig;
};

#endif 
