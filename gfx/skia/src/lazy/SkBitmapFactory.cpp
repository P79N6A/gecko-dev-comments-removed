






#include "SkBitmapFactory.h"

#include "SkBitmap.h"
#include "SkData.h"
#include "SkImageCache.h"
#include "SkImagePriv.h"
#include "SkLazyPixelRef.h"

SkBitmapFactory::SkBitmapFactory(SkBitmapFactory::DecodeProc proc)
    : fDecodeProc(proc)
    , fImageCache(NULL)
    , fCacheSelector(NULL) {
    SkASSERT(fDecodeProc != NULL);
}

SkBitmapFactory::~SkBitmapFactory() {
    SkSafeUnref(fImageCache);
    SkSafeUnref(fCacheSelector);
}

void SkBitmapFactory::setImageCache(SkImageCache *cache) {
    SkRefCnt_SafeAssign(fImageCache, cache);
    if (cache != NULL) {
        SkSafeUnref(fCacheSelector);
        fCacheSelector = NULL;
    }
}

void SkBitmapFactory::setCacheSelector(CacheSelector* selector) {
    SkRefCnt_SafeAssign(fCacheSelector, selector);
    if (selector != NULL) {
        SkSafeUnref(fImageCache);
        fImageCache = NULL;
    }
}

bool SkBitmapFactory::installPixelRef(SkData* data, SkBitmap* dst) {
    if (NULL == data || 0 == data->size() || dst == NULL) {
        return false;
    }

    SkImage::Info info;
    if (!fDecodeProc(data->data(), data->size(), &info, NULL)) {
        return false;
    }

    bool isOpaque = false;
    SkBitmap::Config config = SkImageInfoToBitmapConfig(info, &isOpaque);

    Target target;
    
    
    target.fRowBytes = SkImageMinRowBytes(info);

    dst->setConfig(config, info.fWidth, info.fHeight, target.fRowBytes);
    dst->setIsOpaque(isOpaque);

    
    SkASSERT(NULL == fImageCache || NULL == fCacheSelector);

    SkImageCache* cache = NULL == fCacheSelector ? fImageCache : fCacheSelector->selectCache(info);

    if (cache != NULL) {
        
        SkAutoTUnref<SkLazyPixelRef> lazyRef(SkNEW_ARGS(SkLazyPixelRef,
                                                        (data, fDecodeProc, cache)));
        dst->setPixelRef(lazyRef);
        return true;
    } else {
        dst->allocPixels();
        target.fAddr = dst->getPixels();
        return fDecodeProc(data->data(), data->size(), &info, &target);
    }
}
