




































#ifndef GFX_TEXT_RUN_WORD_CACHE_H
#define GFX_TEXT_RUN_WORD_CACHE_H

#include "gfxFont.h"





class THEBES_API gfxTextRunWordCache {
public:
    gfxTextRunWordCache() {
        mCache.Init(100);
    }
    ~gfxTextRunWordCache() {
        NS_ASSERTION(mCache.Count() == 0, "Textrun cache not empty!");
    }

    






    gfxTextRun *MakeTextRun(const PRUnichar *aText, PRUint32 aLength,
                            gfxFontGroup *aFontGroup,
                            const gfxFontGroup::Parameters *aParams,
                            PRUint32 aFlags, PRBool *aIsInCache);
    






    gfxTextRun *MakeTextRun(const PRUint8 *aText, PRUint32 aLength,
                            gfxFontGroup *aFontGroup,
                            const gfxFontGroup::Parameters *aParams,
                            PRUint32 aFlags, PRBool *aIsInCache);

    



    void RemoveTextRun(gfxTextRun *aTextRun);

protected:
    struct THEBES_API CacheHashKey {
        void        *mFontOrGroup;
        const void  *mString;
        PRUint32     mLength;
        PRUint32     mAppUnitsPerDevUnit;
        PRUint32     mStringHash;
        PRPackedBool mIsDoubleByteText;
    };

    class THEBES_API CacheHashEntry : public PLDHashEntryHdr {
    public:
        typedef const CacheHashKey &KeyType;
        typedef const CacheHashKey *KeyTypePointer;

        
        
        CacheHashEntry(KeyTypePointer aKey) : mTextRun(nsnull), mWordOffset(0),
            mHashedByFont(PR_FALSE) { }
        CacheHashEntry(const CacheHashEntry& toCopy) { NS_ERROR("Should not be called"); }
        ~CacheHashEntry() { }

        PRBool KeyEquals(const KeyTypePointer aKey) const;
        static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
        static PLDHashNumber HashKey(const KeyTypePointer aKey);
        enum { ALLOW_MEMMOVE = PR_TRUE };

        gfxTextRun *mTextRun;
        
        
        
        PRUint32    mWordOffset:31;
        
        
        
        PRUint32    mHashedByFont:1;
    };
    
    
    
    struct DeferredWord {
        gfxTextRun *mSourceTextRun;
        PRUint32    mSourceOffset;
        PRUint32    mDestOffset;
        PRUint32    mLength;
        PRUint32    mHash;
    };
    
    PRBool LookupWord(gfxTextRun *aTextRun, gfxFont *aFirstFont,
                      PRUint32 aStart, PRUint32 aEnd, PRUint32 aHash,
                      nsTArray<DeferredWord>* aDeferredWords);
    void FinishTextRun(gfxTextRun *aTextRun, gfxTextRun *aNewRun,
                       gfxContext *aContext,
                       const nsTArray<DeferredWord>& aDeferredWords,
                       PRBool aSuccessful);
    void RemoveWord(gfxTextRun *aTextRun, PRUint32 aStart,
                    PRUint32 aEnd, PRUint32 aHash);    

    nsTHashtable<CacheHashEntry> mCache;
};

#endif 
