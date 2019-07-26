















#include "unicode/utypes.h"  

#ifdef __GNUC__




#endif

#include "unicode/putil.h"
#include "unicode/udata.h"
#include "unicode/uversion.h"
#include "charstr.h"
#include "cmemory.h"
#include "cstring.h"
#include "putilimp.h"
#include "ucln_cmn.h"
#include "ucmndata.h"
#include "udatamem.h"
#include "uhash.h"
#include "umapfile.h"
#include "umutex.h"






























#if defined(UDATA_DEBUG)
#   include <stdio.h>
#endif

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

U_NAMESPACE_USE




static UDataMemory *udata_findCachedData(const char *path);























static UDataMemory *gCommonICUDataArray[10] = { NULL };

static UBool gHaveTriedToLoadCommonData = FALSE;  

static UHashtable  *gCommonDataCache = NULL;  

static UDataFileAccess  gDataFileAccess = UDATA_DEFAULT_ACCESS;

static UBool U_CALLCONV
udata_cleanup(void)
{
    int32_t i;

    if (gCommonDataCache) {             
        uhash_close(gCommonDataCache);  
        gCommonDataCache = NULL;        
    }

    for (i = 0; i < LENGTHOF(gCommonICUDataArray) && gCommonICUDataArray[i] != NULL; ++i) {
        udata_close(gCommonICUDataArray[i]);
        gCommonICUDataArray[i] = NULL;
    }
    gHaveTriedToLoadCommonData = FALSE;

    return TRUE;                   
}

static UBool U_CALLCONV
findCommonICUDataByName(const char *inBasename)
{
    UBool found = FALSE;
    int32_t i;

    UDataMemory  *pData = udata_findCachedData(inBasename);
    if (pData == NULL)
        return FALSE;

    for (i = 0; i < LENGTHOF(gCommonICUDataArray); ++i) {
        if ((gCommonICUDataArray[i] != NULL) && (gCommonICUDataArray[i]->pHeader == pData->pHeader)) {
            
            found = TRUE;
            break;
        }
    }

    return found;
}





static UBool
setCommonICUData(UDataMemory *pData,     
                 UBool       warn,       
                                         
                 UErrorCode *pErr)
{
    UDataMemory  *newCommonData = UDataMemory_createNewInstance(pErr);
    int32_t i;
    UBool didUpdate = FALSE;
    if (U_FAILURE(*pErr)) {
        return FALSE;
    }

    
    
    
    
    UDatamemory_assign(newCommonData, pData);
    umtx_lock(NULL);
    for (i = 0; i < LENGTHOF(gCommonICUDataArray); ++i) {
        if (gCommonICUDataArray[i] == NULL) {
            gCommonICUDataArray[i] = newCommonData;
            ucln_common_registerCleanup(UCLN_COMMON_UDATA, udata_cleanup);
            didUpdate = TRUE;
            break;
        } else if (gCommonICUDataArray[i]->pHeader == pData->pHeader) {
            
            break;
        }
    }
    umtx_unlock(NULL);

    if (i == LENGTHOF(gCommonICUDataArray) && warn) {
        *pErr = U_USING_DEFAULT_WARNING;
    }
    if (!didUpdate) {
        uprv_free(newCommonData);
    }
    return didUpdate;
}

static UBool
setCommonICUDataPointer(const void *pData, UBool , UErrorCode *pErrorCode) {
    UDataMemory tData;
    UDataMemory_init(&tData);
    tData.pHeader = (const DataHeader *)pData;
    udata_checkCommonData(&tData, pErrorCode);
    return setCommonICUData(&tData, FALSE, pErrorCode);
}

static const char *
findBasename(const char *path) {
    const char *basename=uprv_strrchr(path, U_FILE_SEP_CHAR);
    if(basename==NULL) {
        return path;
    } else {
        return basename+1;
    }
}

#ifdef UDATA_DEBUG
static const char *
packageNameFromPath(const char *path)
{
    if((path == NULL) || (*path == 0)) {
        return U_ICUDATA_NAME;
    }

    path = findBasename(path);

    if((path == NULL) || (*path == 0)) {
        return U_ICUDATA_NAME;
    }

    return path;
}
#endif













typedef struct DataCacheElement {
    char          *name;
    UDataMemory   *item;
} DataCacheElement;








static void U_CALLCONV DataCacheElement_deleter(void *pDCEl) {
    DataCacheElement *p = (DataCacheElement *)pDCEl;
    udata_close(p->item);              
    uprv_free(p->name);                
    uprv_free(pDCEl);                  
}

 



static UHashtable *udata_getHashTable() {
    UErrorCode   err = U_ZERO_ERROR;
    UBool        cacheIsInitialized;
    UHashtable  *tHT = NULL;

    UMTX_CHECK(NULL, (gCommonDataCache != NULL), cacheIsInitialized);

    if (cacheIsInitialized) {
        return gCommonDataCache;
    }

    tHT = uhash_open(uhash_hashChars, uhash_compareChars, NULL, &err);
    
    if (tHT == NULL) {
    	return NULL; 
    }
    uhash_setValueDeleter(tHT, DataCacheElement_deleter);

    umtx_lock(NULL);
    if (gCommonDataCache == NULL) {
        gCommonDataCache = tHT;
        tHT = NULL;
        ucln_common_registerCleanup(UCLN_COMMON_UDATA, udata_cleanup);
    }
    umtx_unlock(NULL);
    if (tHT != NULL) {
        uhash_close(tHT);
    }

    if (U_FAILURE(err)) {
        return NULL;      
    }
    return gCommonDataCache;
}



static UDataMemory *udata_findCachedData(const char *path)
{
    UHashtable        *htable;
    UDataMemory       *retVal = NULL;
    DataCacheElement  *el;
    const char        *baseName;

    baseName = findBasename(path);   
    htable = udata_getHashTable();
    umtx_lock(NULL);
    el = (DataCacheElement *)uhash_get(htable, baseName);
    umtx_unlock(NULL);
    if (el != NULL) {
        retVal = el->item;
    }
#ifdef UDATA_DEBUG
    fprintf(stderr, "Cache: [%s] -> %p\n", baseName, retVal);
#endif
    return retVal;
}


static UDataMemory *udata_cacheDataItem(const char *path, UDataMemory *item, UErrorCode *pErr) {
    DataCacheElement *newElement;
    const char       *baseName;
    int32_t           nameLen;
    UHashtable       *htable;
    DataCacheElement *oldValue = NULL;
    UErrorCode        subErr = U_ZERO_ERROR;

    if (U_FAILURE(*pErr)) {
        return NULL;
    }

    


    newElement = (DataCacheElement *)uprv_malloc(sizeof(DataCacheElement));
    if (newElement == NULL) {
        *pErr = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    newElement->item = UDataMemory_createNewInstance(pErr);
    if (U_FAILURE(*pErr)) {
        uprv_free(newElement);
        return NULL;
    }
    UDatamemory_assign(newElement->item, item);

    baseName = findBasename(path);
    nameLen = (int32_t)uprv_strlen(baseName);
    newElement->name = (char *)uprv_malloc(nameLen+1);
    if (newElement->name == NULL) {
        *pErr = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(newElement->item);
        uprv_free(newElement);
        return NULL;
    }
    uprv_strcpy(newElement->name, baseName);

    

    htable = udata_getHashTable();
    umtx_lock(NULL);
    oldValue = (DataCacheElement *)uhash_get(htable, path);
    if (oldValue != NULL) {
        subErr = U_USING_DEFAULT_WARNING;
    }
    else {
        uhash_put(
            htable,
            newElement->name,               
            newElement,                     
            &subErr);
    }
    umtx_unlock(NULL);

#ifdef UDATA_DEBUG
    fprintf(stderr, "Cache: [%s] <<< %p : %s. vFunc=%p\n", newElement->name, 
    newElement->item, u_errorName(subErr), newElement->item->vFuncs);
#endif

    if (subErr == U_USING_DEFAULT_WARNING || U_FAILURE(subErr)) {
        *pErr = subErr; 
        uprv_free(newElement->name);
        uprv_free(newElement->item);
        uprv_free(newElement);
        return oldValue ? oldValue->item : NULL;
    }

    return newElement->item;
}








#define U_DATA_PATHITER_BUFSIZ  128        /* Size of local buffer for paths         */
                                           

U_NAMESPACE_BEGIN

class UDataPathIterator
{
public:
    UDataPathIterator(const char *path, const char *pkg,
                      const char *item, const char *suffix, UBool doCheckLastFour,
                      UErrorCode *pErrorCode);
    const char *next(UErrorCode *pErrorCode);

private:
    const char *path;                              
    const char *nextPath;                          
    const char *basename;                          
    const char *suffix;                            

    uint32_t    basenameLen;                       

    CharString  itemPath;                          
    CharString  pathBuffer;                        
    CharString  packageStub;                       

    UBool       checkLastFour;                     


};










UDataPathIterator::UDataPathIterator(const char *inPath, const char *pkg,
                                     const char *item, const char *inSuffix, UBool doCheckLastFour,
                                     UErrorCode *pErrorCode)
{
#ifdef UDATA_DEBUG
        fprintf(stderr, "SUFFIX1=%s PATH=%s\n", inSuffix, inPath);
#endif
    
    if(inPath == NULL) {
        path = u_getDataDirectory();
    } else {
        path = inPath;
    }

    
    if(pkg != NULL) {
      packageStub.append(U_FILE_SEP_CHAR, *pErrorCode).append(pkg, *pErrorCode);
#ifdef UDATA_DEBUG
      fprintf(stderr, "STUB=%s [%d]\n", packageStub.data(), packageStub.length());
#endif
    }

    
    basename = findBasename(item);
    basenameLen = (int32_t)uprv_strlen(basename);

    
    if(basename == item) {
        nextPath = path;
    } else {
        itemPath.append(item, (int32_t)(basename-item), *pErrorCode);
        nextPath = itemPath.data();
    }
#ifdef UDATA_DEBUG
    fprintf(stderr, "SUFFIX=%s [%p]\n", inSuffix, inSuffix);
#endif

    
    if(inSuffix != NULL) {
        suffix = inSuffix;
    } else {
        suffix = "";
    }

    checkLastFour = doCheckLastFour;

    

#ifdef UDATA_DEBUG
    fprintf(stderr, "%p: init %s -> [path=%s], [base=%s], [suff=%s], [itempath=%s], [nextpath=%s], [checklast4=%s]\n",
            iter,
            item,
            path,
            basename,
            suffix,
            itemPath.data(),
            nextPath,
            checkLastFour?"TRUE":"false");
#endif
}








const char *UDataPathIterator::next(UErrorCode *pErrorCode)
{
    if(U_FAILURE(*pErrorCode)) {
        return NULL;
    }

    const char *currentPath = NULL;
    int32_t     pathLen = 0;
    const char *pathBasename;

    do
    {
        if( nextPath == NULL ) {
            break;
        }
        currentPath = nextPath;

        if(nextPath == itemPath.data()) { 
            nextPath = path; 
            pathLen = (int32_t)uprv_strlen(currentPath);
        } else {
            
            nextPath = uprv_strchr(currentPath, U_PATH_SEP_CHAR);
            if(nextPath == NULL) {
                
                pathLen = (int32_t)uprv_strlen(currentPath); 
            } else {
                
                pathLen = (int32_t)(nextPath - currentPath);
                
                nextPath ++;
            }
        }

        if(pathLen == 0) {
            continue;
        }

#ifdef UDATA_DEBUG
        fprintf(stderr, "rest of path (IDD) = %s\n", currentPath);
        fprintf(stderr, "                     ");
        { 
            uint32_t qqq;
            for(qqq=0;qqq<pathLen;qqq++)
            {
                fprintf(stderr, " ");
            }

            fprintf(stderr, "^\n");
        }
#endif
        pathBuffer.clear().append(currentPath, pathLen, *pErrorCode);

        
        pathBasename = findBasename(pathBuffer.data());

        if(checkLastFour == TRUE && 
           (pathLen>=4) &&
           uprv_strncmp(pathBuffer.data() +(pathLen-4), suffix, 4)==0 && 
           uprv_strncmp(findBasename(pathBuffer.data()), basename, basenameLen)==0  && 
           uprv_strlen(pathBasename)==(basenameLen+4)) { 

#ifdef UDATA_DEBUG
            fprintf(stderr, "Have %s file on the path: %s\n", suffix, pathBuffer.data());
#endif
            
        }
        else 
        {       
            if(pathBuffer[pathLen-1] != U_FILE_SEP_CHAR) {
                if((pathLen>=4) &&
                   uprv_strncmp(pathBuffer.data()+(pathLen-4), ".dat", 4) == 0)
                {
#ifdef UDATA_DEBUG
                    fprintf(stderr, "skipping non-directory .dat file %s\n", pathBuffer.data());
#endif
                    continue;
                }

                
                if(!packageStub.isEmpty() &&
                   (pathLen > packageStub.length()) &&
                   !uprv_strcmp(pathBuffer.data() + pathLen - packageStub.length(), packageStub.data())) {
#ifdef UDATA_DEBUG
                  fprintf(stderr, "Found stub %s (will add package %s of len %d)\n", packageStub.data(), basename, basenameLen);
#endif
                  pathBuffer.truncate(pathLen - packageStub.length());
                }
                pathBuffer.append(U_FILE_SEP_CHAR, *pErrorCode);
            }

            
            pathBuffer.append(packageStub.data()+1, packageStub.length()-1, *pErrorCode);

            if(*suffix)  
            {
                pathBuffer.append(suffix, *pErrorCode);
            }
        }

#ifdef UDATA_DEBUG
        fprintf(stderr, " -->  %s\n", pathBuffer.data());
#endif

        return pathBuffer.data();

    } while(path);

    
    return NULL;
}

U_NAMESPACE_END











extern "C" const DataHeader U_DATA_API U_ICUDATA_ENTRY_POINT;





















static UDataMemory *
openCommonData(const char *path,          
               int32_t commonDataIndex,   
               UErrorCode *pErrorCode)
{
    UDataMemory tData;
    const char *pathBuffer;
    const char *inBasename;

    if (U_FAILURE(*pErrorCode)) {
        return NULL;
    }

    UDataMemory_init(&tData);

     
    if (commonDataIndex >= 0) {
        
        if(commonDataIndex >= LENGTHOF(gCommonICUDataArray)) {
            return NULL;
        }
        if(gCommonICUDataArray[commonDataIndex] == NULL) {
            int32_t i;
            for(i = 0; i < commonDataIndex; ++i) {
                if(gCommonICUDataArray[i]->pHeader == &U_ICUDATA_ENTRY_POINT) {
                    
                    return NULL;
                }
            }

            
            



            







            setCommonICUDataPointer(&U_ICUDATA_ENTRY_POINT, FALSE, pErrorCode);
        }
        return gCommonICUDataArray[commonDataIndex];
    }


    

    
    
    inBasename = findBasename(path);
#ifdef UDATA_DEBUG
    fprintf(stderr, "inBasename = %s\n", inBasename);
#endif

    if(*inBasename==0) {
        
        
#ifdef UDATA_DEBUG
        fprintf(stderr, "ocd: no basename in %s, bailing.\n", path);
#endif
        *pErrorCode=U_FILE_ACCESS_ERROR;
        return NULL;
    }

   
   
   
   {
        UDataMemory  *dataToReturn = udata_findCachedData(inBasename);
        if (dataToReturn != NULL) {
            return dataToReturn;
        }
    }

    



    UDataPathIterator iter(u_getDataDirectory(), inBasename, path, ".dat", TRUE, pErrorCode);

    while((UDataMemory_isLoaded(&tData)==FALSE) && (pathBuffer = iter.next(pErrorCode)) != NULL)
    {
#ifdef UDATA_DEBUG
        fprintf(stderr, "ocd: trying path %s - ", pathBuffer);
#endif
        uprv_mapFile(&tData, pathBuffer);
#ifdef UDATA_DEBUG
        fprintf(stderr, "%s\n", UDataMemory_isLoaded(&tData)?"LOADED":"not loaded");
#endif
    }

#if defined(OS390_STUBDATA) && defined(OS390BATCH)
    if (!UDataMemory_isLoaded(&tData)) {
        char ourPathBuffer[1024];
        
        uprv_strncpy(ourPathBuffer, path, 1019);
        ourPathBuffer[1019]=0;
        uprv_strcat(ourPathBuffer, ".dat");
        uprv_mapFile(&tData, ourPathBuffer);
    }
#endif

    if (!UDataMemory_isLoaded(&tData)) {
        
        *pErrorCode=U_FILE_ACCESS_ERROR;
        return NULL;
    }

    
    udata_checkCommonData(&tData, pErrorCode);


    


    return udata_cacheDataItem(inBasename, &tData, pErrorCode);
}












static UBool extendICUData(UErrorCode *pErr)
{
    UDataMemory   *pData;
    UDataMemory   copyPData;
    UBool         didUpdate = FALSE;

    









#if MAP_IMPLEMENTATION==MAP_STDIO
    static UMutex extendICUDataMutex = U_MUTEX_INITIALIZER;
    umtx_lock(&extendICUDataMutex);
#endif
    if(!gHaveTriedToLoadCommonData) {
        
        pData = openCommonData(
                   U_ICUDATA_NAME,            
                   -1,                        
                   pErr);

        

       UDataMemory_init(&copyPData);
       if(pData != NULL) {
          UDatamemory_assign(&copyPData, pData);
          copyPData.map = 0;              
          copyPData.mapAddr = 0;          
                                          
                                          
                                          
                                          

          didUpdate = 
              setCommonICUData(&copyPData,
                       FALSE,             
                       pErr);             
        }

        gHaveTriedToLoadCommonData = TRUE;
    }

    didUpdate = findCommonICUDataByName(U_ICUDATA_NAME);  
                                                          
                                                          
                                                          

#if MAP_IMPLEMENTATION==MAP_STDIO
    umtx_unlock(&extendICUDataMutex);
#endif
    return didUpdate;               
                                    
                                    
                                    
}






U_CAPI void U_EXPORT2
udata_setCommonData(const void *data, UErrorCode *pErrorCode) {
    UDataMemory dataMemory;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    if(data==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    
    UDataMemory_init(&dataMemory);
    UDataMemory_setData(&dataMemory, data);
    udata_checkCommonData(&dataMemory, pErrorCode);
    if (U_FAILURE(*pErrorCode)) {return;}

    
    
    setCommonICUData(&dataMemory, TRUE, pErrorCode);
}






U_CAPI void U_EXPORT2
udata_setAppData(const char *path, const void *data, UErrorCode *err)
{
    UDataMemory     udm;

    if(err==NULL || U_FAILURE(*err)) {
        return;
    }
    if(data==NULL) {
        *err=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    UDataMemory_init(&udm);
    UDataMemory_setData(&udm, data);
    udata_checkCommonData(&udm, err);
    udata_cacheDataItem(path, &udm, err);
}












static UDataMemory *
checkDataItem
(
 const DataHeader         *pHeader,         
 UDataMemoryIsAcceptable  *isAcceptable,    
 void                     *context,         
 const char               *type,            
 const char               *name,            
 UErrorCode               *nonFatalErr,     
                                            
                                            
 UErrorCode               *fatalErr         
 )
{
    UDataMemory  *rDataMem = NULL;          

    if (U_FAILURE(*fatalErr)) {
        return NULL;
    }

    if(pHeader->dataHeader.magic1==0xda &&
        pHeader->dataHeader.magic2==0x27 &&
        (isAcceptable==NULL || isAcceptable(context, type, name, &pHeader->info))
    ) {
        rDataMem=UDataMemory_createNewInstance(fatalErr);
        if (U_FAILURE(*fatalErr)) {
            return NULL;
        }
        rDataMem->pHeader = pHeader;
    } else {
        
        
        
        *nonFatalErr=U_INVALID_FORMAT_ERROR;
    }
    return rDataMem;
}




static UDataMemory *doLoadFromIndividualFiles(const char *pkgName, 
        const char *dataPath, const char *tocEntryPathSuffix,
            
            const char *path, const char *type, const char *name,
             UDataMemoryIsAcceptable *isAcceptable, void *context,
             UErrorCode *subErrorCode,
             UErrorCode *pErrorCode)
{
    const char         *pathBuffer;
    UDataMemory         dataMemory;
    UDataMemory *pEntryData;

    
    
    UDataPathIterator iter(dataPath, pkgName, path, tocEntryPathSuffix, FALSE, pErrorCode);

    while((pathBuffer = iter.next(pErrorCode)))
    {
#ifdef UDATA_DEBUG
        fprintf(stderr, "UDATA: trying individual file %s\n", pathBuffer);
#endif
        if(uprv_mapFile(&dataMemory, pathBuffer))
        {
            pEntryData = checkDataItem(dataMemory.pHeader, isAcceptable, context, type, name, subErrorCode, pErrorCode);
            if (pEntryData != NULL) {
                


                pEntryData->mapAddr = dataMemory.mapAddr;
                pEntryData->map     = dataMemory.map;

#ifdef UDATA_DEBUG
                fprintf(stderr, "** Mapped file: %s\n", pathBuffer);
#endif
                return pEntryData;
            }

            
            udata_close(&dataMemory);

            
            if (U_FAILURE(*pErrorCode)) {
                return NULL;
            }

            
            *subErrorCode=U_INVALID_FORMAT_ERROR;
        }
#ifdef UDATA_DEBUG
        fprintf(stderr, "%s\n", UDataMemory_isLoaded(&dataMemory)?"LOADED":"not loaded");
#endif
    }
    return NULL;
}




static UDataMemory *doLoadFromCommonData(UBool isICUData, const char * , 
        const char * , const char * , const char *tocEntryName,
            
            const char *path, const char *type, const char *name,
             UDataMemoryIsAcceptable *isAcceptable, void *context,
             UErrorCode *subErrorCode,
             UErrorCode *pErrorCode)
{
    UDataMemory        *pEntryData;
    const DataHeader   *pHeader;
    UDataMemory        *pCommonData;
    int32_t            commonDataIndex;
    UBool              checkedExtendedICUData = FALSE;
    








    for (commonDataIndex = isICUData ? 0 : -1;;) {
        pCommonData=openCommonData(path, commonDataIndex, subErrorCode); 

        if(U_SUCCESS(*subErrorCode) && pCommonData!=NULL) {
            int32_t length;

            
            pHeader=pCommonData->vFuncs->Lookup(pCommonData, tocEntryName, &length, subErrorCode);
#ifdef UDATA_DEBUG
            fprintf(stderr, "%s: pHeader=%p - %s\n", tocEntryName, pHeader, u_errorName(*subErrorCode));
#endif

            if(pHeader!=NULL) {
                pEntryData = checkDataItem(pHeader, isAcceptable, context, type, name, subErrorCode, pErrorCode);
#ifdef UDATA_DEBUG
                fprintf(stderr, "pEntryData=%p\n", pEntryData);
#endif
                if (U_FAILURE(*pErrorCode)) {
                    return NULL;
                }
                if (pEntryData != NULL) {
                    pEntryData->length = length;
                    return pEntryData;
                }
            }
        }
        


        if (!isICUData) {
            return NULL;
        } else if (pCommonData != NULL) {
            ++commonDataIndex;  
        } else if ((!checkedExtendedICUData) && extendICUData(subErrorCode)) {
            checkedExtendedICUData = TRUE;
            
        } else {
            return NULL;
        }
    }
}

































static UDataMemory *
doOpenChoice(const char *path, const char *type, const char *name,
             UDataMemoryIsAcceptable *isAcceptable, void *context,
             UErrorCode *pErrorCode)
{
    UDataMemory         *retVal = NULL;

    const char         *dataPath;

    int32_t             tocEntrySuffixIndex;
    const char         *tocEntryPathSuffix;
    UErrorCode          subErrorCode=U_ZERO_ERROR;
    const char         *treeChar;

    UBool               isICUData = FALSE;


    
    if(path == NULL ||
       !strcmp(path, U_ICUDATA_ALIAS) ||  
       !uprv_strncmp(path, U_ICUDATA_NAME U_TREE_SEPARATOR_STRING, 
                     uprv_strlen(U_ICUDATA_NAME U_TREE_SEPARATOR_STRING)) ||  
       !uprv_strncmp(path, U_ICUDATA_ALIAS U_TREE_SEPARATOR_STRING, 
                     uprv_strlen(U_ICUDATA_ALIAS U_TREE_SEPARATOR_STRING))) {
      isICUData = TRUE;
    }

#if (U_FILE_SEP_CHAR != U_FILE_ALT_SEP_CHAR)  
    
    CharString altSepPath;
    if(path) {
        if(uprv_strchr(path,U_FILE_ALT_SEP_CHAR) != NULL) {
            altSepPath.append(path, *pErrorCode);
            char *p;
            while((p=uprv_strchr(altSepPath.data(), U_FILE_ALT_SEP_CHAR))) {
                *p = U_FILE_SEP_CHAR;
            }
#if defined (UDATA_DEBUG)
            fprintf(stderr, "Changed path from [%s] to [%s]\n", path, altSepPath.s);
#endif
            path = altSepPath.data();
        }
    }
#endif

    CharString tocEntryName; 
    CharString tocEntryPath; 

    CharString pkgName;
    CharString treeName;

    
    if(path==NULL) {
        pkgName.append(U_ICUDATA_NAME, *pErrorCode);
    } else {
        const char *pkg;
        const char *first;
        pkg = uprv_strrchr(path, U_FILE_SEP_CHAR);
        first = uprv_strchr(path, U_FILE_SEP_CHAR);
        if(uprv_pathIsAbsolute(path) || (pkg != first)) { 
            
            if(pkg) {
                pkgName.append(pkg+1, *pErrorCode);
            } else {
                pkgName.append(path, *pErrorCode);
            }
        } else {
            treeChar = uprv_strchr(path, U_TREE_SEPARATOR);
            if(treeChar) { 
                treeName.append(treeChar+1, *pErrorCode); 
                if(isICUData) {
                    pkgName.append(U_ICUDATA_NAME, *pErrorCode);
                } else {
                    pkgName.append(path, (int32_t)(treeChar-path), *pErrorCode);
                    if (first == NULL) {
                        



                        path = pkgName.data();
                    }
                }
            } else {
                if(isICUData) {
                    pkgName.append(U_ICUDATA_NAME, *pErrorCode);
                } else {
                    pkgName.append(path, *pErrorCode);
                }
            }
        }
    }

#ifdef UDATA_DEBUG
    fprintf(stderr, " P=%s T=%s\n", pkgName.data(), treeName.data());
#endif

    




    
    tocEntryName.append(pkgName, *pErrorCode);
    tocEntryPath.append(pkgName, *pErrorCode);
    tocEntrySuffixIndex = tocEntryName.length();

    if(!treeName.isEmpty()) {
        tocEntryName.append(U_TREE_ENTRY_SEP_CHAR, *pErrorCode).append(treeName, *pErrorCode);
        tocEntryPath.append(U_FILE_SEP_CHAR, *pErrorCode).append(treeName, *pErrorCode);
    }

    tocEntryName.append(U_TREE_ENTRY_SEP_CHAR, *pErrorCode).append(name, *pErrorCode);
    tocEntryPath.append(U_FILE_SEP_CHAR, *pErrorCode).append(name, *pErrorCode);
    if(type!=NULL && *type!=0) {
        tocEntryName.append(".", *pErrorCode).append(type, *pErrorCode);
        tocEntryPath.append(".", *pErrorCode).append(type, *pErrorCode);
    }
    tocEntryPathSuffix = tocEntryPath.data()+tocEntrySuffixIndex; 

#ifdef UDATA_DEBUG
    fprintf(stderr, " tocEntryName = %s\n", tocEntryName.data());
    fprintf(stderr, " tocEntryPath = %s\n", tocEntryName.data());
#endif

    if(path == NULL) {
        path = COMMON_DATA_NAME; 
    }

    
#ifdef UDATA_DEBUG
    fprintf(stderr, "IND: inBasename = %s, pkg=%s\n", "(n/a)", packageNameFromPath(path));
#endif

    
    dataPath = u_getDataDirectory();

    
    if(gDataFileAccess == UDATA_PACKAGES_FIRST) {
#ifdef UDATA_DEBUG
        fprintf(stderr, "Trying packages (UDATA_PACKAGES_FIRST)\n");
#endif
        
        retVal = doLoadFromCommonData(isICUData, 
                            pkgName.data(), dataPath, tocEntryPathSuffix, tocEntryName.data(),
                            path, type, name, isAcceptable, context, &subErrorCode, pErrorCode);
        if((retVal != NULL) || U_FAILURE(*pErrorCode)) {
            return retVal;
        }
    }

    
    if((gDataFileAccess==UDATA_PACKAGES_FIRST) ||
       (gDataFileAccess==UDATA_FILES_FIRST)) {
#ifdef UDATA_DEBUG
        fprintf(stderr, "Trying individual files\n");
#endif
        
        if ((dataPath && *dataPath) || !isICUData) {
            retVal = doLoadFromIndividualFiles(pkgName.data(), dataPath, tocEntryPathSuffix,
                            path, type, name, isAcceptable, context, &subErrorCode, pErrorCode);
            if((retVal != NULL) || U_FAILURE(*pErrorCode)) {
                return retVal;
            }
        }
    }

    
    if((gDataFileAccess==UDATA_ONLY_PACKAGES) || 
       (gDataFileAccess==UDATA_FILES_FIRST)) {
#ifdef UDATA_DEBUG
        fprintf(stderr, "Trying packages (UDATA_ONLY_PACKAGES || UDATA_FILES_FIRST)\n");
#endif
        retVal = doLoadFromCommonData(isICUData,
                            pkgName.data(), dataPath, tocEntryPathSuffix, tocEntryName.data(),
                            path, type, name, isAcceptable, context, &subErrorCode, pErrorCode);
        if((retVal != NULL) || U_FAILURE(*pErrorCode)) {
            return retVal;
        }
    }
    
    

  
    if(gDataFileAccess==UDATA_NO_FILES) {
#ifdef UDATA_DEBUG
        fprintf(stderr, "Trying common data (UDATA_NO_FILES)\n");
#endif
        retVal = doLoadFromCommonData(isICUData,
                            pkgName.data(), "", tocEntryPathSuffix, tocEntryName.data(),
                            path, type, name, isAcceptable, context, &subErrorCode, pErrorCode);
        if((retVal != NULL) || U_FAILURE(*pErrorCode)) {
            return retVal;
        }
    }

    
    if(U_SUCCESS(*pErrorCode)) {
        if(U_SUCCESS(subErrorCode)) {
            
            *pErrorCode=U_FILE_ACCESS_ERROR;
        } else {
            
            *pErrorCode=subErrorCode;
        }
    }
    return retVal;
}





U_CAPI UDataMemory * U_EXPORT2
udata_open(const char *path, const char *type, const char *name,
           UErrorCode *pErrorCode) {
#ifdef UDATA_DEBUG
  fprintf(stderr, "udata_open(): Opening: %s : %s . %s\n", (path?path:"NULL"), name, type);
    fflush(stderr);
#endif

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return NULL;
    } else if(name==NULL || *name==0) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    } else {
        return doOpenChoice(path, type, name, NULL, NULL, pErrorCode);
    }
}



U_CAPI UDataMemory * U_EXPORT2
udata_openChoice(const char *path, const char *type, const char *name,
                 UDataMemoryIsAcceptable *isAcceptable, void *context,
                 UErrorCode *pErrorCode) {
#ifdef UDATA_DEBUG
  fprintf(stderr, "udata_openChoice(): Opening: %s : %s . %s\n", (path?path:"NULL"), name, type);
#endif

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return NULL;
    } else if(name==NULL || *name==0 || isAcceptable==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    } else {
        return doOpenChoice(path, type, name, isAcceptable, context, pErrorCode);
    }
}



U_CAPI void U_EXPORT2
udata_getInfo(UDataMemory *pData, UDataInfo *pInfo) {
    if(pInfo!=NULL) {
        if(pData!=NULL && pData->pHeader!=NULL) {
            const UDataInfo *info=&pData->pHeader->info;
            uint16_t dataInfoSize=udata_getInfoSize(info);
            if(pInfo->size>dataInfoSize) {
                pInfo->size=dataInfoSize;
            }
            uprv_memcpy((uint16_t *)pInfo+1, (const uint16_t *)info+1, pInfo->size-2);
            if(info->isBigEndian!=U_IS_BIG_ENDIAN) {
                
                uint16_t x=info->reservedWord;
                pInfo->reservedWord=(uint16_t)((x<<8)|(x>>8));
            }
        } else {
            pInfo->size=0;
        }
    }
}


U_CAPI void U_EXPORT2 udata_setFileAccess(UDataFileAccess access, UErrorCode * )
{
    gDataFileAccess = access;
}
