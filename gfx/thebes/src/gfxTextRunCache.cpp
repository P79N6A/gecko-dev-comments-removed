




































#include "gfxTextRunCache.h"

static inline PRBool
IsAscii(const char *aString, PRUint32 aLength)
{
    const char *end = aString + aLength;
    while (aString < end) {
        if (0x80 & *aString)
            return PR_FALSE;
        ++aString;
    }
    return PR_TRUE;
}

static inline PRBool
IsAscii(const PRUnichar *aString, PRUint32 aLength)
{
    const PRUnichar *end = aString + aLength;
    while (aString < end) {
        if (0x0080 <= *aString)
            return PR_FALSE;
        ++aString;
    }
    return PR_TRUE;
}

static inline PRBool
Is8Bit(const PRUnichar *aString, PRUint32 aLength)
{
    const PRUnichar *end = aString + aLength;
    while (aString < end) {
        if (0x0100 <= *aString)
            return PR_FALSE;
        ++aString;
    }
    return PR_TRUE;
}

gfxTextRunCache* gfxTextRunCache::mGlobalCache = nsnull;

static int gDisableCache = -1;

gfxTextRunCache::gfxTextRunCache()
{
    if (getenv("MOZ_GFX_NO_TEXT_CACHE"))
        gDisableCache = 1;
    else
        gDisableCache = 0;
        
    mHashTableUTF16.Init();
    mHashTableASCII.Init();

    mLastUTF16Eviction = mLastASCIIEviction = PR_Now();
}


nsresult
gfxTextRunCache::Init()
{
    NS_ASSERTION(!mGlobalCache, "Why do we have an mGlobalCache?");
    mGlobalCache = new gfxTextRunCache();

    if (!mGlobalCache) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_OK;
}


void
gfxTextRunCache::Shutdown()
{
    delete mGlobalCache;
    mGlobalCache = nsnull;
}    

static PRUint32
ComputeFlags(PRBool aIsRTL, PRBool aEnableSpacing)
{
    PRUint32 flags = gfxTextRunFactory::TEXT_HAS_SURROGATES;
    if (aIsRTL) {
        flags |= gfxTextRunFactory::TEXT_IS_RTL;
    }
    if (aEnableSpacing) {
        flags |= gfxTextRunFactory::TEXT_ENABLE_SPACING |
                 gfxTextRunFactory::TEXT_ABSOLUTE_SPACING |
                 gfxTextRunFactory::TEXT_ENABLE_NEGATIVE_SPACING;
    }
    return flags;
}

gfxTextRun*
gfxTextRunCache::GetOrMakeTextRun(gfxContext *aContext, gfxFontGroup *aFontGroup,
                                  const PRUnichar *aString, PRUint32 aLength,
                                  PRUint32 aAppUnitsPerDevUnit, PRBool aIsRTL,
                                  PRBool aEnableSpacing, PRBool *aCallerOwns)
{
    gfxSkipChars skipChars;
    
    gfxTextRunFactory::Parameters params = {
        aContext, nsnull, nsnull, &skipChars, nsnull, 0, aAppUnitsPerDevUnit,
        ComputeFlags(aIsRTL, aEnableSpacing)
    };
    if (IsAscii(aString, aLength))
        params.mFlags |= gfxTextRunFactory::TEXT_IS_ASCII;
    
    

    gfxTextRun *tr = nsnull;
    
    if (!gDisableCache && !aEnableSpacing) {
        
        EvictUTF16();
    
        TextRunEntry *entry;
        nsDependentSubstring keyStr(aString, aString + aLength);
        FontGroupAndString key(aFontGroup, &keyStr);

        if (mHashTableUTF16.Get(key, &entry)) {
            gfxTextRun *cachedTR = entry->textRun;
            
            
            
            if (cachedTR->GetAppUnitsPerDevUnit() == aAppUnitsPerDevUnit &&
                cachedTR->IsRightToLeft() == aIsRTL) {
                entry->Used();
                tr = cachedTR;
                tr->SetContext(aContext);
            }
        } else {
            tr = aFontGroup->MakeTextRun(aString, aLength, &params);
            entry = new TextRunEntry(tr);
            key.Realize();
            mHashTableUTF16.Put(key, entry);
        }
    }

    if (tr) {
        *aCallerOwns = PR_FALSE;
    } else {
        
        *aCallerOwns = PR_TRUE;
        tr = aFontGroup->MakeTextRun(aString, aLength, &params);
    }
    if (tr) {
        
        tr->RememberText(aString, aLength);
    }

    return tr;
}

gfxTextRun*
gfxTextRunCache::GetOrMakeTextRun(gfxContext *aContext, gfxFontGroup *aFontGroup,
                                  const char *aString, PRUint32 aLength,
                                  PRUint32 aAppUnitsPerDevUnit, PRBool aIsRTL,
                                  PRBool aEnableSpacing, PRBool *aCallerOwns)
{
    gfxSkipChars skipChars;
    
    gfxTextRunFactory::Parameters params = { 
        aContext, nsnull, nsnull, &skipChars, nsnull, 0, aAppUnitsPerDevUnit,
        ComputeFlags(aIsRTL, aEnableSpacing)
    };
    if (IsAscii(aString, aLength))
        params.mFlags |= gfxTextRunFactory::TEXT_IS_ASCII;
    else
        params.mFlags |= gfxTextRunFactory::TEXT_IS_8BIT;

    const PRUint8 *str = reinterpret_cast<const PRUint8*>(aString);

    gfxTextRun *tr = nsnull;
    
    if (!gDisableCache && !aEnableSpacing) {
        
        EvictASCII();
    
        TextRunEntry *entry;
        nsDependentCSubstring keyStr(aString, aString + aLength);
        FontGroupAndCString key(aFontGroup, &keyStr);

        if (mHashTableASCII.Get(key, &entry)) {
            gfxTextRun *cachedTR = entry->textRun;
            
            
            
            if (cachedTR->GetAppUnitsPerDevUnit() == aAppUnitsPerDevUnit &&
                cachedTR->IsRightToLeft() == aIsRTL) {
                entry->Used();
                tr = cachedTR;
                tr->SetContext(aContext);
            }
        } else {
            tr = aFontGroup->MakeTextRun(str, aLength, &params);
            entry = new TextRunEntry(tr);
            key.Realize();
            mHashTableASCII.Put(key, entry);
        }
    }

    if (tr) {
        *aCallerOwns = PR_FALSE;
    } else {
        
        *aCallerOwns = PR_TRUE;
        tr = aFontGroup->MakeTextRun(str, aLength, &params);
    }
    if (tr) {
        
        tr->RememberText(str, aLength);
    }

    return tr;
}













#define EVICT_MIN_COUNT 1000


#define EVICT_AGE 1000000

void
gfxTextRunCache::EvictUTF16()
{
    PRTime evictBarrier = PR_Now();

    if (mLastUTF16Eviction > (evictBarrier - (3*EVICT_AGE)))
        return;

    if (mHashTableUTF16.Count() < EVICT_MIN_COUNT)
        return;

    
    mLastUTF16Eviction = evictBarrier;
    evictBarrier -= EVICT_AGE;
    mHashTableUTF16.Enumerate(UTF16EvictEnumerator, &evictBarrier);
}

void
gfxTextRunCache::EvictASCII()
{
    PRTime evictBarrier = PR_Now();

    if (mLastASCIIEviction > (evictBarrier - (3*EVICT_AGE)))
        return;

    if (mHashTableASCII.Count() < EVICT_MIN_COUNT)
        return;

    
    mLastASCIIEviction = evictBarrier;
    evictBarrier -= EVICT_AGE;
    mHashTableASCII.Enumerate(ASCIIEvictEnumerator, &evictBarrier);
}

PLDHashOperator
gfxTextRunCache::UTF16EvictEnumerator(const FontGroupAndString& key,
                                      nsAutoPtr<TextRunEntry> &value,
                                      void *closure)
{
    PRTime t = *(PRTime *)closure;

    if (value->lastUse < t)
        return PL_DHASH_REMOVE;

    return PL_DHASH_NEXT;
}

PLDHashOperator
gfxTextRunCache::ASCIIEvictEnumerator(const FontGroupAndCString& key,
                                      nsAutoPtr<TextRunEntry> &value,
                                      void *closure)
{
    PRTime t = *(PRTime *)closure;

    if (value->lastUse < t)
        return PL_DHASH_REMOVE;

    return PL_DHASH_NEXT;
}
