




































#ifndef GFX_TEXT_RUN_CACHE_H
#define GFX_TEXT_RUN_CACHE_H

#include "nsRefPtrHashtable.h"
#include "nsClassHashtable.h"

#include "gfxFont.h"

#include "prtime.h"

class THEBES_API gfxTextRunCache {
public:
    



    static gfxTextRunCache* GetCache() {
        return mGlobalCache;
    }

    static nsresult Init();
    static void Shutdown();

    




    gfxTextRun *GetOrMakeTextRun (gfxContext* aContext, gfxFontGroup *aFontGroup,
                                  const char *aString, PRUint32 aLength,
                                  PRUint32 aAppUnitsPerDevUnit, PRBool aIsRTL,
                                  PRBool aEnableSpacing, PRBool *aCallerOwns);
    gfxTextRun *GetOrMakeTextRun (gfxContext* aContext, gfxFontGroup *aFontGroup,
                                  const PRUnichar *aString, PRUint32 aLength,
                                  PRUint32 aAppUnitsPerDevUnit, PRBool aIsRTL,
                                  PRBool aEnableSpacing, PRBool *aCallerOwns);

protected:
    gfxTextRunCache();

    static gfxTextRunCache *mGlobalCache;
    static PRInt32 mGlobalCacheRefCount;

    






    template<class GenericString, class RealString>
    struct FontGroupAndStringT {
        FontGroupAndStringT(gfxFontGroup *fg, const GenericString* str)
            : mFontGroup(fg), mString(str) { }

        FontGroupAndStringT(const FontGroupAndStringT<GenericString,RealString>& other)
            : mFontGroup(other.mFontGroup), mString(&mRealString)
        {
            mRealString.Assign(*other.mString);
        }

        void Realize() {
            mRealString.Assign(*mString);
            mString = &mRealString;
        }

        nsRefPtr<gfxFontGroup> mFontGroup;
        RealString mRealString;
        const GenericString* mString;
    };

    static PRUint32 HashDouble(const double d) {
        if (d == 0.0)
            return 0;
        int exponent;
        double mantissa = frexp (d, &exponent);
        return (PRUint32) (2 * fabs(mantissa) - 1);
    }

    template<class T>
    struct FontGroupAndStringHashKeyT : public PLDHashEntryHdr {
        typedef const T& KeyType;
        typedef const T* KeyTypePointer;

        FontGroupAndStringHashKeyT(KeyTypePointer aObj) : mObj(*aObj) { }
        FontGroupAndStringHashKeyT(const FontGroupAndStringHashKeyT<T>& other) : mObj(other.mObj) { }
        ~FontGroupAndStringHashKeyT() { }

        KeyType GetKey() const { return mObj; }

        PRBool KeyEquals(KeyTypePointer aKey) const {
            return
                mObj.mString->Equals(*(aKey->mString)) &&
                mObj.mFontGroup->Equals(*(aKey->mFontGroup.get()));
        }

        static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
        static PLDHashNumber HashKey(KeyTypePointer aKey) {
            PRUint32 h1 = HashString(*(aKey->mString));
            PRUint32 h2 = HashString(aKey->mFontGroup->GetFamilies());
            PRUint32 h3 = HashDouble(aKey->mFontGroup->GetStyle()->size);

            return h1 ^ h2 ^ h3;
        }
        enum { ALLOW_MEMMOVE = PR_FALSE };

    private:
        const T mObj;
    };

    struct TextRunEntry {
        TextRunEntry(gfxTextRun *tr) : textRun(tr), lastUse(PR_Now()) { }
        void Used() { lastUse = PR_Now(); }

        gfxTextRun* textRun;
        PRTime      lastUse;
        
        ~TextRunEntry() { delete textRun; }
    };

    typedef FontGroupAndStringT<nsAString, nsString> FontGroupAndString;
    typedef FontGroupAndStringT<nsACString, nsCString> FontGroupAndCString;

    typedef FontGroupAndStringHashKeyT<FontGroupAndString> FontGroupAndStringHashKey;
    typedef FontGroupAndStringHashKeyT<FontGroupAndCString> FontGroupAndCStringHashKey;

    nsClassHashtable<FontGroupAndStringHashKey, TextRunEntry> mHashTableUTF16;
    nsClassHashtable<FontGroupAndCStringHashKey, TextRunEntry> mHashTableASCII;

    void EvictUTF16();
    void EvictASCII();

    PRTime mLastUTF16Eviction;
    PRTime mLastASCIIEviction;

    static PLDHashOperator UTF16EvictEnumerator(const FontGroupAndString& key,
                                                nsAutoPtr<TextRunEntry> &value,
                                                void *closure);

    static PLDHashOperator ASCIIEvictEnumerator(const FontGroupAndCString& key,
                                                nsAutoPtr<TextRunEntry> &value,
                                                void *closure);
};


#endif 
