




















#include "unicode/ustring.h"
#include "unicode/ucnv.h"
#include "charstr.h"
#include "uresimp.h"
#include "ustr_imp.h"
#include "cwchar.h"
#include "ucln_cmn.h"
#include "cmemory.h"
#include "cstring.h"
#include "uhash.h"
#include "unicode/uenum.h"
#include "uenumimp.h"
#include "ulocimp.h"
#include "umutex.h"
#include "putilimp.h"
#include "uassert.h"

using namespace icu;






static UHashtable *cache = NULL;
static icu::UInitOnce gCacheInitOnce;

static UMutex resbMutex = U_MUTEX_INITIALIZER;


static int32_t U_CALLCONV hashEntry(const UHashTok parm) {
    UResourceDataEntry *b = (UResourceDataEntry *)parm.pointer;
    UHashTok namekey, pathkey;
    namekey.pointer = b->fName;
    pathkey.pointer = b->fPath;
    return uhash_hashChars(namekey)+37u*uhash_hashChars(pathkey);
}


static UBool U_CALLCONV compareEntries(const UHashTok p1, const UHashTok p2) {
    UResourceDataEntry *b1 = (UResourceDataEntry *)p1.pointer;
    UResourceDataEntry *b2 = (UResourceDataEntry *)p2.pointer;
    UHashTok name1, name2, path1, path2;
    name1.pointer = b1->fName;
    name2.pointer = b2->fName;
    path1.pointer = b1->fPath;
    path2.pointer = b2->fPath;
    return (UBool)(uhash_compareChars(name1, name2) &&
        uhash_compareChars(path1, path2));
}






static UBool chopLocale(char *name) {
    char *i = uprv_strrchr(name, '_');

    if(i != NULL) {
        *i = '\0';
        return TRUE;
    }

    return FALSE;
}




static void entryIncrease(UResourceDataEntry *entry) {
    umtx_lock(&resbMutex);
    entry->fCountExisting++;
    while(entry->fParent != NULL) {
      entry = entry->fParent;
      entry->fCountExisting++;
    }
    umtx_unlock(&resbMutex);
}





static const ResourceData *getFallbackData(const UResourceBundle* resBundle, const char* * resTag, UResourceDataEntry* *realData, Resource *res, UErrorCode *status) {
    UResourceDataEntry *resB = resBundle->fData;
    int32_t indexR = -1;
    int32_t i = 0;
    *res = RES_BOGUS;
    if(resB != NULL) {
        if(resB->fBogus == U_ZERO_ERROR) { 
            *res = res_getTableItemByKey(&(resB->fData), resB->fData.rootRes, &indexR, resTag); 
            i++;
        }
        if(resBundle->fHasFallback == TRUE) {
            while(*res == RES_BOGUS && resB->fParent != NULL) { 
                resB = resB->fParent;
                if(resB->fBogus == U_ZERO_ERROR) {
                    i++;
                    *res = res_getTableItemByKey(&(resB->fData), resB->fData.rootRes, &indexR, resTag);
                }
            }
        }

        if(*res != RES_BOGUS) { 
            if(i>1) {
                if(uprv_strcmp(resB->fName, uloc_getDefault())==0 || uprv_strcmp(resB->fName, kRootLocaleName)==0) {
                    *status = U_USING_DEFAULT_WARNING;
                } else {
                    *status = U_USING_FALLBACK_WARNING;
                }
            }
            *realData = resB;
            return (&(resB->fData));
        } else { 
            *status = U_MISSING_RESOURCE_ERROR;
            return NULL;
        }
    } else {
            *status = U_MISSING_RESOURCE_ERROR;
            return NULL;
    }
}

static void
free_entry(UResourceDataEntry *entry) {
    UResourceDataEntry *alias;
    res_unload(&(entry->fData));
    if(entry->fName != NULL && entry->fName != entry->fNameBuffer) {
        uprv_free(entry->fName);
    }
    if(entry->fPath != NULL) {
        uprv_free(entry->fPath);
    }
    if(entry->fPool != NULL) {
        --entry->fPool->fCountExisting;
    }
    alias = entry->fAlias;
    if(alias != NULL) {
        while(alias->fAlias != NULL) {
            alias = alias->fAlias;
        }
        --alias->fCountExisting;
    }
    uprv_free(entry);
}


static int32_t ures_flushCache()
{
    UResourceDataEntry *resB;
    int32_t pos;
    int32_t rbDeletedNum = 0;
    const UHashElement *e;
    UBool deletedMore;

    


    umtx_lock(&resbMutex);
    if (cache == NULL) {
        umtx_unlock(&resbMutex);
        return 0;
    }

    do {
        deletedMore = FALSE;
        
        pos = UHASH_FIRST;
        while ((e = uhash_nextElement(cache, &pos)) != NULL)
        {
            resB = (UResourceDataEntry *) e->value.pointer;
            





            
            

            if (resB->fCountExisting == 0) {
                rbDeletedNum++;
                deletedMore = TRUE;
                uhash_removeElement(cache, e);
                free_entry(resB);
            }
        }
        



    } while(deletedMore);
    umtx_unlock(&resbMutex);

    return rbDeletedNum;
}

#ifdef URES_DEBUG
#include <stdio.h>

U_CAPI UBool U_EXPORT2 ures_dumpCacheContents(void) {
  UBool cacheNotEmpty = FALSE;
  int32_t pos = UHASH_FIRST;
  const UHashElement *e;
  UResourceDataEntry *resB;
  
    umtx_lock(&resbMutex);
    if (cache == NULL) {
      umtx_unlock(&resbMutex);
      fprintf(stderr,"%s:%d: RB Cache is NULL.\n", __FILE__, __LINE__);
      return FALSE;
    }

    while ((e = uhash_nextElement(cache, &pos)) != NULL) {
      cacheNotEmpty=TRUE;
      resB = (UResourceDataEntry *) e->value.pointer;
      fprintf(stderr,"%s:%d: RB Cache: Entry @0x%p, refcount %d, name %s:%s.  Pool 0x%p, alias 0x%p, parent 0x%p\n",
              __FILE__, __LINE__,
              (void*)resB, resB->fCountExisting,
              resB->fName?resB->fName:"NULL",
              resB->fPath?resB->fPath:"NULL",
              (void*)resB->fPool,
              (void*)resB->fAlias,
              (void*)resB->fParent);       
    }
    
    fprintf(stderr,"%s:%d: RB Cache still contains %d items.\n", __FILE__, __LINE__, uhash_count(cache));

    umtx_unlock(&resbMutex);
    
    return cacheNotEmpty;
}

#endif

static UBool U_CALLCONV ures_cleanup(void)
{
    if (cache != NULL) {
        ures_flushCache();
        uhash_close(cache);
        cache = NULL;
    }
    gCacheInitOnce.reset();
    return TRUE;
}


static void createCache(UErrorCode &status) {
    U_ASSERT(cache == NULL);
    cache = uhash_open(hashEntry, compareEntries, NULL, &status);
    ucln_common_registerCleanup(UCLN_COMMON_URES, ures_cleanup);
}
     
static void initCache(UErrorCode *status) {
    umtx_initOnce(gCacheInitOnce, &createCache, *status);
}



static void setEntryName(UResourceDataEntry *res, const char *name, UErrorCode *status) {
    int32_t len = (int32_t)uprv_strlen(name);
    if(res->fName != NULL && res->fName != res->fNameBuffer) {
        uprv_free(res->fName);
    }
    if (len < (int32_t)sizeof(res->fNameBuffer)) {
        res->fName = res->fNameBuffer;
    }
    else {
        res->fName = (char *)uprv_malloc(len+1);
    }
    if(res->fName == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
    } else {
        uprv_strcpy(res->fName, name);
    }
}

static UResourceDataEntry *
getPoolEntry(const char *path, UErrorCode *status);





static UResourceDataEntry *init_entry(const char *localeID, const char *path, UErrorCode *status) {
    UResourceDataEntry *r = NULL;
    UResourceDataEntry find;
    
    const char *name;
    char aliasName[100] = { 0 };
    int32_t aliasLen = 0;
    
    

    if(U_FAILURE(*status)) {
        return NULL;
    }

    
    if(localeID == NULL) { 
        name = uloc_getDefault();
    } else if(*localeID == 0) { 
        name = kRootLocaleName;
    } else { 
        name = localeID;
    }

    find.fName = (char *)name;
    find.fPath = (char *)path;

    
    
    

    
    r = (UResourceDataEntry *)uhash_get(cache, &find);
    if(r == NULL) {
        
        r = (UResourceDataEntry *) uprv_malloc(sizeof(UResourceDataEntry));
        if(r == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }

        uprv_memset(r, 0, sizeof(UResourceDataEntry));
        

        setEntryName(r, name, status);
        if (U_FAILURE(*status)) {
            uprv_free(r);
            return NULL;
        }

        if(path != NULL) {
            r->fPath = (char *)uprv_strdup(path);
            if(r->fPath == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                uprv_free(r);
                return NULL;
            }
        }

        
        res_load(&(r->fData), r->fPath, r->fName, status);

        if (U_FAILURE(*status)) { 
            
            *status = U_USING_FALLBACK_WARNING;
            r->fBogus = U_USING_FALLBACK_WARNING;
        } else { 
            Resource aliasres;
            if (r->fData.usesPoolBundle) {
                r->fPool = getPoolEntry(r->fPath, status);
                if (U_SUCCESS(*status)) {
                    const int32_t *poolIndexes = r->fPool->fData.pRoot + 1;
                    if(r->fData.pRoot[1 + URES_INDEX_POOL_CHECKSUM] == poolIndexes[URES_INDEX_POOL_CHECKSUM]) {
                        r->fData.poolBundleKeys = (const char *)(poolIndexes + (poolIndexes[URES_INDEX_LENGTH] & 0xff));
                    } else {
                        r->fBogus = *status = U_INVALID_FORMAT_ERROR;
                    }
                } else {
                    r->fBogus = *status;
                }
            }
            if (U_SUCCESS(*status)) {
                
                
                aliasres = res_getResource(&(r->fData), "%%ALIAS");
                if (aliasres != RES_BOGUS) {
                    const UChar *alias = res_getString(&(r->fData), aliasres, &aliasLen);
                    if(alias != NULL && aliasLen > 0) { 
                        u_UCharsToChars(alias, aliasName, aliasLen+1);
                        r->fAlias = init_entry(aliasName, path, status);
                    }
                }
            }
        }

        {
            UResourceDataEntry *oldR = NULL;
            if((oldR = (UResourceDataEntry *)uhash_get(cache, r)) == NULL) { 
                
                UErrorCode cacheStatus = U_ZERO_ERROR;
                uhash_put(cache, (void *)r, r, &cacheStatus);
                if (U_FAILURE(cacheStatus)) {
                    *status = cacheStatus;
                    free_entry(r);
                    r = NULL;
                }
            } else {
                
                
                free_entry(r);
                r = oldR;
            }
        }

    }
    if(r != NULL) {
        
        while(r->fAlias != NULL) {
            r = r->fAlias;
        }
        r->fCountExisting++; 
        
        
        if(r->fBogus != U_ZERO_ERROR && U_SUCCESS(*status)) {
             *status = r->fBogus; 
        }
    }
    return r;
}

static UResourceDataEntry *
getPoolEntry(const char *path, UErrorCode *status) {
    UResourceDataEntry *poolBundle = init_entry(kPoolBundleName, path, status);
    if( U_SUCCESS(*status) &&
        (poolBundle == NULL || poolBundle->fBogus != U_ZERO_ERROR || !poolBundle->fData.isPoolBundle)
    ) {
        *status = U_INVALID_FORMAT_ERROR;
    }
    return poolBundle;
}



static UResourceDataEntry *
findFirstExisting(const char* path, char* name,
                  UBool *isRoot, UBool *hasChopped, UBool *isDefault, UErrorCode* status) {
    UResourceDataEntry *r = NULL;
    UBool hasRealData = FALSE;
    const char *defaultLoc = uloc_getDefault();
    *hasChopped = TRUE; 

    while(*hasChopped && !hasRealData) {
        r = init_entry(name, path, status);
        
        if (U_FAILURE(*status)) {
            return NULL;
        }
        *isDefault = (UBool)(uprv_strncmp(name, defaultLoc, uprv_strlen(name)) == 0);
        hasRealData = (UBool)(r->fBogus == U_ZERO_ERROR);
        if(!hasRealData) {
            
            
            
            
            
            r->fCountExisting--;
            
            r = NULL;
            *status = U_USING_FALLBACK_WARNING;
        } else {
            uprv_strcpy(name, r->fName); 
        }

        *isRoot = (UBool)(uprv_strcmp(name, kRootLocaleName) == 0);

        
        *hasChopped = chopLocale(name);
    }
    return r;
}

static void ures_setIsStackObject( UResourceBundle* resB, UBool state) {
    if(state) {
        resB->fMagic1 = 0;
        resB->fMagic2 = 0;
    } else {
        resB->fMagic1 = MAGIC1;
        resB->fMagic2 = MAGIC2;
    }
}

static UBool ures_isStackObject(const UResourceBundle* resB) {
  return((resB->fMagic1 == MAGIC1 && resB->fMagic2 == MAGIC2)?FALSE:TRUE);
}


U_CFUNC void ures_initStackObject(UResourceBundle* resB) {
  uprv_memset(resB, 0, sizeof(UResourceBundle));
  ures_setIsStackObject(resB, TRUE);
}

static UBool  
loadParentsExceptRoot(UResourceDataEntry *&t1,
                      char name[], int32_t nameCapacity,
                      UBool usingUSRData, char usrDataPath[], UErrorCode *status) {
    if (U_FAILURE(*status)) { return FALSE; }
    UBool hasChopped = TRUE;
    while (hasChopped && t1->fParent == NULL && !t1->fData.noFallback &&
            res_getResource(&t1->fData,"%%ParentIsRoot") == RES_BOGUS) {
        Resource parentRes = res_getResource(&t1->fData, "%%Parent");
        if (parentRes != RES_BOGUS) {  
            int32_t parentLocaleLen = 0;
            const UChar *parentLocaleName = res_getString(&(t1->fData), parentRes, &parentLocaleLen);
            if(parentLocaleName != NULL && 0 < parentLocaleLen && parentLocaleLen < nameCapacity) {
                u_UCharsToChars(parentLocaleName, name, parentLocaleLen + 1);
                if (uprv_strcmp(name, kRootLocaleName) == 0) {
                    return TRUE;
                }
            }
        }
        
        UErrorCode parentStatus = U_ZERO_ERROR;
        UResourceDataEntry *t2 = init_entry(name, t1->fPath, &parentStatus);
        if (U_FAILURE(parentStatus)) {
            *status = parentStatus;
            return FALSE;
        }
        UResourceDataEntry *u2 = NULL;
        UErrorCode usrStatus = U_ZERO_ERROR;
        if (usingUSRData) {  
            u2 = init_entry(name, usrDataPath, &usrStatus);
        }

        if (usingUSRData && U_SUCCESS(usrStatus) && u2->fBogus == U_ZERO_ERROR) {
            t1->fParent = u2;
            u2->fParent = t2;
        } else {
            t1->fParent = t2;
            if (usingUSRData) {
                
                u2->fCountExisting = 0;
            }
        }
        t1 = t2;
        hasChopped = chopLocale(name);
    }
    return TRUE;
}

static UBool  
insertRootBundle(UResourceDataEntry *&t1, UErrorCode *status) {
    if (U_FAILURE(*status)) { return FALSE; }
    UErrorCode parentStatus = U_ZERO_ERROR;
    UResourceDataEntry *t2 = init_entry(kRootLocaleName, t1->fPath, &parentStatus);
    if (U_FAILURE(parentStatus)) {
        *status = parentStatus;
        return FALSE;
    }
    t1->fParent = t2;
    t1 = t2;
    return TRUE;
}

enum UResOpenType {
    






    URES_OPEN_LOCALE_DEFAULT_ROOT,
    
    
    
    
    







    URES_OPEN_LOCALE_ROOT,
    





    URES_OPEN_DIRECT
};
typedef enum UResOpenType UResOpenType;

static UResourceDataEntry *entryOpen(const char* path, const char* localeID,
                                     UResOpenType openType, UErrorCode* status) {
    U_ASSERT(openType != URES_OPEN_DIRECT);
    UErrorCode intStatus = U_ZERO_ERROR;
    UResourceDataEntry *r = NULL;
    UResourceDataEntry *t1 = NULL;
    UBool isDefault = FALSE;
    UBool isRoot = FALSE;
    UBool hasRealData = FALSE;
    UBool hasChopped = TRUE;
    UBool usingUSRData = U_USE_USRDATA && ( path == NULL || uprv_strncmp(path,U_ICUDATA_NAME,8) == 0);

    char name[ULOC_FULLNAME_CAPACITY];
    char usrDataPath[96];

    initCache(status);

    if(U_FAILURE(*status)) {
        return NULL;
    }

    uprv_strncpy(name, localeID, sizeof(name) - 1);
    name[sizeof(name) - 1] = 0;

    if ( usingUSRData ) {
        if ( path == NULL ) {
            uprv_strcpy(usrDataPath, U_USRDATA_NAME);
        } else {
            uprv_strncpy(usrDataPath, path, sizeof(usrDataPath) - 1);
            usrDataPath[0] = 'u';
            usrDataPath[1] = 's';
            usrDataPath[2] = 'r';
            usrDataPath[sizeof(usrDataPath) - 1] = 0;
        }
    }
 
    umtx_lock(&resbMutex);
    { 
        
        r = findFirstExisting(path, name, &isRoot, &hasChopped, &isDefault, &intStatus);

        if(r != NULL) { 
            t1 = r;
            hasRealData = TRUE;
            if ( usingUSRData ) {  
                UErrorCode usrStatus = U_ZERO_ERROR;
                UResourceDataEntry *u1 = init_entry(t1->fName, usrDataPath, &usrStatus);
               if ( u1 != NULL ) {
                 if(u1->fBogus == U_ZERO_ERROR) {
                   u1->fParent = t1;
                   r = u1;
                 } else {
                   
                   u1->fCountExisting = 0;
                 }
               }
            }
            if (hasChopped && !isRoot) {
                if (!loadParentsExceptRoot(t1, name, UPRV_LENGTHOF(name), usingUSRData, usrDataPath, status)) {
                    goto finishUnlock;
                }
            }
        }

        
        
        if(r==NULL && openType == URES_OPEN_LOCALE_DEFAULT_ROOT && !isDefault && !isRoot) {
            
            uprv_strcpy(name, uloc_getDefault());
            r = findFirstExisting(path, name, &isRoot, &hasChopped, &isDefault, &intStatus);
            intStatus = U_USING_DEFAULT_WARNING;
            if(r != NULL) { 
                t1 = r;
                hasRealData = TRUE;
                isDefault = TRUE;
                
                if (hasChopped && !isRoot) {
                    if (!loadParentsExceptRoot(t1, name, UPRV_LENGTHOF(name), usingUSRData, usrDataPath, status)) {
                        goto finishUnlock;
                    }
                }
            } 
        }

        
        
        if(r == NULL) {
            uprv_strcpy(name, kRootLocaleName);
            r = findFirstExisting(path, name, &isRoot, &hasChopped, &isDefault, &intStatus);
            if(r != NULL) {
                t1 = r;
                intStatus = U_USING_DEFAULT_WARNING;
                hasRealData = TRUE;
            } else { 
                *status = U_MISSING_RESOURCE_ERROR;
                goto finishUnlock;
            }
        } else if(!isRoot && uprv_strcmp(t1->fName, kRootLocaleName) != 0 &&
                t1->fParent == NULL && !r->fData.noFallback) {
            if (!insertRootBundle(t1, status)) {
                goto finishUnlock;
            }
            if(!hasRealData) {
                r->fBogus = U_USING_DEFAULT_WARNING;
            }
        }

        
        while(r != NULL && !isRoot && t1->fParent != NULL) {
            t1->fParent->fCountExisting++;
            t1 = t1->fParent;
        }
    } 
finishUnlock:
    umtx_unlock(&resbMutex);

    if(U_SUCCESS(*status)) {
        if(intStatus != U_ZERO_ERROR) {
            *status = intStatus;  
        }
        return r;
    } else {
        return NULL;
    }
}







static UResourceDataEntry *
entryOpenDirect(const char* path, const char* localeID, UErrorCode* status) {
    initCache(status);
    if(U_FAILURE(*status)) {
        return NULL;
    }

    umtx_lock(&resbMutex);
    
    UResourceDataEntry *r = init_entry(localeID, path, status);
    if(U_SUCCESS(*status)) {
        if(r->fBogus != U_ZERO_ERROR) {
            r->fCountExisting--;
            r = NULL;
        }
    } else {
        r = NULL;
    }

    
    
    UResourceDataEntry *t1 = r;
    if(r != NULL && uprv_strcmp(localeID, kRootLocaleName) != 0 &&  
            r->fParent == NULL && !r->fData.noFallback &&
            uprv_strlen(localeID) < ULOC_FULLNAME_CAPACITY) {
        char name[ULOC_FULLNAME_CAPACITY];
        uprv_strcpy(name, localeID);
        if(!chopLocale(name) || uprv_strcmp(name, kRootLocaleName) == 0 ||
                loadParentsExceptRoot(t1, name, UPRV_LENGTHOF(name), FALSE, NULL, status)) {
            if(uprv_strcmp(t1->fName, kRootLocaleName) != 0 && t1->fParent == NULL) {
                insertRootBundle(t1, status);
            }
        }
        if(U_FAILURE(*status)) {
            r = NULL;
        }
    }

    if(r != NULL) {
        
        while(t1->fParent != NULL) {
            t1->fParent->fCountExisting++;
            t1 = t1->fParent;
        }
    }
    umtx_unlock(&resbMutex);
    return r;
}






static void entryCloseInt(UResourceDataEntry *resB) {
    UResourceDataEntry *p = resB;

    while(resB != NULL) {
        p = resB->fParent;
        resB->fCountExisting--;

        

















        resB = p;
    }
}





static void entryClose(UResourceDataEntry *resB) {
  umtx_lock(&resbMutex);
  entryCloseInt(resB);
  umtx_unlock(&resbMutex);
}


















static void ures_appendResPath(UResourceBundle *resB, const char* toAdd, int32_t lenToAdd, UErrorCode *status) {
    int32_t resPathLenOrig = resB->fResPathLen;
    if(resB->fResPath == NULL) {
        resB->fResPath = resB->fResBuf;
        *(resB->fResPath) = 0;
        resB->fResPathLen = 0;
    } 
    resB->fResPathLen += lenToAdd;
    if(RES_BUFSIZE <= resB->fResPathLen+1) {
        if(resB->fResPath == resB->fResBuf) {
            resB->fResPath = (char *)uprv_malloc((resB->fResPathLen+1)*sizeof(char));
            
            if (resB->fResPath == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            uprv_strcpy(resB->fResPath, resB->fResBuf);
        } else {
            char *temp = (char *)uprv_realloc(resB->fResPath, (resB->fResPathLen+1)*sizeof(char));
            
            if (temp == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            resB->fResPath = temp;
        }
    }
    uprv_strcpy(resB->fResPath + resPathLenOrig, toAdd);
}

static void ures_freeResPath(UResourceBundle *resB) {
    if (resB->fResPath && resB->fResPath != resB->fResBuf) {
        uprv_free(resB->fResPath);
    }
    resB->fResPath = NULL;
    resB->fResPathLen = 0;
}

static void
ures_closeBundle(UResourceBundle* resB, UBool freeBundleObj)
{
    if(resB != NULL) {
        if(resB->fData != NULL) {
            entryClose(resB->fData);
        }
        if(resB->fVersion != NULL) {
            uprv_free(resB->fVersion);
        }
        ures_freeResPath(resB);

        if(ures_isStackObject(resB) == FALSE && freeBundleObj) {
            uprv_free(resB);
        }
#if 0 
        else {
            
            uprv_memset(resB, -1, sizeof(UResourceBundle));
        }
#endif
    }
}

U_CAPI void  U_EXPORT2
ures_close(UResourceBundle* resB)
{
    ures_closeBundle(resB, TRUE);
}

static UResourceBundle *init_resb_result(const ResourceData *rdata, Resource r, 
                                         const char *key, int32_t idx, UResourceDataEntry *realData, 
                                         const UResourceBundle *parent, int32_t noAlias,
                                         UResourceBundle *resB, UErrorCode *status) 
{
    if(status == NULL || U_FAILURE(*status)) {
        return resB;
    }
    if (parent == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    if(RES_GET_TYPE(r) == URES_ALIAS) { 
        if(noAlias < URES_MAX_ALIAS_LEVEL) { 
            int32_t len = 0;
            const UChar *alias = res_getAlias(rdata, r, &len); 
            if(len > 0) {
                
                char stackAlias[200];
                char *chAlias = NULL, *path = NULL, *locale = NULL, *keyPath = NULL;
                int32_t capacity;

                












                ++len; 
                if(parent->fResPath != NULL) {
                    capacity = (int32_t)uprv_strlen(parent->fResPath) + 1;
                } else {
                    capacity = 0;
                }
                if(capacity < len) {
                    capacity = len;
                }
                if(capacity <= (int32_t)sizeof(stackAlias)) {
                    capacity = (int32_t)sizeof(stackAlias);
                    chAlias = stackAlias;
                } else {
                    chAlias = (char *)uprv_malloc(capacity);
                    
                    if(chAlias == NULL) {
                        *status = U_MEMORY_ALLOCATION_ERROR;
                        return NULL;
                    }
                }
                u_UCharsToChars(alias, chAlias, len);

                if(*chAlias == RES_PATH_SEPARATOR) {
                    
                    locale = uprv_strchr(chAlias+1, RES_PATH_SEPARATOR);
                    if(locale == NULL) {
                        locale = uprv_strchr(chAlias, 0); 
                    } else {
                        *locale = 0;
                        locale++;
                    }
                    path = chAlias+1;
                    if(uprv_strcmp(path, "LOCALE") == 0) {
                        
                        
                        
                        keyPath = locale; 
                        locale = parent->fTopLevelData->fName; 
                        path = realData->fPath; 
                    } else {
                        if(uprv_strcmp(path, "ICUDATA") == 0) { 
                            path = NULL;
                        }
                        keyPath = uprv_strchr(locale, RES_PATH_SEPARATOR);
                        if(keyPath) {
                            *keyPath = 0;
                            keyPath++;
                        }
                    }
                } else {
                    
                    locale = chAlias;
                    keyPath = uprv_strchr(locale, RES_PATH_SEPARATOR);
                    if(keyPath) {
                        *keyPath = 0;
                        keyPath++;
                    }
                    path = realData->fPath;
                }


                {
                    
                    
                    UResourceBundle *result = resB;
                    const char* temp = NULL;
                    UErrorCode intStatus = U_ZERO_ERROR;
                    UResourceBundle *mainRes = ures_openDirect(path, locale, &intStatus); 
                    if(U_SUCCESS(intStatus)) {
                        if(keyPath == NULL) {
                            



                            


                            char *aKey = parent->fResPath;
                            if(aKey) {
                                uprv_strcpy(chAlias, aKey); 
                                aKey = chAlias;
                                r = res_findResource(&(mainRes->fResData), mainRes->fRes, &aKey, &temp);
                            } else {
                                r = mainRes->fRes;
                            }
                            if(key) {
                                


                                len = (int32_t)(uprv_strlen(key) + 1);
                                if(len > capacity) {
                                    capacity = len;
                                    if(chAlias == stackAlias) {
                                        chAlias = (char *)uprv_malloc(capacity);
                                    } else {
                                        chAlias = (char *)uprv_realloc(chAlias, capacity);
                                    }
                                    if(chAlias == NULL) {
                                        ures_close(mainRes);
                                        *status = U_MEMORY_ALLOCATION_ERROR;
                                        return NULL;
                                    }
                                }
                                uprv_memcpy(chAlias, key, len);
                                aKey = chAlias;
                                r = res_findResource(&(mainRes->fResData), r, &aKey, &temp);
                            } else if(idx != -1) {
                                
                                
                                int32_t type = RES_GET_TYPE(r);
                                if(URES_IS_TABLE(type)) {
                                    r = res_getTableItemByIndex(&(mainRes->fResData), r, idx, (const char **)&aKey);
                                } else { 
                                    r = res_getArrayItem(&(mainRes->fResData), r, idx);
                                }
                            }
                            if(r != RES_BOGUS) {
                                result = init_resb_result(&(mainRes->fResData), r, temp, -1, mainRes->fData, mainRes, noAlias+1, resB, status);
                            } else {
                                *status = U_MISSING_RESOURCE_ERROR;
                                result = resB;
                            }
                        } else {
                            






                            UResourceDataEntry *dataEntry = mainRes->fData;
                            char stackPath[URES_MAX_BUFFER_SIZE];
                            char *pathBuf = stackPath, *myPath = pathBuf;
                            if(uprv_strlen(keyPath) > URES_MAX_BUFFER_SIZE) {
                                pathBuf = (char *)uprv_malloc((uprv_strlen(keyPath)+1)*sizeof(char));
                                if(pathBuf == NULL) {
                                    *status = U_MEMORY_ALLOCATION_ERROR;
                                    return NULL;
                                }
                            }
                            uprv_strcpy(pathBuf, keyPath);
                            result = mainRes;
                            
                            do {
                                r = dataEntry->fData.rootRes;     
                                
                                while(*myPath && U_SUCCESS(*status)) {
                                    r = res_findResource(&(dataEntry->fData), r, &myPath, &temp);
                                    if(r != RES_BOGUS) { 
                                        resB = init_resb_result(&(dataEntry->fData), r, temp, -1, dataEntry, result, noAlias+1, resB, status);
                                        result = resB;
                                        if(result) {
                                            r = result->fRes; 
                                            dataEntry = result->fData;
                                        }
                                    } else { 
                                        break;
                                    }
                                }
                                dataEntry = dataEntry->fParent;
                                uprv_strcpy(pathBuf, keyPath);
                                myPath = pathBuf;
                            } while(r == RES_BOGUS && dataEntry != NULL);
                            if(r == RES_BOGUS) {
                                *status = U_MISSING_RESOURCE_ERROR;
                                result = resB;
                            }
                            if(pathBuf != stackPath) {
                                uprv_free(pathBuf);
                            }
                        }
                    } else { 
                        *status = intStatus;
                    }
                    if(chAlias != stackAlias) {
                        uprv_free(chAlias);
                    }
                    if(mainRes != result) {
                        ures_close(mainRes);
                    }
                    return result;
                }
            } else {
                 
                *status = U_ILLEGAL_ARGUMENT_ERROR;
                return resB;
            }
        } else {
            *status = U_TOO_MANY_ALIASES_ERROR;
            return resB;
        }
    }
    if(resB == NULL) {
        resB = (UResourceBundle *)uprv_malloc(sizeof(UResourceBundle));
        
        if (resB == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        ures_setIsStackObject(resB, FALSE);
        resB->fResPath = NULL;
        resB->fResPathLen = 0;
    } else {
        if(resB->fData != NULL) {
            entryClose(resB->fData);
        }
        if(resB->fVersion != NULL) {
            uprv_free(resB->fVersion);
        }
        





        




        if(parent != resB) {
            ures_freeResPath(resB);
        }
    }
    resB->fData = realData;
    entryIncrease(resB->fData);
    resB->fHasFallback = FALSE;
    resB->fIsTopLevel = FALSE;
    resB->fIndex = -1;
    resB->fKey = key; 
    
    resB->fTopLevelData = parent->fTopLevelData;
    if(parent->fResPath && parent != resB) {
        ures_appendResPath(resB, parent->fResPath, parent->fResPathLen, status);
    }
    if(key != NULL) {
        ures_appendResPath(resB, key, (int32_t)uprv_strlen(key), status);
        if(resB->fResPath[resB->fResPathLen-1] != RES_PATH_SEPARATOR) {
            ures_appendResPath(resB, RES_PATH_SEPARATOR_S, 1, status);
        }
    } else if(idx >= 0) {
        char buf[256];
        int32_t len = T_CString_integerToString(buf, idx, 10);
        ures_appendResPath(resB, buf, len, status);
        if(resB->fResPath[resB->fResPathLen-1] != RES_PATH_SEPARATOR) {
            ures_appendResPath(resB, RES_PATH_SEPARATOR_S, 1, status);
        }
    }
    
    {
        int32_t usedLen = ((resB->fResBuf == resB->fResPath) ? resB->fResPathLen : 0);
        uprv_memset(resB->fResBuf + usedLen, 0, sizeof(resB->fResBuf) - usedLen);
    }

    resB->fVersion = NULL;
    resB->fRes = r;
    
    uprv_memmove(&resB->fResData, rdata, sizeof(ResourceData));
    resB->fSize = res_countArrayItems(&(resB->fResData), resB->fRes);
    return resB;
}

UResourceBundle *ures_copyResb(UResourceBundle *r, const UResourceBundle *original, UErrorCode *status) {
    UBool isStackObject;
    if(U_FAILURE(*status) || r == original) {
        return r;
    }
    if(original != NULL) {
        if(r == NULL) {
            isStackObject = FALSE;
            r = (UResourceBundle *)uprv_malloc(sizeof(UResourceBundle));
            
            if (r == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                return NULL;
            }
        } else {
            isStackObject = ures_isStackObject(r);
            ures_closeBundle(r, FALSE);
        }
        uprv_memcpy(r, original, sizeof(UResourceBundle));
        r->fResPath = NULL;
        r->fResPathLen = 0;
        if(original->fResPath) {
            ures_appendResPath(r, original->fResPath, original->fResPathLen, status);
        }
        ures_setIsStackObject(r, isStackObject);
        if(r->fData != NULL) {
            entryIncrease(r->fData);
        }
    }
    return r;
}





U_CAPI const UChar* U_EXPORT2 ures_getString(const UResourceBundle* resB, int32_t* len, UErrorCode* status) {
    const UChar *s;
    if (status==NULL || U_FAILURE(*status)) {
        return NULL;
    }
    if(resB == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    s = res_getString(&(resB->fResData), resB->fRes, len);
    if (s == NULL) {
        *status = U_RESOURCE_TYPE_MISMATCH;
    }
    return s;
}

static const char *
ures_toUTF8String(const UChar *s16, int32_t length16,
                  char *dest, int32_t *pLength,
                  UBool forceCopy,
                  UErrorCode *status) {
    int32_t capacity;

    if (U_FAILURE(*status)) {
        return NULL;
    }
    if (pLength != NULL) {
        capacity = *pLength;
    } else {
        capacity = 0;
    }
    if (capacity < 0 || (capacity > 0 && dest == NULL)) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    if (length16 == 0) {
        
        if (pLength != NULL) {
            *pLength = 0;
        }
        if (forceCopy) {
            u_terminateChars(dest, capacity, 0, status);
            return dest;
        } else {
            return "";
        }
    } else {
        
        if (capacity < length16) {
            
            return u_strToUTF8(NULL, 0, pLength, s16, length16, status);
        }
        if (!forceCopy && (length16 <= 0x2aaaaaaa)) {
            













            int32_t maxLength = 3 * length16 + 1;
            if (capacity > maxLength) {
                dest += capacity - maxLength;
                capacity = maxLength;
            }
        }
        return u_strToUTF8(dest, capacity, pLength, s16, length16, status);
    }
}

U_CAPI const char * U_EXPORT2
ures_getUTF8String(const UResourceBundle *resB,
                   char *dest, int32_t *pLength,
                   UBool forceCopy,
                   UErrorCode *status) {
    int32_t length16;
    const UChar *s16 = ures_getString(resB, &length16, status);
    return ures_toUTF8String(s16, length16, dest, pLength, forceCopy, status);
}

U_CAPI const uint8_t* U_EXPORT2 ures_getBinary(const UResourceBundle* resB, int32_t* len, 
                                               UErrorCode*               status) {
  const uint8_t *p;
  if (status==NULL || U_FAILURE(*status)) {
    return NULL;
  }
  if(resB == NULL) {
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return NULL;
  }
  p = res_getBinary(&(resB->fResData), resB->fRes, len);
  if (p == NULL) {
    *status = U_RESOURCE_TYPE_MISMATCH;
  }
  return p;
}

U_CAPI const int32_t* U_EXPORT2 ures_getIntVector(const UResourceBundle* resB, int32_t* len, 
                                                   UErrorCode*               status) {
  const int32_t *p;
  if (status==NULL || U_FAILURE(*status)) {
    return NULL;
  }
  if(resB == NULL) {
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return NULL;
  }
  p = res_getIntVector(&(resB->fResData), resB->fRes, len);
  if (p == NULL) {
    *status = U_RESOURCE_TYPE_MISMATCH;
  }
  return p;
}

 

U_CAPI int32_t U_EXPORT2 ures_getInt(const UResourceBundle* resB, UErrorCode *status) {
  if (status==NULL || U_FAILURE(*status)) {
    return 0xffffffff;
  }
  if(resB == NULL) {
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return 0xffffffff;
  }
  if(RES_GET_TYPE(resB->fRes) != URES_INT) {
    *status = U_RESOURCE_TYPE_MISMATCH;
    return 0xffffffff;
  }
  return RES_GET_INT(resB->fRes);
}

U_CAPI uint32_t U_EXPORT2 ures_getUInt(const UResourceBundle* resB, UErrorCode *status) {
  if (status==NULL || U_FAILURE(*status)) {
    return 0xffffffff;
  }
  if(resB == NULL) {
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return 0xffffffff;
  }
  if(RES_GET_TYPE(resB->fRes) != URES_INT) {
    *status = U_RESOURCE_TYPE_MISMATCH;
    return 0xffffffff;
  }
  return RES_GET_UINT(resB->fRes);
}

U_CAPI UResType U_EXPORT2 ures_getType(const UResourceBundle *resB) {
  if(resB == NULL) {
    return URES_NONE;
  }
  return res_getPublicType(resB->fRes);
}

U_CAPI const char * U_EXPORT2 ures_getKey(const UResourceBundle *resB) {
  if(resB == NULL) {
    return NULL;
  }
  
  return(resB->fKey);
}

U_CAPI int32_t U_EXPORT2 ures_getSize(const UResourceBundle *resB) {
  if(resB == NULL) {
    return 0;
  }
  
  return resB->fSize;
}

static const UChar* ures_getStringWithAlias(const UResourceBundle *resB, Resource r, int32_t sIndex, int32_t *len, UErrorCode *status) {
  if(RES_GET_TYPE(r) == URES_ALIAS) {
    const UChar* result = 0;
    UResourceBundle *tempRes = ures_getByIndex(resB, sIndex, NULL, status);
    result = ures_getString(tempRes, len, status);
    ures_close(tempRes);
    return result;
  } else {
    return res_getString(&(resB->fResData), r, len); 
  }
}

U_CAPI void U_EXPORT2 ures_resetIterator(UResourceBundle *resB){
  if(resB == NULL) {
    return;
  }
  resB->fIndex = -1;
}

U_CAPI UBool U_EXPORT2 ures_hasNext(const UResourceBundle *resB) {
  if(resB == NULL) {
    return FALSE;
  }
  return (UBool)(resB->fIndex < resB->fSize-1);
}

U_CAPI const UChar* U_EXPORT2 ures_getNextString(UResourceBundle *resB, int32_t* len, const char ** key, UErrorCode *status) {
  Resource r = RES_BOGUS;
  
  if (status==NULL || U_FAILURE(*status)) {
    return NULL;
  }
  if(resB == NULL) {
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return NULL;
  }
  
  if(resB->fIndex == resB->fSize-1) {
    *status = U_INDEX_OUTOFBOUNDS_ERROR;
  } else {
    resB->fIndex++;
    switch(RES_GET_TYPE(resB->fRes)) {
    case URES_STRING:
    case URES_STRING_V2:
      return res_getString(&(resB->fResData), resB->fRes, len); 
    case URES_TABLE:
    case URES_TABLE16:
    case URES_TABLE32:
      r = res_getTableItemByIndex(&(resB->fResData), resB->fRes, resB->fIndex, key);
      if(r == RES_BOGUS && resB->fHasFallback) {
        
      }
      return ures_getStringWithAlias(resB, r, resB->fIndex, len, status);
    case URES_ARRAY:
    case URES_ARRAY16:
      r = res_getArrayItem(&(resB->fResData), resB->fRes, resB->fIndex);
      if(r == RES_BOGUS && resB->fHasFallback) {
        
      }
      return ures_getStringWithAlias(resB, r, resB->fIndex, len, status);
    case URES_ALIAS:
      return ures_getStringWithAlias(resB, resB->fRes, resB->fIndex, len, status);
    case URES_INT:
    case URES_BINARY:
    case URES_INT_VECTOR:
        *status = U_RESOURCE_TYPE_MISMATCH;
    default: 
      return NULL;
    }
  }

  return NULL;
}

U_CAPI UResourceBundle* U_EXPORT2 ures_getNextResource(UResourceBundle *resB, UResourceBundle *fillIn, UErrorCode *status) {
    const char *key = NULL;
    Resource r = RES_BOGUS;

    if (status==NULL || U_FAILURE(*status)) {
            
            return fillIn;
    }
    if(resB == NULL) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
            
            return fillIn;
    }

    if(resB->fIndex == resB->fSize-1) {
      *status = U_INDEX_OUTOFBOUNDS_ERROR;
      
    } else {
        resB->fIndex++;
        switch(RES_GET_TYPE(resB->fRes)) {
        case URES_INT:
        case URES_BINARY:
        case URES_STRING:
        case URES_STRING_V2:
        case URES_INT_VECTOR:
            return ures_copyResb(fillIn, resB, status);
        case URES_TABLE:
        case URES_TABLE16:
        case URES_TABLE32:
            r = res_getTableItemByIndex(&(resB->fResData), resB->fRes, resB->fIndex, &key);
            if(r == RES_BOGUS && resB->fHasFallback) {
                
            }
            return init_resb_result(&(resB->fResData), r, key, resB->fIndex, resB->fData, resB, 0, fillIn, status);
        case URES_ARRAY:
        case URES_ARRAY16:
            r = res_getArrayItem(&(resB->fResData), resB->fRes, resB->fIndex);
            if(r == RES_BOGUS && resB->fHasFallback) {
                
            }
            return init_resb_result(&(resB->fResData), r, key, resB->fIndex, resB->fData, resB, 0, fillIn, status);
        default:
            
            return fillIn;
        }
    }
    
    return fillIn;
}

U_CAPI UResourceBundle* U_EXPORT2 ures_getByIndex(const UResourceBundle *resB, int32_t indexR, UResourceBundle *fillIn, UErrorCode *status) {
    const char* key = NULL;
    Resource r = RES_BOGUS;

    if (status==NULL || U_FAILURE(*status)) {
        
        return fillIn;
    }
    if(resB == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        
        return fillIn;
    }

    if(indexR >= 0 && resB->fSize > indexR) {
        switch(RES_GET_TYPE(resB->fRes)) {
        case URES_INT:
        case URES_BINARY:
        case URES_STRING:
        case URES_STRING_V2:
        case URES_INT_VECTOR:
            return ures_copyResb(fillIn, resB, status);
        case URES_TABLE:
        case URES_TABLE16:
        case URES_TABLE32:
            r = res_getTableItemByIndex(&(resB->fResData), resB->fRes, indexR, &key);
            if(r == RES_BOGUS && resB->fHasFallback) {
                
            }
            return init_resb_result(&(resB->fResData), r, key, indexR, resB->fData, resB, 0, fillIn, status);
        case URES_ARRAY:
        case URES_ARRAY16:
            r = res_getArrayItem(&(resB->fResData), resB->fRes, indexR);
            if(r == RES_BOGUS && resB->fHasFallback) {
                
            }
            return init_resb_result(&(resB->fResData), r, key, indexR, resB->fData, resB, 0, fillIn, status);
        default:
            
            return fillIn;
        }
    } else {
        *status = U_MISSING_RESOURCE_ERROR;
    }
    
    return fillIn;
}

U_CAPI const UChar* U_EXPORT2 ures_getStringByIndex(const UResourceBundle *resB, int32_t indexS, int32_t* len, UErrorCode *status) {
    const char* key = NULL;
    Resource r = RES_BOGUS;

    if (status==NULL || U_FAILURE(*status)) {
        return NULL;
    }
    if(resB == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    if(indexS >= 0 && resB->fSize > indexS) {
        switch(RES_GET_TYPE(resB->fRes)) {
        case URES_STRING:
        case URES_STRING_V2:
            return res_getString(&(resB->fResData), resB->fRes, len);
        case URES_TABLE:
        case URES_TABLE16:
        case URES_TABLE32:
            r = res_getTableItemByIndex(&(resB->fResData), resB->fRes, indexS, &key);
            if(r == RES_BOGUS && resB->fHasFallback) {
                
            }
            return ures_getStringWithAlias(resB, r, indexS, len, status);
        case URES_ARRAY:
        case URES_ARRAY16:
            r = res_getArrayItem(&(resB->fResData), resB->fRes, indexS);
            if(r == RES_BOGUS && resB->fHasFallback) {
                
            }
            return ures_getStringWithAlias(resB, r, indexS, len, status);
        case URES_ALIAS:
            return ures_getStringWithAlias(resB, resB->fRes, indexS, len, status);
        case URES_INT:
        case URES_BINARY:
        case URES_INT_VECTOR:
            *status = U_RESOURCE_TYPE_MISMATCH;
            break;
        default:
          
          *status = U_INTERNAL_PROGRAM_ERROR;
          break;
        }
    } else {
        *status = U_MISSING_RESOURCE_ERROR;
    }
    return NULL;
}

U_CAPI const char * U_EXPORT2
ures_getUTF8StringByIndex(const UResourceBundle *resB,
                          int32_t idx,
                          char *dest, int32_t *pLength,
                          UBool forceCopy,
                          UErrorCode *status) {
    int32_t length16;
    const UChar *s16 = ures_getStringByIndex(resB, idx, &length16, status);
    return ures_toUTF8String(s16, length16, dest, pLength, forceCopy, status);
}





U_CAPI UResourceBundle* U_EXPORT2
ures_findResource(const char* path, UResourceBundle *fillIn, UErrorCode *status) 
{
  UResourceBundle *first = NULL; 
  UResourceBundle *result = fillIn;
  char *packageName = NULL;
  char *pathToResource = NULL, *save = NULL;
  char *locale = NULL, *localeEnd = NULL;
  int32_t length;

  if(status == NULL || U_FAILURE(*status)) {
    return result;
  }

  length = (int32_t)(uprv_strlen(path)+1);
  save = pathToResource = (char *)uprv_malloc(length*sizeof(char));
  
  if(pathToResource == NULL) {
    *status = U_MEMORY_ALLOCATION_ERROR;
    return result;
  }
  uprv_memcpy(pathToResource, path, length);

  locale = pathToResource;
  if(*pathToResource == RES_PATH_SEPARATOR) { 
    pathToResource++;
    packageName = pathToResource;
    pathToResource = uprv_strchr(pathToResource, RES_PATH_SEPARATOR);
    if(pathToResource == NULL) {
      *status = U_ILLEGAL_ARGUMENT_ERROR;
    } else {
      *pathToResource = 0;
      locale = pathToResource+1;
    }
  }

  localeEnd = uprv_strchr(locale, RES_PATH_SEPARATOR);
  if(localeEnd != NULL) {
    *localeEnd = 0;
  }

  first = ures_open(packageName, locale, status);

  if(U_SUCCESS(*status)) {
    if(localeEnd) {
      result = ures_findSubResource(first, localeEnd+1, fillIn, status);
    } else {
      result = ures_copyResb(fillIn, first, status);
    }
    ures_close(first);
  }
  uprv_free(save);
  return result;
}

U_CAPI UResourceBundle* U_EXPORT2
ures_findSubResource(const UResourceBundle *resB, char* path, UResourceBundle *fillIn, UErrorCode *status) 
{
  Resource res = RES_BOGUS;
  UResourceBundle *result = fillIn;
  const char *key;

  if(status == NULL || U_FAILURE(*status)) {
    return result;
  }

  
  
  
  do {
    res = res_findResource(&(resB->fResData), resB->fRes, &path, &key); 
    if(res != RES_BOGUS) {
        result = init_resb_result(&(resB->fResData), res, key, -1, resB->fData, resB, 0, fillIn, status);
        resB = result;
    } else {
        *status = U_MISSING_RESOURCE_ERROR;
        break;
    }
  } while(*path); 

  return result;
}
U_INTERNAL const UChar* U_EXPORT2 
ures_getStringByKeyWithFallback(const UResourceBundle *resB, 
                                const char* inKey, 
                                int32_t* len,
                                UErrorCode *status) {

    UResourceBundle stack;
    const UChar* retVal = NULL;
    ures_initStackObject(&stack);
    ures_getByKeyWithFallback(resB, inKey, &stack, status);
    int32_t length;
    retVal = ures_getString(&stack, &length, status);
    ures_close(&stack);
    if (U_FAILURE(*status)) {
        return NULL;
    }
    if (length == 3 && retVal[0] == EMPTY_SET && retVal[1] == EMPTY_SET && retVal[2] == EMPTY_SET ) {
        retVal = NULL;
        length = 0;
        *status = U_MISSING_RESOURCE_ERROR;
    }
    if (len != NULL) {
        *len = length;
    }
    return retVal;
}



  
static Resource getTableItemByKeyPath(const ResourceData *pResData, Resource table, const char *key) {
  Resource resource = table;  
  icu::CharString path;
  UErrorCode errorCode = U_ZERO_ERROR;
  path.append(key, errorCode);
  if (U_FAILURE(errorCode)) { return RES_BOGUS; }
  char *pathPart = path.data();  
  UResType type = (UResType)RES_GET_TYPE(resource);  
  while (*pathPart && resource != RES_BOGUS && URES_IS_CONTAINER(type)) {
    char *nextPathPart = uprv_strchr(pathPart, RES_PATH_SEPARATOR);
    if (nextPathPart != NULL) {
      *nextPathPart = 0;  
      nextPathPart++;
    } else {
      nextPathPart = uprv_strchr(pathPart, 0);
    }
    int32_t t;
    const char *pathP = pathPart;
    resource = res_getTableItemByKey(pResData, resource, &t, &pathP);
    type = (UResType)RES_GET_TYPE(resource);
    pathPart = nextPathPart; 
  }
  if (*pathPart) {
    return RES_BOGUS;
  }
  return resource;
}

U_CAPI UResourceBundle* U_EXPORT2 
ures_getByKeyWithFallback(const UResourceBundle *resB, 
                          const char* inKey, 
                          UResourceBundle *fillIn, 
                          UErrorCode *status) {
    Resource res = RES_BOGUS, rootRes = RES_BOGUS;
    
    UResourceBundle *helper = NULL;

    if (status==NULL || U_FAILURE(*status)) {
        return fillIn;
    }
    if(resB == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return fillIn;
    }

    int32_t type = RES_GET_TYPE(resB->fRes);
    if(URES_IS_TABLE(type)) {
        res = getTableItemByKeyPath(&(resB->fResData), resB->fRes, inKey);
        const char* key = inKey;
        if(res == RES_BOGUS) {
            UResourceDataEntry *dataEntry = resB->fData;
            CharString path;
            char *myPath = NULL;
            const char* resPath = resB->fResPath;
            int32_t len = resB->fResPathLen;
            while(res == RES_BOGUS && dataEntry->fParent != NULL) { 
                dataEntry = dataEntry->fParent;
                rootRes = dataEntry->fData.rootRes;

                if(dataEntry->fBogus == U_ZERO_ERROR) {
                    path.clear();
                    if (len > 0) {
                        path.append(resPath, len, *status);
                    }
                    path.append(inKey, *status);
                    if (U_FAILURE(*status)) {
                        ures_close(helper);
                        return fillIn;
                    }
                    myPath = path.data();
                    key = inKey;
                    do {
                        res = res_findResource(&(dataEntry->fData), rootRes, &myPath, &key);
                        if (RES_GET_TYPE(res) == URES_ALIAS && *myPath) {
                            
                            helper = init_resb_result(&(dataEntry->fData), res, NULL, -1, dataEntry, resB, 0, helper, status); 
                            
                            if(helper) {
                              dataEntry = helper->fData;
                              rootRes = helper->fRes;
                              resPath = helper->fResPath;
                              len = helper->fResPathLen;

                            } else {
                              break;
                            }
                        }
                    } while(*myPath); 
                }
            }
            
            if(res != RES_BOGUS) {
              
                if(uprv_strcmp(dataEntry->fName, uloc_getDefault())==0 || uprv_strcmp(dataEntry->fName, kRootLocaleName)==0) {
                    *status = U_USING_DEFAULT_WARNING;
                } else {
                    *status = U_USING_FALLBACK_WARNING;
                }

                fillIn = init_resb_result(&(dataEntry->fData), res, inKey, -1, dataEntry, resB, 0, fillIn, status);
            } else {
                *status = U_MISSING_RESOURCE_ERROR;
            }
        } else {
            fillIn = init_resb_result(&(resB->fResData), res, key, -1, resB->fData, resB, 0, fillIn, status);
        }
    } 
    else {
        *status = U_RESOURCE_TYPE_MISMATCH;
    }
    ures_close(helper);
    return fillIn;
}


U_CAPI UResourceBundle* U_EXPORT2 ures_getByKey(const UResourceBundle *resB, const char* inKey, UResourceBundle *fillIn, UErrorCode *status) {
    Resource res = RES_BOGUS;
    UResourceDataEntry *realData = NULL;
    const char *key = inKey;

    if (status==NULL || U_FAILURE(*status)) {
        return fillIn;
    }
    if(resB == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return fillIn;
    }

    int32_t type = RES_GET_TYPE(resB->fRes);
    if(URES_IS_TABLE(type)) {
        int32_t t;
        res = res_getTableItemByKey(&(resB->fResData), resB->fRes, &t, &key);
        if(res == RES_BOGUS) {
            key = inKey;
            if(resB->fHasFallback == TRUE) {
                const ResourceData *rd = getFallbackData(resB, &key, &realData, &res, status);
                if(U_SUCCESS(*status)) {
                  
                    return init_resb_result(rd, res, key, -1, realData, resB, 0, fillIn, status);
                } else {
                    *status = U_MISSING_RESOURCE_ERROR;
                }
            } else {
                *status = U_MISSING_RESOURCE_ERROR;
            }
        } else {
            return init_resb_result(&(resB->fResData), res, key, -1, resB->fData, resB, 0, fillIn, status);
        }
    } 
#if 0
    
    
    else if(RES_GET_TYPE(resB->fRes) == URES_ARRAY && resB->fHasFallback == TRUE) {
        
        const ResourceData *rd = getFallbackData(resB, &key, &realData, &res, status);
        if(U_SUCCESS(*status)) {
            return init_resb_result(rd, res, key, realData, resB, fillIn, status);
        } else {
            *status = U_MISSING_RESOURCE_ERROR;
        }
    }
#endif    
    else {
        *status = U_RESOURCE_TYPE_MISMATCH;
    }
    return fillIn;
}

U_CAPI const UChar* U_EXPORT2 ures_getStringByKey(const UResourceBundle *resB, const char* inKey, int32_t* len, UErrorCode *status) {
    Resource res = RES_BOGUS;
    UResourceDataEntry *realData = NULL;
    const char* key = inKey;

    if (status==NULL || U_FAILURE(*status)) {
        return NULL;
    }
    if(resB == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    int32_t type = RES_GET_TYPE(resB->fRes);
    if(URES_IS_TABLE(type)) {
        int32_t t=0;

        res = res_getTableItemByKey(&(resB->fResData), resB->fRes, &t, &key);

        if(res == RES_BOGUS) {
            key = inKey;
            if(resB->fHasFallback == TRUE) {
                const ResourceData *rd = getFallbackData(resB, &key, &realData, &res, status);
                if(U_SUCCESS(*status)) {
                    switch (RES_GET_TYPE(res)) {
                    case URES_STRING:
                    case URES_STRING_V2:
                        return res_getString(rd, res, len);
                    case URES_ALIAS:
                      {
                        const UChar* result = 0;
                        UResourceBundle *tempRes = ures_getByKey(resB, inKey, NULL, status);
                        result = ures_getString(tempRes, len, status);
                        ures_close(tempRes);
                        return result;
                      }
                    default:
                        *status = U_RESOURCE_TYPE_MISMATCH;
                    }
                } else {
                    *status = U_MISSING_RESOURCE_ERROR;
                }
            } else {
                *status = U_MISSING_RESOURCE_ERROR;
            }
        } else {
            switch (RES_GET_TYPE(res)) {
            case URES_STRING:
            case URES_STRING_V2:
                return res_getString(&(resB->fResData), res, len);
            case URES_ALIAS:
              {
                const UChar* result = 0;
                UResourceBundle *tempRes = ures_getByKey(resB, inKey, NULL, status);
                result = ures_getString(tempRes, len, status);
                ures_close(tempRes);
                return result;
              }
            default:
                *status = U_RESOURCE_TYPE_MISMATCH;
            }
        }
    } 
#if 0 
    
       
    else if(RES_GET_TYPE(resB->fRes) == URES_ARRAY && resB->fHasFallback == TRUE) {
        
        const ResourceData *rd = getFallbackData(resB, &key, &realData, &res, status);
        if(U_SUCCESS(*status)) {
            return res_getString(rd, res, len);
        } else {
            *status = U_MISSING_RESOURCE_ERROR;
        }
    } 
#endif    
    else {
        *status = U_RESOURCE_TYPE_MISMATCH;
    }
    return NULL;
}

U_CAPI const char * U_EXPORT2
ures_getUTF8StringByKey(const UResourceBundle *resB,
                        const char *key,
                        char *dest, int32_t *pLength,
                        UBool forceCopy,
                        UErrorCode *status) {
    int32_t length16;
    const UChar *s16 = ures_getStringByKey(resB, key, &length16, status);
    return ures_toUTF8String(s16, length16, dest, pLength, forceCopy, status);
}







U_INTERNAL const char*  U_EXPORT2
ures_getLocaleInternal(const UResourceBundle* resourceBundle, UErrorCode* status)
{
    if (status==NULL || U_FAILURE(*status)) {
        return NULL;
    }
    if (!resourceBundle) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    } else {
      return resourceBundle->fData->fName;
    }
}

U_CAPI const char* U_EXPORT2 
ures_getLocale(const UResourceBundle* resourceBundle, 
               UErrorCode* status)
{
  return ures_getLocaleInternal(resourceBundle, status);
}


U_CAPI const char* U_EXPORT2 
ures_getLocaleByType(const UResourceBundle* resourceBundle, 
                     ULocDataLocaleType type, 
                     UErrorCode* status) {
    if (status==NULL || U_FAILURE(*status)) {
        return NULL;
    }
    if (!resourceBundle) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    } else {
        switch(type) {
        case ULOC_ACTUAL_LOCALE:
            return resourceBundle->fData->fName;
        case ULOC_VALID_LOCALE:
            return resourceBundle->fTopLevelData->fName;
        case ULOC_REQUESTED_LOCALE:
        default:
            *status = U_ILLEGAL_ARGUMENT_ERROR;
            return NULL;
        }
    }
}

U_CFUNC const char* ures_getName(const UResourceBundle* resB) {
  if(resB == NULL) {
    return NULL;
  }

  return resB->fData->fName;
}

#ifdef URES_DEBUG
U_CFUNC const char* ures_getPath(const UResourceBundle* resB) {
  if(resB == NULL) {
    return NULL;
  }

  return resB->fData->fPath;
}
#endif

static UResourceBundle*
ures_openWithType(UResourceBundle *r, const char* path, const char* localeID,
                  UResOpenType openType, UErrorCode* status) {
    if(U_FAILURE(*status)) {
        return NULL;
    }

    UResourceDataEntry *entry;
    if(openType != URES_OPEN_DIRECT) {
        
        char canonLocaleID[ULOC_FULLNAME_CAPACITY];
        uloc_getBaseName(localeID, canonLocaleID, UPRV_LENGTHOF(canonLocaleID), status);
        if(U_FAILURE(*status) || *status == U_STRING_NOT_TERMINATED_WARNING) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
            return NULL;
        }
        entry = entryOpen(path, canonLocaleID, openType, status);
    } else {
        entry = entryOpenDirect(path, localeID, status);
    }
    if(U_FAILURE(*status)) {
        return NULL;
    }
    if(entry == NULL) {
        *status = U_MISSING_RESOURCE_ERROR;
        return NULL;
    }

    UBool isStackObject;
    if(r == NULL) {
        r = (UResourceBundle *)uprv_malloc(sizeof(UResourceBundle));
        if(r == NULL) {
            entryClose(entry);
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        isStackObject = FALSE;
    } else {  
        isStackObject = ures_isStackObject(r);
        ures_closeBundle(r, FALSE);
    }
    uprv_memset(r, 0, sizeof(UResourceBundle));
    ures_setIsStackObject(r, isStackObject);

    r->fTopLevelData = r->fData = entry;
    uprv_memcpy(&r->fResData, &entry->fData, sizeof(ResourceData));
    r->fHasFallback = openType != URES_OPEN_DIRECT && !r->fResData.noFallback;
    r->fIsTopLevel = TRUE;
    r->fRes = r->fResData.rootRes;
    r->fSize = res_countArrayItems(&(r->fResData), r->fRes);
    r->fIndex = -1;

    return r;
}

U_CAPI UResourceBundle* U_EXPORT2
ures_open(const char* path, const char* localeID, UErrorCode* status) {
    return ures_openWithType(NULL, path, localeID, URES_OPEN_LOCALE_DEFAULT_ROOT, status);
}

U_CAPI UResourceBundle* U_EXPORT2
ures_openNoDefault(const char* path, const char* localeID, UErrorCode* status) {
    return ures_openWithType(NULL, path, localeID, URES_OPEN_LOCALE_ROOT, status);
}





U_CAPI UResourceBundle*  U_EXPORT2
ures_openDirect(const char* path, const char* localeID, UErrorCode* status) {
    return ures_openWithType(NULL, path, localeID, URES_OPEN_DIRECT, status);
}






U_CAPI void U_EXPORT2
ures_openFillIn(UResourceBundle *r, const char* path,
                const char* localeID, UErrorCode* status) {
    if(U_SUCCESS(*status) && r == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    ures_openWithType(r, path, localeID, URES_OPEN_LOCALE_DEFAULT_ROOT, status);
}





U_CAPI int32_t  U_EXPORT2
ures_countArrayItems(const UResourceBundle* resourceBundle,
                  const char* resourceKey,
                  UErrorCode* status)
{
    UResourceBundle resData;
    ures_initStackObject(&resData);
    if (status==NULL || U_FAILURE(*status)) {
        return 0;
    }
    if(resourceBundle == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    ures_getByKey(resourceBundle, resourceKey, &resData, status);
    
    if(resData.fResData.data != NULL) {
        int32_t result = res_countArrayItems(&resData.fResData, resData.fRes);
        ures_close(&resData);
        return result;
    } else {
        *status = U_MISSING_RESOURCE_ERROR;
        ures_close(&resData);
        return 0;
    }
}











U_INTERNAL const char* U_EXPORT2 
ures_getVersionNumberInternal(const UResourceBundle *resourceBundle)
{
    if (!resourceBundle) return NULL;

    if(resourceBundle->fVersion == NULL) {

        
        
        UErrorCode status = U_ZERO_ERROR;
        int32_t minor_len = 0;
        int32_t len;

        const UChar* minor_version = ures_getStringByKey(resourceBundle, kVersionTag, &minor_len, &status);
        
        
        
        
        

        len = (minor_len > 0) ? minor_len : 1;
    
        
        


        ((UResourceBundle *)resourceBundle)->fVersion = (char *)uprv_malloc(1 + len); 
        
        if (((UResourceBundle *)resourceBundle)->fVersion == NULL) {
            return NULL;
        }
       
        if(minor_len > 0) {
            u_UCharsToChars(minor_version, resourceBundle->fVersion , minor_len);
            resourceBundle->fVersion[len] =  '\0';
        }
        else {
            uprv_strcpy(resourceBundle->fVersion, kDefaultMinorVersion);
        }
    }

    return resourceBundle->fVersion;
}

U_CAPI const char*  U_EXPORT2
ures_getVersionNumber(const UResourceBundle*   resourceBundle)
{
    return ures_getVersionNumberInternal(resourceBundle);
}

U_CAPI void U_EXPORT2 ures_getVersion(const UResourceBundle* resB, UVersionInfo versionInfo) {
    if (!resB) return;

    u_versionFromString(versionInfo, ures_getVersionNumberInternal(resB));
}


#define INDEX_LOCALE_NAME "res_index"
#define INDEX_TAG         "InstalledLocales"
#define DEFAULT_TAG       "default"

#if defined(URES_TREE_DEBUG)
#include <stdio.h>
#endif

typedef struct ULocalesContext {
    UResourceBundle installed;
    UResourceBundle curr;
} ULocalesContext;

static void U_CALLCONV
ures_loc_closeLocales(UEnumeration *enumerator) {
    ULocalesContext *ctx = (ULocalesContext *)enumerator->context;
    ures_close(&ctx->curr);
    ures_close(&ctx->installed);
    uprv_free(ctx);
    uprv_free(enumerator);
}

static int32_t U_CALLCONV
ures_loc_countLocales(UEnumeration *en, UErrorCode * ) {
    ULocalesContext *ctx = (ULocalesContext *)en->context;
    return ures_getSize(&ctx->installed);
}

static const char* U_CALLCONV 
ures_loc_nextLocale(UEnumeration* en,
                    int32_t* resultLength,
                    UErrorCode* status) {
    ULocalesContext *ctx = (ULocalesContext *)en->context;
    UResourceBundle *res = &(ctx->installed);
    UResourceBundle *k = NULL;
    const char *result = NULL;
    int32_t len = 0;
    if(ures_hasNext(res) && (k = ures_getNextResource(res, &ctx->curr, status))) {
        result = ures_getKey(k);
        len = (int32_t)uprv_strlen(result);
    }
    if (resultLength) {
        *resultLength = len;
    }
    return result;
}

static void U_CALLCONV 
ures_loc_resetLocales(UEnumeration* en, 
                      UErrorCode* ) {
    UResourceBundle *res = &((ULocalesContext *)en->context)->installed;
    ures_resetIterator(res);
}


static const UEnumeration gLocalesEnum = {
    NULL,
        NULL,
        ures_loc_closeLocales,
        ures_loc_countLocales,
        uenum_unextDefault,
        ures_loc_nextLocale,
        ures_loc_resetLocales
};


U_CAPI UEnumeration* U_EXPORT2
ures_openAvailableLocales(const char *path, UErrorCode *status)
{
    UResourceBundle *idx = NULL;
    UEnumeration *en = NULL;
    ULocalesContext *myContext = NULL;

    if(U_FAILURE(*status)) {
        return NULL;
    }
    myContext = static_cast<ULocalesContext *>(uprv_malloc(sizeof(ULocalesContext)));
    en =  (UEnumeration *)uprv_malloc(sizeof(UEnumeration));
    if(!en || !myContext) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(en);
        uprv_free(myContext);
        return NULL;
    }
    uprv_memcpy(en, &gLocalesEnum, sizeof(UEnumeration));

    ures_initStackObject(&myContext->installed);
    ures_initStackObject(&myContext->curr);
    idx = ures_openDirect(path, INDEX_LOCALE_NAME, status);
    ures_getByKey(idx, INDEX_TAG, &myContext->installed, status);
    if(U_SUCCESS(*status)) {
#if defined(URES_TREE_DEBUG)
        fprintf(stderr, "Got %s::%s::[%s] : %s\n", 
            path, INDEX_LOCALE_NAME, INDEX_TAG, ures_getKey(&myContext->installed));
#endif
        en->context = myContext;
    } else {
#if defined(URES_TREE_DEBUG)
        fprintf(stderr, "%s open failed - %s\n", path, u_errorName(*status));
#endif
        ures_close(&myContext->installed);
        uprv_free(myContext);
        uprv_free(en);
        en = NULL;
    }
    
    ures_close(idx);
    
    return en;
}

static UBool isLocaleInList(UEnumeration *locEnum, const char *locToSearch, UErrorCode *status) {
    const char *loc;
    while ((loc = uenum_next(locEnum, NULL, status)) != NULL) {
        if (uprv_strcmp(loc, locToSearch) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

U_CAPI int32_t U_EXPORT2
ures_getFunctionalEquivalent(char *result, int32_t resultCapacity,
                             const char *path, const char *resName, const char *keyword, const char *locid,
                             UBool *isAvailable, UBool omitDefault, UErrorCode *status)
{
    char kwVal[1024] = ""; 
    char defVal[1024] = ""; 
    char defLoc[1024] = ""; 
    char base[1024] = ""; 
    char found[1024];
    char parent[1024];
    char full[1024] = "";
    UResourceBundle bund1, bund2;
    UResourceBundle *res = NULL;
    UErrorCode subStatus = U_ZERO_ERROR;
    int32_t length = 0;
    if(U_FAILURE(*status)) return 0;
    uloc_getKeywordValue(locid, keyword, kwVal, 1024-1,&subStatus);
    if(!uprv_strcmp(kwVal, DEFAULT_TAG)) {
        kwVal[0]=0;
    }
    uloc_getBaseName(locid, base, 1024-1,&subStatus);
#if defined(URES_TREE_DEBUG)
    fprintf(stderr, "getFunctionalEquivalent: \"%s\" [%s=%s] in %s - %s\n", 
            locid, keyword, kwVal, base, u_errorName(subStatus));
#endif
    ures_initStackObject(&bund1);
    ures_initStackObject(&bund2);
    
    
    uprv_strcpy(parent, base);
    uprv_strcpy(found, base);

    if(isAvailable) { 
        UEnumeration *locEnum = ures_openAvailableLocales(path, &subStatus);
        *isAvailable = TRUE;
        if (U_SUCCESS(subStatus)) {
            *isAvailable = isLocaleInList(locEnum, parent, &subStatus);
        }
        uenum_close(locEnum);
    }

    if(U_FAILURE(subStatus)) {
        *status = subStatus;
        return 0;
    }
    
    do {
        subStatus = U_ZERO_ERROR;
        res = ures_open(path, parent, &subStatus);
        if(((subStatus == U_USING_FALLBACK_WARNING) ||
            (subStatus == U_USING_DEFAULT_WARNING)) && isAvailable)
        {
            *isAvailable = FALSE;
        }
        isAvailable = NULL; 
        
#if defined(URES_TREE_DEBUG)
        fprintf(stderr, "%s;%s -> %s [%s]\n", path?path:"ICUDATA", parent, u_errorName(subStatus), ures_getLocale(res, &subStatus));
#endif
        if(U_FAILURE(subStatus)) {
            *status = subStatus;
        } else if(subStatus == U_ZERO_ERROR) {
            ures_getByKey(res,resName,&bund1, &subStatus);
            if(subStatus == U_ZERO_ERROR) {
                const UChar *defUstr;
                int32_t defLen;
                
#if defined(URES_TREE_DEBUG)
                fprintf(stderr, "%s;%s : loaded default -> %s\n",
                    path?path:"ICUDATA", parent, u_errorName(subStatus));
#endif
                defUstr = ures_getStringByKey(&bund1, DEFAULT_TAG, &defLen, &subStatus);
                if(U_SUCCESS(subStatus) && defLen) {
                    u_UCharsToChars(defUstr, defVal, u_strlen(defUstr));
#if defined(URES_TREE_DEBUG)
                    fprintf(stderr, "%s;%s -> default %s=%s,  %s\n", 
                        path?path:"ICUDATA", parent, keyword, defVal, u_errorName(subStatus));
#endif
                    uprv_strcpy(defLoc, parent);
                    if(kwVal[0]==0) {
                        uprv_strcpy(kwVal, defVal);
#if defined(URES_TREE_DEBUG)
                        fprintf(stderr, "%s;%s -> kwVal =  %s\n", 
                            path?path:"ICUDATA", parent, keyword, kwVal);
#endif
                    }
                }
            }
        }
        
        subStatus = U_ZERO_ERROR;

        if (res != NULL) {
            uprv_strcpy(found, ures_getLocaleByType(res, ULOC_VALID_LOCALE, &subStatus));
        }

        uloc_getParent(found,parent,sizeof(parent),&subStatus);
        ures_close(res);
    } while(!defVal[0] && *found && uprv_strcmp(found, "root") != 0 && U_SUCCESS(*status));
    
    
    uprv_strcpy(parent, base);
    uprv_strcpy(found, base);
    
    do {
        subStatus = U_ZERO_ERROR;
        res = ures_open(path, parent, &subStatus);
        if((subStatus == U_USING_FALLBACK_WARNING) && isAvailable) {
            *isAvailable = FALSE;
        }
        isAvailable = NULL; 
        
#if defined(URES_TREE_DEBUG)
        fprintf(stderr, "%s;%s -> %s (looking for %s)\n", 
            path?path:"ICUDATA", parent, u_errorName(subStatus), kwVal);
#endif
        if(U_FAILURE(subStatus)) {
            *status = subStatus;
        } else if(subStatus == U_ZERO_ERROR) {
            ures_getByKey(res,resName,&bund1, &subStatus);
#if defined(URES_TREE_DEBUG)
 fprintf(stderr,"@%d [%s] %s\n", __LINE__, resName, u_errorName(subStatus));
#endif
            if(subStatus == U_ZERO_ERROR) {
                ures_getByKey(&bund1, kwVal, &bund2, &subStatus);
#if defined(URES_TREE_DEBUG)
 fprintf(stderr,"@%d [%s] %s\n", __LINE__, kwVal, u_errorName(subStatus));
#endif
                if(subStatus == U_ZERO_ERROR) {
#if defined(URES_TREE_DEBUG)
                    fprintf(stderr, "%s;%s -> full0 %s=%s,  %s\n", 
                        path?path:"ICUDATA", parent, keyword, kwVal, u_errorName(subStatus));
#endif
                    uprv_strcpy(full, parent);
                    if(*full == 0) {
                        uprv_strcpy(full, "root");
                    }
                        
                        if(uprv_strlen(defLoc) > uprv_strlen(full)) {
                          const UChar *defUstr;
                          int32_t defLen;
                          
#if defined(URES_TREE_DEBUG)
                            fprintf(stderr, "%s;%s -> recalculating Default0\n", 
                                    path?path:"ICUDATA", full);
#endif
                          defUstr = ures_getStringByKey(&bund1, DEFAULT_TAG, &defLen, &subStatus);
                          if(U_SUCCESS(subStatus) && defLen) {
                            u_UCharsToChars(defUstr, defVal, u_strlen(defUstr));
#if defined(URES_TREE_DEBUG)
                            fprintf(stderr, "%s;%s -> default0 %s=%s,  %s\n", 
                                    path?path:"ICUDATA", full, keyword, defVal, u_errorName(subStatus));
#endif
                            uprv_strcpy(defLoc, full);
                          }
                        } 
#if defined(URES_TREE_DEBUG)
                        else {
                          fprintf(stderr, "No trim0,  %s <= %s\n", defLoc, full);
                        }
#endif
                } else {
#if defined(URES_TREE_DEBUG)
                    fprintf(stderr, "err=%s in %s looking for %s\n", 
                        u_errorName(subStatus), parent, kwVal);
#endif
                }
            }
        }
        
        subStatus = U_ZERO_ERROR;
        
        uprv_strcpy(found, parent);
        uloc_getParent(found,parent,1023,&subStatus);
        ures_close(res);
    } while(!full[0] && *found && U_SUCCESS(*status));
    
    if((full[0]==0) && uprv_strcmp(kwVal, defVal)) {
#if defined(URES_TREE_DEBUG)
        fprintf(stderr, "Failed to locate kw %s - try default %s\n", kwVal, defVal);
#endif
        uprv_strcpy(kwVal, defVal);
        uprv_strcpy(parent, base);
        uprv_strcpy(found, base);
        
        do { 
            subStatus = U_ZERO_ERROR;
            res = ures_open(path, parent, &subStatus);
            if((subStatus == U_USING_FALLBACK_WARNING) && isAvailable) {
                *isAvailable = FALSE;
            }
            isAvailable = NULL; 
            
#if defined(URES_TREE_DEBUG)
            fprintf(stderr, "%s;%s -> %s (looking for default %s)\n",
                path?path:"ICUDATA", parent, u_errorName(subStatus), kwVal);
#endif
            if(U_FAILURE(subStatus)) {
                *status = subStatus;
            } else if(subStatus == U_ZERO_ERROR) {
                ures_getByKey(res,resName,&bund1, &subStatus);
                if(subStatus == U_ZERO_ERROR) {
                    ures_getByKey(&bund1, kwVal, &bund2, &subStatus);
                    if(subStatus == U_ZERO_ERROR) {
#if defined(URES_TREE_DEBUG)
                        fprintf(stderr, "%s;%s -> full1 %s=%s,  %s\n", path?path:"ICUDATA",
                            parent, keyword, kwVal, u_errorName(subStatus));
#endif
                        uprv_strcpy(full, parent);
                        if(*full == 0) {
                            uprv_strcpy(full, "root");
                        }
                        
                        
                        if(uprv_strlen(defLoc) > uprv_strlen(full)) {
                          const UChar *defUstr;
                          int32_t defLen;
                          
#if defined(URES_TREE_DEBUG)
                            fprintf(stderr, "%s;%s -> recalculating Default1\n", 
                                    path?path:"ICUDATA", full);
#endif
                          defUstr = ures_getStringByKey(&bund1, DEFAULT_TAG, &defLen, &subStatus);
                          if(U_SUCCESS(subStatus) && defLen) {
                            u_UCharsToChars(defUstr, defVal, u_strlen(defUstr));
#if defined(URES_TREE_DEBUG)
                            fprintf(stderr, "%s;%s -> default %s=%s,  %s\n", 
                                    path?path:"ICUDATA", full, keyword, defVal, u_errorName(subStatus));
#endif
                            uprv_strcpy(defLoc, full);
                          }
                        } 
#if defined(URES_TREE_DEBUG)
                        else {
                          fprintf(stderr, "No trim1,  %s <= %s\n", defLoc, full);
                        }
#endif
                    }
                }
            }
            subStatus = U_ZERO_ERROR;
            
            uprv_strcpy(found, parent);
            uloc_getParent(found,parent,1023,&subStatus);
            ures_close(res);
        } while(!full[0] && *found && U_SUCCESS(*status));
    }
    
    if(U_SUCCESS(*status)) {
        if(!full[0]) {
#if defined(URES_TREE_DEBUG)
          fprintf(stderr, "Still could not load keyword %s=%s\n", keyword, kwVal);
#endif
          *status = U_MISSING_RESOURCE_ERROR;
        } else if(omitDefault) {
#if defined(URES_TREE_DEBUG)
          fprintf(stderr,"Trim? full=%s, defLoc=%s, found=%s\n", full, defLoc, found);
#endif        
          if(uprv_strlen(defLoc) <= uprv_strlen(full)) {
            
            if(!uprv_strcmp(kwVal, defVal)) { 
              
#if defined(URES_TREE_DEBUG)
              fprintf(stderr, "Removing unneeded var %s=%s\n", keyword, kwVal);
#endif
              kwVal[0]=0;
            }
          }
        }
        uprv_strcpy(found, full);
        if(kwVal[0]) {
            uprv_strcat(found, "@");
            uprv_strcat(found, keyword);
            uprv_strcat(found, "=");
            uprv_strcat(found, kwVal);
        } else if(!omitDefault) {
            uprv_strcat(found, "@");
            uprv_strcat(found, keyword);
            uprv_strcat(found, "=");
            uprv_strcat(found, defVal);
        }
    }
    
    
    ures_close(&bund1);
    ures_close(&bund2);
    
    length = (int32_t)uprv_strlen(found);

    if(U_SUCCESS(*status)) {
        int32_t copyLength = uprv_min(length, resultCapacity);
        if(copyLength>0) {
            uprv_strncpy(result, found, copyLength);
        }
        if(length == 0) {
          *status = U_MISSING_RESOURCE_ERROR; 
        }
    } else {
        length = 0;
        result[0]=0;
    }
    return u_terminateChars(result, resultCapacity, length, status);
}

U_CAPI UEnumeration* U_EXPORT2
ures_getKeywordValues(const char *path, const char *keyword, UErrorCode *status)
{
#define VALUES_BUF_SIZE 2048
#define VALUES_LIST_SIZE 512
    
    char       valuesBuf[VALUES_BUF_SIZE];
    int32_t    valuesIndex = 0;
    const char *valuesList[VALUES_LIST_SIZE];
    int32_t    valuesCount = 0;
    
    const char *locale;
    int32_t     locLen;
    
    UEnumeration *locs = NULL;
    
    UResourceBundle    item;
    UResourceBundle    subItem;
    
    ures_initStackObject(&item);
    ures_initStackObject(&subItem);
    locs = ures_openAvailableLocales(path, status);
    
    if(U_FAILURE(*status)) {
        ures_close(&item);
        ures_close(&subItem);
        return NULL;
    }
    
    valuesBuf[0]=0;
    valuesBuf[1]=0;
    
    while((locale = uenum_next(locs, &locLen, status))) {
        UResourceBundle   *bund = NULL;
        UResourceBundle   *subPtr = NULL;
        UErrorCode subStatus = U_ZERO_ERROR; 
        bund = ures_openDirect(path, locale, &subStatus);
        
#if defined(URES_TREE_DEBUG)
        if(!bund || U_FAILURE(subStatus)) {
            fprintf(stderr, "%s-%s values: Can't open %s locale - skipping. (%s)\n", 
                path?path:"<ICUDATA>", keyword, locale, u_errorName(subStatus));
        }
#endif
        
        ures_getByKey(bund, keyword, &item, &subStatus);
        
        if(!bund || U_FAILURE(subStatus)) {
#if defined(URES_TREE_DEBUG)
            fprintf(stderr, "%s-%s values: Can't find in %s - skipping. (%s)\n", 
                path?path:"<ICUDATA>", keyword, locale, u_errorName(subStatus));
#endif
            ures_close(bund);
            bund = NULL;
            continue;
        }
        
        while((subPtr = ures_getNextResource(&item,&subItem,&subStatus))
            && U_SUCCESS(subStatus)) {
            const char *k;
            int32_t i;
            k = ures_getKey(subPtr);

#if defined(URES_TREE_DEBUG)
            
#endif
            if(k == NULL || *k == 0 ||
                    uprv_strcmp(k, DEFAULT_TAG) == 0 || uprv_strncmp(k, "private-", 8) == 0) {
                
                continue;
            }
            for(i=0; i<valuesCount; i++) {
                if(!uprv_strcmp(valuesList[i],k)) {
                    k = NULL; 
                    break;
                }
            }
            if(k != NULL) {
                int32_t kLen = (int32_t)uprv_strlen(k);
                if((valuesCount >= (VALUES_LIST_SIZE-1)) ||       
                    ((valuesIndex+kLen+1+1) >= VALUES_BUF_SIZE)) { 
                    *status = U_ILLEGAL_ARGUMENT_ERROR; 
                } else {
                    uprv_strcpy(valuesBuf+valuesIndex, k);
                    valuesList[valuesCount++] = valuesBuf+valuesIndex;
                    valuesIndex += kLen;
#if defined(URES_TREE_DEBUG)
                    fprintf(stderr, "%s | %s | %s | [%s]   (UNIQUE)\n",
                        path?path:"<ICUDATA>", keyword, locale, k);
#endif
                    valuesBuf[valuesIndex++] = 0; 
                }
            }
        }
        ures_close(bund);
    }
    valuesBuf[valuesIndex++] = 0; 
    
    ures_close(&item);
    ures_close(&subItem);
    uenum_close(locs);
#if defined(URES_TREE_DEBUG)
    fprintf(stderr, "%s:  size %d, #%d\n", u_errorName(*status), 
        valuesIndex, valuesCount);
#endif
    return uloc_openKeywordList(valuesBuf, valuesIndex, status);
}
#if 0

U_INTERNAL UBool U_EXPORT2
ures_equal(const UResourceBundle* res1, const UResourceBundle* res2){
    if(res1==NULL || res2==NULL){
        return res1==res2; 
    }
    if(res1->fKey==NULL||  res2->fKey==NULL){
        return (res1->fKey==res2->fKey);
    }else{
        if(uprv_strcmp(res1->fKey, res2->fKey)!=0){
            return FALSE;
        }
    }
    if(uprv_strcmp(res1->fData->fName, res2->fData->fName)!=0){
        return FALSE;
    }
    if(res1->fData->fPath == NULL||  res2->fData->fPath==NULL){
        return (res1->fData->fPath == res2->fData->fPath);
    }else{
        if(uprv_strcmp(res1->fData->fPath, res2->fData->fPath)!=0){
            return FALSE;
        }
    }
    if(uprv_strcmp(res1->fData->fParent->fName, res2->fData->fParent->fName)!=0){
        return FALSE;
    }
    if(uprv_strcmp(res1->fData->fParent->fPath, res2->fData->fParent->fPath)!=0){
        return FALSE;
    }
    if(uprv_strncmp(res1->fResPath, res2->fResPath, res1->fResPathLen)!=0){
        return FALSE;
    }
    if(res1->fRes != res2->fRes){
        return FALSE;
    }
    return TRUE;
}
U_INTERNAL UResourceBundle* U_EXPORT2
ures_clone(const UResourceBundle* res, UErrorCode* status){
    UResourceBundle* bundle = NULL;
    UResourceBundle* ret = NULL;
    if(U_FAILURE(*status) || res == NULL){
        return NULL;
    }
    bundle = ures_open(res->fData->fPath, res->fData->fName, status);
    if(res->fResPath!=NULL){
        ret = ures_findSubResource(bundle, res->fResPath, NULL, status);
        ures_close(bundle);
    }else{
        ret = bundle;
    }
    return ret;
}
U_INTERNAL const UResourceBundle* U_EXPORT2
ures_getParentBundle(const UResourceBundle* res){
    if(res==NULL){
        return NULL;
    }
    return res->fParentRes;
}
#endif

U_INTERNAL void U_EXPORT2
ures_getVersionByKey(const UResourceBundle* res, const char *key, UVersionInfo ver, UErrorCode *status) {
  const UChar *str;
  int32_t len;
  str = ures_getStringByKey(res, key, &len, status);
  if(U_SUCCESS(*status)) {
    u_versionFromUString(ver, str);
  } 
}


