









#include "uhash.h"
#include "unifiedcache.h"
#include "umutex.h"
#include "mutex.h"
#include "uassert.h"
#include "ucln_cmn.h"

static icu::UnifiedCache *gCache = NULL;
static icu::SharedObject *gNoValue = NULL;
static UMutex gCacheMutex = U_MUTEX_INITIALIZER;
static UConditionVar gInProgressValueAddedCond = U_CONDITION_INITIALIZER;
static icu::UInitOnce gCacheInitOnce = U_INITONCE_INITIALIZER;

U_CDECL_BEGIN
static UBool U_CALLCONV unifiedcache_cleanup() {
    gCacheInitOnce.reset();
    if (gCache) {
        delete gCache;
        gCache = NULL;
    }
    if (gNoValue) {
        delete gNoValue;
        gNoValue = NULL;
    }
    return TRUE;
}
U_CDECL_END


U_NAMESPACE_BEGIN

U_CAPI int32_t U_EXPORT2
ucache_hashKeys(const UHashTok key) {
    const CacheKeyBase *ckey = (const CacheKeyBase *) key.pointer;
    return ckey->hashCode();
}

U_CAPI UBool U_EXPORT2
ucache_compareKeys(const UHashTok key1, const UHashTok key2) {
    const CacheKeyBase *p1 = (const CacheKeyBase *) key1.pointer;
    const CacheKeyBase *p2 = (const CacheKeyBase *) key2.pointer;
    return *p1 == *p2;
}

U_CAPI void U_EXPORT2
ucache_deleteKey(void *obj) {
    CacheKeyBase *p = (CacheKeyBase *) obj;
    delete p;
}

CacheKeyBase::~CacheKeyBase() {
}

static void U_CALLCONV cacheInit(UErrorCode &status) {
    U_ASSERT(gCache == NULL);
    ucln_common_registerCleanup(
            UCLN_COMMON_UNIFIED_CACHE, unifiedcache_cleanup);

    
    
    gNoValue = new SharedObject();
    gCache = new UnifiedCache(status);
    if (gCache == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    if (U_FAILURE(status)) {
        delete gCache;
        delete gNoValue;
        gCache = NULL;
        gNoValue = NULL;
        return;
    }
    
    
    gNoValue->addSoftRef();
}

const UnifiedCache *UnifiedCache::getInstance(UErrorCode &status) {
    umtx_initOnce(gCacheInitOnce, &cacheInit, status);
    if (U_FAILURE(status)) {
        return NULL;
    }
    U_ASSERT(gCache != NULL);
    return gCache;
}

UnifiedCache::UnifiedCache(UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    U_ASSERT(gNoValue != NULL);
    fHashtable = uhash_open(
            &ucache_hashKeys,
            &ucache_compareKeys,
            NULL,
            &status);
    if (U_FAILURE(status)) {
        return;
    }
    uhash_setKeyDeleter(fHashtable, &ucache_deleteKey);
}

int32_t UnifiedCache::keyCount() const {
    Mutex lock(&gCacheMutex);
    return uhash_count(fHashtable);
}

void UnifiedCache::flush() const {
    Mutex lock(&gCacheMutex);

    
    
    
    while (_flush(FALSE));
    umtx_condBroadcast(&gInProgressValueAddedCond);
}

#ifdef UNIFIED_CACHE_DEBUG
#include <stdio.h>

void UnifiedCache::dump() {
    UErrorCode status = U_ZERO_ERROR;
    const UnifiedCache *cache = getInstance(status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "Unified Cache: Error fetching cache.\n");
        return;
    }
    cache->dumpContents();
}

void UnifiedCache::dumpContents() const {
    Mutex lock(&gCacheMutex);
    _dumpContents();
}




void UnifiedCache::_dumpContents() const {
    int32_t pos = UHASH_FIRST;
    const UHashElement *element = uhash_nextElement(fHashtable, &pos);
    char buffer[256];
    int32_t cnt = 0;
    for (; element != NULL; element = uhash_nextElement(fHashtable, &pos)) {
        const SharedObject *sharedObject =
                (const SharedObject *) element->value.pointer;
        const CacheKeyBase *key =
                (const CacheKeyBase *) element->key.pointer;
        if (!sharedObject->allSoftReferences()) {
            ++cnt;
            fprintf(
                    stderr,
                    "Unified Cache: Key '%s', error %d, value %p, total refcount %d, soft refcount %d\n", 
                    key->writeDescription(buffer, 256),
                    key->creationStatus,
                    sharedObject == gNoValue ? NULL :sharedObject,
                    sharedObject->getRefCount(),
                    sharedObject->getSoftRefCount());
        }
    }
    fprintf(stderr, "Unified Cache: %d out of a total of %d still have hard references\n", cnt, uhash_count(fHashtable));
}
#endif

UnifiedCache::~UnifiedCache() {
    
    flush();
    {
        
        
        
        Mutex lock(&gCacheMutex);
        _flush(TRUE);
    }
    uhash_close(fHashtable);
}







UBool UnifiedCache::_flush(UBool all) const {
    UBool result = FALSE;
    int32_t pos = UHASH_FIRST;
    const UHashElement *element = uhash_nextElement(fHashtable, &pos);
    for (; element != NULL; element = uhash_nextElement(fHashtable, &pos)) {
        const SharedObject *sharedObject =
                (const SharedObject *) element->value.pointer;
        if (all || sharedObject->allSoftReferences()) {
            uhash_removeElement(fHashtable, element);
            sharedObject->removeSoftRef();
            result = TRUE;
        }
    }
    return result;
}





void UnifiedCache::_putNew(
        const CacheKeyBase &key, 
        const SharedObject *value,
        const UErrorCode creationStatus,
        UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return;
    }
    CacheKeyBase *keyToAdopt = key.clone();
    if (keyToAdopt == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    keyToAdopt->creationStatus = creationStatus;
    uhash_put(fHashtable, keyToAdopt, (void *) value, &status);
    if (U_SUCCESS(status)) {
        value->addSoftRef();
    }
}










void UnifiedCache::_putIfAbsentAndGet(
        const CacheKeyBase &key,
        const SharedObject *&value,
        UErrorCode &status) const {
    Mutex lock(&gCacheMutex);
    const UHashElement *element = uhash_find(fHashtable, &key);
    if (element != NULL && !_inProgress(element)) {
        _fetch(element, value, status);
        return;
    }
    if (element == NULL) {
        UErrorCode putError = U_ZERO_ERROR;
        
        _putNew(key, value, status, putError);
        return;
    }
    _put(element, value, status);
}










UBool UnifiedCache::_poll(
        const CacheKeyBase &key,
        const SharedObject *&value,
        UErrorCode &status) const {
    U_ASSERT(value == NULL);
    U_ASSERT(status == U_ZERO_ERROR);
    Mutex lock(&gCacheMutex);
    const UHashElement *element = uhash_find(fHashtable, &key);
    while (element != NULL && _inProgress(element)) {
        umtx_condWait(&gInProgressValueAddedCond, &gCacheMutex);
        element = uhash_find(fHashtable, &key);
    }
    if (element != NULL) {
        _fetch(element, value, status);
        return TRUE;
    }
    _putNew(key, gNoValue, U_ZERO_ERROR, status);
    return FALSE;
}









void UnifiedCache::_get(
        const CacheKeyBase &key,
        const SharedObject *&value,
        const void *creationContext,
        UErrorCode &status) const {
    U_ASSERT(value == NULL);
    U_ASSERT(status == U_ZERO_ERROR);
    if (_poll(key, value, status)) {
        if (value == gNoValue) {
            SharedObject::clearPtr(value);
        }
        return;
    }
    if (U_FAILURE(status)) {
        return;
    }
    value = key.createObject(creationContext, status);
    U_ASSERT(value == NULL || !value->allSoftReferences());
    U_ASSERT(value != NULL || status != U_ZERO_ERROR);
    if (value == NULL) {
        SharedObject::copyPtr(gNoValue, value);
    }
    _putIfAbsentAndGet(key, value, status);
    if (value == gNoValue) {
        SharedObject::clearPtr(value);
    }
}







void UnifiedCache::_put(
        const UHashElement *element, 
        const SharedObject *value,
        const UErrorCode status) {
    U_ASSERT(_inProgress(element));
    const CacheKeyBase *theKey = (const CacheKeyBase *) element->key.pointer;
    const SharedObject *oldValue = (const SharedObject *) element->value.pointer;
    theKey->creationStatus = status;
    value->addSoftRef();
    UHashElement *ptr = const_cast<UHashElement *>(element);
    ptr->value.pointer = (void *) value;
    oldValue->removeSoftRef();

    
    
    umtx_condBroadcast(&gInProgressValueAddedCond);
}








void UnifiedCache::_fetch(
        const UHashElement *element,
        const SharedObject *&value,
        UErrorCode &status) {
    const CacheKeyBase *theKey = (const CacheKeyBase *) element->key.pointer;
    status = theKey->creationStatus;
    SharedObject::copyPtr(
            (const SharedObject *) element->value.pointer, value);
}
    


UBool UnifiedCache::_inProgress(const UHashElement *element) {
    const SharedObject *value = NULL;
    UErrorCode status = U_ZERO_ERROR;
    _fetch(element, value, status);
    UBool result = (value == gNoValue && status == U_ZERO_ERROR);
    SharedObject::clearPtr(value);
    return result;
}

U_NAMESPACE_END
