




































#ifndef GFX_TEXT_RUN_CACHE_H
#define GFX_TEXT_RUN_CACHE_H

#include "gfxFont.h"
#include "nsCheapSets.h"







class THEBES_API gfxTextRunCache {
public:
    gfxTextRunCache() {
        mCache.Init(100);
    }
    ~gfxTextRunCache() {}

    









    gfxTextRun *GetOrMakeTextRun(const PRUnichar *aText, PRUint32 aLength,
                                 gfxFontGroup *aFontGroup,
                                 const gfxFontGroup::Parameters *aParams,
                                 PRUint32 aFlags, PRBool *aCallerOwns = nsnull);
    









    gfxTextRun *GetOrMakeTextRun(const PRUint8 *aText, PRUint32 aLength,
                                 gfxFontGroup *aFontGroup,
                                 const gfxFontGroup::Parameters *aParams,
                                 PRUint32 aFlags, PRBool *aCallerOwns = nsnull);

    



    virtual void NotifyRemovedFromCache(gfxTextRun *aTextRun) {}

    



    void RemoveTextRun(gfxTextRun *aTextRun);

    
    enum { FLAG_MASK =
        gfxTextRunFactory::TEXT_IS_RTL |
        gfxTextRunFactory::TEXT_ENABLE_SPACING |
        gfxTextRunFactory::TEXT_ABSOLUTE_SPACING |
        gfxTextRunFactory::TEXT_ENABLE_NEGATIVE_SPACING |
        gfxTextRunFactory::TEXT_ENABLE_HYPHEN_BREAKS |
        gfxTextRunFactory::TEXT_NEED_BOUNDING_BOX
    };

protected:
    struct THEBES_API CacheHashKey {
        void       *mFontOrGroup;
        const void *mString;
        PRUint32    mLength;
        PRUint32    mAppUnitsPerDevUnit;
        PRUint32    mFlags;
        PRUint32    mStringHash;

        CacheHashKey(void *aFontOrGroup, const void *aString, PRUint32 aLength,
                     PRUint32 aAppUnitsPerDevUnit, PRUint32 aFlags, PRUint32 aStringHash)
            : mFontOrGroup(aFontOrGroup), mString(aString), mLength(aLength),
              mAppUnitsPerDevUnit(aAppUnitsPerDevUnit), mFlags(aFlags),
              mStringHash(aStringHash) {}
    };

    class THEBES_API CacheHashEntry : public PLDHashEntryHdr {
    public:
        typedef const CacheHashKey &KeyType;
        typedef const CacheHashKey *KeyTypePointer;

        
        
        CacheHashEntry(KeyTypePointer aKey)  { }
        CacheHashEntry(const CacheHashEntry& toCopy) { NS_ERROR("Should not be called"); }
        ~CacheHashEntry() { }

        PRBool KeyEquals(const KeyTypePointer aKey) const;
        static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
        static PLDHashNumber HashKey(const KeyTypePointer aKey);
        enum { ALLOW_MEMMOVE = PR_TRUE };

        gfxTextRun *mTextRun;
    };

    CacheHashKey GetKeyForTextRun(gfxTextRun *aTextRun);

    nsTHashtable<CacheHashEntry> mCache;
};







class THEBES_API gfxGlobalTextRunCache {
public:
    









    static gfxTextRun *GetTextRun(const PRUnichar *aText, PRUint32 aLength,
                                  gfxFontGroup *aFontGroup,
                                  gfxContext *aRefContext,
                                  PRUint32 aAppUnitsPerDevUnit,
                                  PRUint32 aFlags);

    









    static gfxTextRun *GetTextRun(const PRUint8 *aText, PRUint32 aLength,
                                  gfxFontGroup *aFontGroup,
                                  gfxContext *aRefContext,
                                  PRUint32 aAppUnitsPerDevUnit,
                                  PRUint32 aFlags);

protected:
    friend class gfxPlatform;

    static nsresult Init();
    static void Shutdown();
};

#endif 
