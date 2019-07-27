




#ifndef GFX_FONTCONFIG_UTILS_H
#define GFX_FONTCONFIG_UTILS_H

#include "gfxPlatform.h"

#include "mozilla/MathAlgorithms.h"
#include "nsAutoRef.h"
#include "nsTArray.h"
#include "nsTHashtable.h"
#include "nsISupportsImpl.h"

#include <fontconfig/fontconfig.h>


template <>
class nsAutoRefTraits<FcPattern> : public nsPointerRefTraits<FcPattern>
{
public:
    static void Release(FcPattern *ptr) { FcPatternDestroy(ptr); }
    static void AddRef(FcPattern *ptr) { FcPatternReference(ptr); }
};

template <>
class nsAutoRefTraits<FcFontSet> : public nsPointerRefTraits<FcFontSet>
{
public:
    static void Release(FcFontSet *ptr) { FcFontSetDestroy(ptr); }
};

template <>
class nsAutoRefTraits<FcCharSet> : public nsPointerRefTraits<FcCharSet>
{
public:
    static void Release(FcCharSet *ptr) { FcCharSetDestroy(ptr); }
};

class gfxIgnoreCaseCStringComparator
{
  public:
    bool Equals(const nsACString& a, const nsACString& b) const
    {
      return nsCString(a).Equals(b, nsCaseInsensitiveCStringComparator());
    }

    bool LessThan(const nsACString& a, const nsACString& b) const
    { 
      return a < b;
    }
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

    nsresult GetFontList(nsIAtom *aLangGroup,
                         const nsACString& aGenericFamily,
                         nsTArray<nsString>& aListOfFonts);

    nsresult UpdateFontList();

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

    static uint8_t FcSlantToThebesStyle(int aFcSlant);
    static uint8_t GetThebesStyle(FcPattern *aPattern); 
    static uint16_t GetThebesWeight(FcPattern *aPattern);
    static int16_t GetThebesStretch(FcPattern *aPattern);

    static int GetFcSlant(const gfxFontStyle& aFontStyle);
    
    
    static int FcWeightForBaseWeight(int8_t aBaseWeight);

    static int FcWidthForThebesStretch(int16_t aStretch);

    static bool GetFullnameFromFamilyAndStyle(FcPattern *aFont,
                                                nsACString *aFullname);

    
    
    static nsReturnRef<FcPattern>
    NewPattern(const nsTArray<nsString>& aFamilies,
               const gfxFontStyle& aFontStyle, const char *aLang);

    




    static void GetSampleLangForGroup(nsIAtom *aLangGroup,
                                      nsACString *aFcLang);

protected:
    
    
    class FcStrEntryBase : public PLDHashEntryHdr {
    public:
        typedef const FcChar8 *KeyType;
        typedef const FcChar8 *KeyTypePointer;

        static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }
        
        
        
        
        
        
        
        
        
        
        static PLDHashNumber HashKey(const FcChar8 *aKey) {
            uint32_t hash = 0;
            for (const FcChar8 *c = aKey; *c != '\0'; ++c) {
                hash = mozilla::RotateLeft(hash, 3) ^ FcToLower(*c);
            }
            return hash;
        }
        enum { ALLOW_MEMMOVE = true };
    };

public:
    
    
    
    
    
    class DepFcStrEntry : public FcStrEntryBase {
    public:
        
        
        
        
        explicit DepFcStrEntry(KeyTypePointer aName)
            : mKey(nullptr) { }

        DepFcStrEntry(const DepFcStrEntry& toCopy)
            : mKey(toCopy.mKey) { }

        bool KeyEquals(KeyTypePointer aKey) const {
            return FcStrCmpIgnoreCase(aKey, mKey) == 0;
        }

        const FcChar8 *mKey;
    };

    
    
    
    class CopiedFcStrEntry : public FcStrEntryBase {
    public:
        
        
        
        
        explicit CopiedFcStrEntry(KeyTypePointer aName) {
            mKey.SetIsVoid(true);
        }

        CopiedFcStrEntry(const CopiedFcStrEntry& toCopy)
            : mKey(toCopy.mKey) { }

        bool KeyEquals(KeyTypePointer aKey) const {
            return FcStrCmpIgnoreCase(aKey, ToFcChar8(mKey)) == 0;
        }

        bool IsKeyInitialized() { return !mKey.IsVoid(); }
        void InitKey(const FcChar8* aKey) { mKey.Assign(ToCString(aKey)); }

    private:
        nsCString mKey;
    };

protected:
    class FontsByFcStrEntry : public DepFcStrEntry {
    public:
        explicit FontsByFcStrEntry(KeyTypePointer aName)
            : DepFcStrEntry(aName) { }

        FontsByFcStrEntry(const FontsByFcStrEntry& toCopy)
            : DepFcStrEntry(toCopy), mFonts(toCopy.mFonts) { }

        bool AddFont(FcPattern *aFont) {
            return mFonts.AppendElement(aFont) != nullptr;
        }
        const nsTArray< nsCountedRef<FcPattern> >& GetFonts() {
            return mFonts;
        }
    private:
        nsTArray< nsCountedRef<FcPattern> > mFonts;
    };

    
    
    
    
    
    
    
    class FontsByFullnameEntry : public DepFcStrEntry {
    public:
        
        
        
        
        explicit FontsByFullnameEntry(KeyTypePointer aName)
            : DepFcStrEntry(aName) { }

        FontsByFullnameEntry(const FontsByFullnameEntry& toCopy)
            : DepFcStrEntry(toCopy), mFonts(toCopy.mFonts) { }

        bool KeyEquals(KeyTypePointer aKey) const;

        bool AddFont(FcPattern *aFont) {
            return mFonts.AppendElement(aFont) != nullptr;
        }
        const nsTArray< nsCountedRef<FcPattern> >& GetFonts() {
            return mFonts;
        }

        
        enum { ALLOW_MEMMOVE = false };
    private:
        
        nsAutoTArray<nsCountedRef<FcPattern>,1> mFonts;
    };

    class LangSupportEntry : public CopiedFcStrEntry {
    public:
        explicit LangSupportEntry(KeyTypePointer aName)
            : CopiedFcStrEntry(aName) { }

        LangSupportEntry(const LangSupportEntry& toCopy)
            : CopiedFcStrEntry(toCopy), mSupport(toCopy.mSupport) { }

        FcLangResult mSupport;
        nsTArray< nsCountedRef<FcPattern> > mFonts;
    };

    static gfxFontconfigUtils* sUtils;

    bool IsExistingFamily(const nsCString& aFamilyName);

    nsresult GetFontListInternal(nsTArray<nsCString>& aListOfFonts,
                                 nsIAtom *aLangGroup);
    nsresult UpdateFontListInternal(bool aForce = false);

    void AddFullnameEntries();

    LangSupportEntry *GetLangSupportEntry(const FcChar8 *aLang,
                                          bool aWithFonts);

    
    
    nsTHashtable<FontsByFcStrEntry> mFontsByFamily;
    nsTHashtable<FontsByFullnameEntry> mFontsByFullname;
    
    
    
    nsTHashtable<LangSupportEntry> mLangSupportTable;
    const nsTArray< nsCountedRef<FcPattern> > mEmptyPatternArray;

    FcConfig *mLastConfig;

#ifdef MOZ_BUNDLED_FONTS
    void      ActivateBundledFonts();

    nsCString mBundledFontsPath;
    bool      mBundledFontsInitialized;
#endif
};

#endif 
