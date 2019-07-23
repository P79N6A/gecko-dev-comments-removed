




































#include "gfxTextRunCache.h"
#include "gfxTextRunWordCache.h"

#include "nsExpirationTracker.h"

enum {
    TEXTRUN_TIMEOUT_MS = 10 * 1000 
};




class TextRunExpiringCache : public nsExpirationTracker<gfxTextRun,3> {
public:
    TextRunExpiringCache()
        : nsExpirationTracker<gfxTextRun,3>(TEXTRUN_TIMEOUT_MS) {}
    ~TextRunExpiringCache() {
        AgeAllGenerations();
    }

    
    virtual void NotifyExpired(gfxTextRun *aTextRun) {
        RemoveObject(aTextRun);
        gfxTextRunWordCache::RemoveTextRun(aTextRun);
        delete aTextRun;
    }
};

static TextRunExpiringCache *gTextRunCache = nsnull;

gfxTextRun *
gfxTextRunCache::MakeTextRun(const PRUnichar *aText, PRUint32 aLength,
                             gfxFontGroup *aFontGroup,
                             const gfxTextRunFactory::Parameters* aParams,
                             PRUint32 aFlags)
{
    if (!gTextRunCache)
        return nsnull;
    return gfxTextRunWordCache::MakeTextRun(aText, aLength, aFontGroup,
        aParams, aFlags);
}

gfxTextRun *
gfxTextRunCache::MakeTextRun(const PRUnichar *aText, PRUint32 aLength,
                             gfxFontGroup *aFontGroup,
                             gfxContext *aRefContext,
                             PRUint32 aAppUnitsPerDevUnit,
                             PRUint32 aFlags)
{
    if (!gTextRunCache)
        return nsnull;
    gfxTextRunFactory::Parameters params = {
        aRefContext, nsnull, nsnull, nsnull, 0, aAppUnitsPerDevUnit
    };
    return gfxTextRunWordCache::MakeTextRun(aText, aLength, aFontGroup,
        &params, aFlags);
}

gfxTextRun *
gfxTextRunCache::MakeTextRun(const PRUint8 *aText, PRUint32 aLength,
                             gfxFontGroup *aFontGroup,
                             gfxContext *aRefContext,
                             PRUint32 aAppUnitsPerDevUnit,
                             PRUint32 aFlags)
{
    if (!gTextRunCache)
        return nsnull;
    gfxTextRunFactory::Parameters params = {
        aRefContext, nsnull, nsnull, nsnull, 0, aAppUnitsPerDevUnit
    };
    return gfxTextRunWordCache::MakeTextRun(aText, aLength, aFontGroup,
        &params, aFlags);
}

void
gfxTextRunCache::ReleaseTextRun(gfxTextRun *aTextRun)
{
    if (!aTextRun)
        return;
    if (!(aTextRun->GetFlags() & gfxTextRunWordCache::TEXT_IN_CACHE)) {
        delete aTextRun;
        return;
    }
    nsresult rv = gTextRunCache->AddObject(aTextRun);
    if (NS_FAILED(rv)) {
        delete aTextRun;
        return;
    }
}

nsresult
gfxTextRunCache::Init()
{
    gTextRunCache = new TextRunExpiringCache();
    return gTextRunCache ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

void
gfxTextRunCache::Shutdown()
{
    delete gTextRunCache;
    gTextRunCache = nsnull;
}
