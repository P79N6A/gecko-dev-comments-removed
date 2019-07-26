




















#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "unicode/putil.h"
#include "unicode/udata.h"
#include "unicode/ucnv.h"
#include "unicode/uloc.h"
#include "putilimp.h"
#include "utracimp.h"
#include "ucnv_io.h"
#include "ucnv_bld.h"
#include "ucnvmbcs.h"
#include "ucnv_ext.h"
#include "ucnv_cnv.h"
#include "ucnv_imp.h"
#include "uhash.h"
#include "umutex.h"
#include "cstring.h"
#include "cmemory.h"
#include "ucln_cmn.h"
#include "ustr_cnv.h"



#if 0
#include <stdio.h>
extern void UCNV_DEBUG_LOG(char *what, char *who, void *p, int l);
#define UCNV_DEBUG_LOG(x,y,z) UCNV_DEBUG_LOG(x,y,z,__LINE__)
#else
# define UCNV_DEBUG_LOG(x,y,z)
#endif

static const UConverterSharedData * const
converterData[UCNV_NUMBER_OF_SUPPORTED_CONVERTER_TYPES]={
    NULL, NULL,

#if UCONFIG_NO_LEGACY_CONVERSION
    NULL,
#else
    &_MBCSData,
#endif

    &_Latin1Data,
    &_UTF8Data, &_UTF16BEData, &_UTF16LEData, &_UTF32BEData, &_UTF32LEData,
    NULL,

#if UCONFIG_NO_LEGACY_CONVERSION
    NULL,
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL,
#else
    &_ISO2022Data,
    &_LMBCSData1,&_LMBCSData2, &_LMBCSData3, &_LMBCSData4, &_LMBCSData5, &_LMBCSData6,
    &_LMBCSData8,&_LMBCSData11,&_LMBCSData16,&_LMBCSData17,&_LMBCSData18,&_LMBCSData19,
    &_HZData,
#endif

    &_SCSUData,

#if UCONFIG_NO_LEGACY_CONVERSION
    NULL,
#else
    &_ISCIIData,
#endif

    &_ASCIIData,
    &_UTF7Data, &_Bocu1Data, &_UTF16Data, &_UTF32Data, &_CESU8Data, &_IMAPData,

#if UCONFIG_NO_LEGACY_CONVERSION
    NULL,
#else
    &_CompoundTextData
#endif
};





static struct {
  const char *name;
  const UConverterType type;
} const cnvNameType[] = {
  { "bocu1", UCNV_BOCU1 },
  { "cesu8", UCNV_CESU8 },
#if !UCONFIG_NO_LEGACY_CONVERSION
  { "hz",UCNV_HZ },
#endif
  { "imapmailboxname", UCNV_IMAP_MAILBOX },
#if !UCONFIG_NO_LEGACY_CONVERSION
  { "iscii", UCNV_ISCII },
  { "iso2022", UCNV_ISO_2022 },
#endif
  { "iso88591", UCNV_LATIN_1 },
#if !UCONFIG_NO_LEGACY_CONVERSION
  { "lmbcs1", UCNV_LMBCS_1 },
  { "lmbcs11",UCNV_LMBCS_11 },
  { "lmbcs16",UCNV_LMBCS_16 },
  { "lmbcs17",UCNV_LMBCS_17 },
  { "lmbcs18",UCNV_LMBCS_18 },
  { "lmbcs19",UCNV_LMBCS_19 },
  { "lmbcs2", UCNV_LMBCS_2 },
  { "lmbcs3", UCNV_LMBCS_3 },
  { "lmbcs4", UCNV_LMBCS_4 },
  { "lmbcs5", UCNV_LMBCS_5 },
  { "lmbcs6", UCNV_LMBCS_6 },
  { "lmbcs8", UCNV_LMBCS_8 },
#endif
  { "scsu", UCNV_SCSU },
  { "usascii", UCNV_US_ASCII },
  { "utf16", UCNV_UTF16 },
  { "utf16be", UCNV_UTF16_BigEndian },
  { "utf16le", UCNV_UTF16_LittleEndian },
#if U_IS_BIG_ENDIAN
  { "utf16oppositeendian", UCNV_UTF16_LittleEndian },
  { "utf16platformendian", UCNV_UTF16_BigEndian },
#else
  { "utf16oppositeendian", UCNV_UTF16_BigEndian},
  { "utf16platformendian", UCNV_UTF16_LittleEndian },
#endif
  { "utf32", UCNV_UTF32 },
  { "utf32be", UCNV_UTF32_BigEndian },
  { "utf32le", UCNV_UTF32_LittleEndian },
#if U_IS_BIG_ENDIAN
  { "utf32oppositeendian", UCNV_UTF32_LittleEndian },
  { "utf32platformendian", UCNV_UTF32_BigEndian },
#else
  { "utf32oppositeendian", UCNV_UTF32_BigEndian },
  { "utf32platformendian", UCNV_UTF32_LittleEndian },
#endif
  { "utf7", UCNV_UTF7 },
  { "utf8", UCNV_UTF8 },
  { "x11compoundtext", UCNV_COMPOUND_TEXT}
};



static UHashtable *SHARED_DATA_HASHTABLE = NULL;
static UMutex cnvCacheMutex = U_MUTEX_INITIALIZER;  
                                                    
                                                    

static const char **gAvailableConverters = NULL;
static uint16_t gAvailableConverterCount = 0;

#if !U_CHARSET_IS_UTF8


static char gDefaultConverterNameBuffer[UCNV_MAX_CONVERTER_NAME_LENGTH + 1]; 
static const char *gDefaultConverterName = NULL;







static const UConverterSharedData *gDefaultAlgorithmicSharedData = NULL;


static UBool gDefaultConverterContainsOption;

#endif  

static const char DATA_TYPE[] = "cnv";

static void
ucnv_flushAvailableConverterCache() {
    if (gAvailableConverters) {
        umtx_lock(&cnvCacheMutex);
        gAvailableConverterCount = 0;
        uprv_free((char **)gAvailableConverters);
        gAvailableConverters = NULL;
        umtx_unlock(&cnvCacheMutex);
    }
}





static UBool U_CALLCONV ucnv_cleanup(void) {
    ucnv_flushCache();
    if (SHARED_DATA_HASHTABLE != NULL && uhash_count(SHARED_DATA_HASHTABLE) == 0) {
        uhash_close(SHARED_DATA_HASHTABLE);
        SHARED_DATA_HASHTABLE = NULL;
    }

    
    ucnv_flushAvailableConverterCache();

#if !U_CHARSET_IS_UTF8
    gDefaultConverterName = NULL;
    gDefaultConverterNameBuffer[0] = 0;
    gDefaultConverterContainsOption = FALSE;
    gDefaultAlgorithmicSharedData = NULL;
#endif

    return (SHARED_DATA_HASHTABLE == NULL);
}

static UBool U_CALLCONV
isCnvAcceptable(void * ,
                const char * , const char * ,
                const UDataInfo *pInfo) {
    return (UBool)(
        pInfo->size>=20 &&
        pInfo->isBigEndian==U_IS_BIG_ENDIAN &&
        pInfo->charsetFamily==U_CHARSET_FAMILY &&
        pInfo->sizeofUChar==U_SIZEOF_UCHAR &&
        pInfo->dataFormat[0]==0x63 &&   
        pInfo->dataFormat[1]==0x6e &&
        pInfo->dataFormat[2]==0x76 &&
        pInfo->dataFormat[3]==0x74 &&
        pInfo->formatVersion[0]==6);  
}




static UConverterSharedData*
ucnv_data_unFlattenClone(UConverterLoadArgs *pArgs, UDataMemory *pData, UErrorCode *status)
{
    
    const uint8_t *raw = (const uint8_t *)udata_getMemory(pData);
    const UConverterStaticData *source = (const UConverterStaticData *) raw;
    UConverterSharedData *data;
    UConverterType type = (UConverterType)source->conversionType;

    if(U_FAILURE(*status))
        return NULL;

    if( (uint16_t)type >= UCNV_NUMBER_OF_SUPPORTED_CONVERTER_TYPES ||
        converterData[type] == NULL ||
        converterData[type]->referenceCounter != 1 ||
        source->structSize != sizeof(UConverterStaticData))
    {
        *status = U_INVALID_TABLE_FORMAT;
        return NULL;
    }

    data = (UConverterSharedData *)uprv_malloc(sizeof(UConverterSharedData));
    if(data == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    
    uprv_memcpy(data, converterData[type], sizeof(UConverterSharedData));

#if 0 
    









    data->table = (UConverterTable *)uprv_malloc(sizeof(UConverterTable));
    if(data->table == NULL) {
        uprv_free(data);
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    uprv_memset(data->table, 0, sizeof(UConverterTable));
#endif

    data->staticData = source;

    data->sharedDataCached = FALSE;

    
    data->dataMemory = (void*)pData; 

    if(data->impl->load != NULL) {
        data->impl->load(data, pArgs, raw + source->structSize, status);
        if(U_FAILURE(*status)) {
            uprv_free(data->table);
            uprv_free(data);
            return NULL;
        }
    }
    return data;
}





static UConverterSharedData *createConverterFromFile(UConverterLoadArgs *pArgs, UErrorCode * err)
{
    UDataMemory *data;
    UConverterSharedData *sharedData;

    UTRACE_ENTRY_OC(UTRACE_UCNV_LOAD);

    if (U_FAILURE (*err)) {
        UTRACE_EXIT_STATUS(*err);
        return NULL;
    }

    UTRACE_DATA2(UTRACE_OPEN_CLOSE, "load converter %s from package %s", pArgs->name, pArgs->pkg);

    data = udata_openChoice(pArgs->pkg, DATA_TYPE, pArgs->name, isCnvAcceptable, NULL, err);
    if(U_FAILURE(*err))
    {
        UTRACE_EXIT_STATUS(*err);
        return NULL;
    }

    sharedData = ucnv_data_unFlattenClone(pArgs, data, err);
    if(U_FAILURE(*err))
    {
        udata_close(data);
        UTRACE_EXIT_STATUS(*err);
        return NULL;
    }

    






    UTRACE_EXIT_PTR_STATUS(sharedData, *err);
    return sharedData;
}



static const UConverterSharedData *
getAlgorithmicTypeFromName(const char *realName)
{
    uint32_t mid, start, limit;
    uint32_t lastMid;
    int result;
    char strippedName[UCNV_MAX_CONVERTER_NAME_LENGTH];

    
    ucnv_io_stripForCompare(strippedName, realName);

    
    start = 0;
    limit = sizeof(cnvNameType)/sizeof(cnvNameType[0]);
    mid = limit;
    lastMid = UINT32_MAX;

    for (;;) {
        mid = (uint32_t)((start + limit) / 2);
        if (lastMid == mid) {   
            break;  
        }
        lastMid = mid;
        result = uprv_strcmp(strippedName, cnvNameType[mid].name);

        if (result < 0) {
            limit = mid;
        } else if (result > 0) {
            start = mid;
        } else {
            return converterData[cnvNameType[mid].type];
        }
    }

    return NULL;
}









#define UCNV_CACHE_LOAD_FACTOR 2







static void
ucnv_shareConverterData(UConverterSharedData * data)
{
    UErrorCode err = U_ZERO_ERROR;
    
    

    if (SHARED_DATA_HASHTABLE == NULL)
    {
        SHARED_DATA_HASHTABLE = uhash_openSize(uhash_hashChars, uhash_compareChars, NULL,
                            ucnv_io_countKnownConverters(&err)*UCNV_CACHE_LOAD_FACTOR,
                            &err);
        ucln_common_registerCleanup(UCLN_COMMON_UCNV, ucnv_cleanup);

        if (U_FAILURE(err))
            return;
    }

    

    








    
    data->sharedDataCached = TRUE;

    uhash_put(SHARED_DATA_HASHTABLE,
            (void*) data->staticData->name, 

            data,
            &err);
    UCNV_DEBUG_LOG("put", data->staticData->name,data);

}







static UConverterSharedData *
ucnv_getSharedConverterData(const char *name)
{
    
    if (SHARED_DATA_HASHTABLE == NULL)
    {
        return NULL;
    }
    else
    {
        UConverterSharedData *rc;

        rc = (UConverterSharedData*)uhash_get(SHARED_DATA_HASHTABLE, name);
        UCNV_DEBUG_LOG("get",name,rc);
        return rc;
    }
}












static UBool
ucnv_deleteSharedConverterData(UConverterSharedData * deadSharedData)
{
    UTRACE_ENTRY_OC(UTRACE_UCNV_UNLOAD);
    UTRACE_DATA2(UTRACE_OPEN_CLOSE, "unload converter %s shared data %p", deadSharedData->staticData->name, deadSharedData);

    if (deadSharedData->referenceCounter > 0) {
        UTRACE_EXIT_VALUE((int32_t)FALSE);
        return FALSE;
    }

    if (deadSharedData->impl->unload != NULL) {
        deadSharedData->impl->unload(deadSharedData);
    }

    if(deadSharedData->dataMemory != NULL)
    {
        UDataMemory *data = (UDataMemory*)deadSharedData->dataMemory;
        udata_close(data);
    }

    if(deadSharedData->table != NULL)
    {
        uprv_free(deadSharedData->table);
    }

#if 0
    
    
    if(deadSharedData->staticDataOwned == TRUE) 
    {
        uprv_free((void*)deadSharedData->staticData);
    }
#endif

#if 0
    
    uprv_memset(deadSharedData->0, sizeof(*deadSharedData));
#endif

    uprv_free(deadSharedData);

    UTRACE_EXIT_VALUE((int32_t)TRUE);
    return TRUE;
}





UConverterSharedData *
ucnv_load(UConverterLoadArgs *pArgs, UErrorCode *err) {
    UConverterSharedData *mySharedConverterData;

    if(err == NULL || U_FAILURE(*err)) {
        return NULL;
    }

    if(pArgs->pkg != NULL && *pArgs->pkg != 0) {
        
        return createConverterFromFile(pArgs, err);
    }

    mySharedConverterData = ucnv_getSharedConverterData(pArgs->name);
    if (mySharedConverterData == NULL)
    {
        
        mySharedConverterData = createConverterFromFile(pArgs, err);
        if (U_FAILURE (*err) || (mySharedConverterData == NULL))
        {
            return NULL;
        }
        else if (!pArgs->onlyTestIsLoadable)
        {
            
            ucnv_shareConverterData(mySharedConverterData);
        }
    }
    else
    {
        
        
        mySharedConverterData->referenceCounter++;
    }

    return mySharedConverterData;
}






U_CAPI void
ucnv_unload(UConverterSharedData *sharedData) {
    if(sharedData != NULL) {
        if (sharedData->referenceCounter > 0) {
            sharedData->referenceCounter--;
        }

        if((sharedData->referenceCounter <= 0)&&(sharedData->sharedDataCached == FALSE)) {
            ucnv_deleteSharedConverterData(sharedData);
        }
    }
}

U_CFUNC void
ucnv_unloadSharedDataIfReady(UConverterSharedData *sharedData)
{
    




    if(sharedData != NULL && sharedData->referenceCounter != (uint32_t)~0) {
        umtx_lock(&cnvCacheMutex);
        ucnv_unload(sharedData);
        umtx_unlock(&cnvCacheMutex);
    }
}

U_CFUNC void
ucnv_incrementRefCount(UConverterSharedData *sharedData)
{
    




    if(sharedData != NULL && sharedData->referenceCounter != (uint32_t)~0) {
        umtx_lock(&cnvCacheMutex);
        sharedData->referenceCounter++;
        umtx_unlock(&cnvCacheMutex);
    }
}








static void
parseConverterOptions(const char *inName,
                      UConverterNamePieces *pPieces,
                      UConverterLoadArgs *pArgs,
                      UErrorCode *err)
{
    char *cnvName = pPieces->cnvName;
    char c;
    int32_t len = 0;

    pArgs->name=inName;
    pArgs->locale=pPieces->locale;
    pArgs->options=pPieces->options;

    
    while((c=*inName)!=0 && c!=UCNV_OPTION_SEP_CHAR) {
        if (++len>=UCNV_MAX_CONVERTER_NAME_LENGTH) {
            *err = U_ILLEGAL_ARGUMENT_ERROR;    
            pPieces->cnvName[0]=0;
            return;
        }
        *cnvName++=c;
        inName++;
    }
    *cnvName=0;
    pArgs->name=pPieces->cnvName;

    
    while((c=*inName)!=0) {
        if(c==UCNV_OPTION_SEP_CHAR) {
            ++inName;
        }

        
        if(uprv_strncmp(inName, "locale=", 7)==0) {
            
            char *dest=pPieces->locale;

            
            inName+=7;
            len=0;
            while((c=*inName)!=0 && c!=UCNV_OPTION_SEP_CHAR) {
                ++inName;

                if(++len>=ULOC_FULLNAME_CAPACITY) {
                    *err=U_ILLEGAL_ARGUMENT_ERROR;    
                    pPieces->locale[0]=0;
                    return;
                }

                *dest++=c;
            }
            *dest=0;
        } else if(uprv_strncmp(inName, "version=", 8)==0) {
            
            inName+=8;
            c=*inName;
            if(c==0) {
                pArgs->options=(pPieces->options&=~UCNV_OPTION_VERSION);
                return;
            } else if((uint8_t)(c-'0')<10) {
                pArgs->options=pPieces->options=(pPieces->options&~UCNV_OPTION_VERSION)|(uint32_t)(c-'0');
                ++inName;
            }
        } else if(uprv_strncmp(inName, "swaplfnl", 8)==0) {
            inName+=8;
            pArgs->options=(pPieces->options|=UCNV_OPTION_SWAP_LFNL);
        
        } else {
            
            while(((c = *inName++) != 0) && (c != UCNV_OPTION_SEP_CHAR)) {
            }
            if(c==0) {
                return;
            }
        }
    }
}








U_CFUNC UConverterSharedData *
ucnv_loadSharedData(const char *converterName,
                    UConverterNamePieces *pPieces,
                    UConverterLoadArgs *pArgs,
                    UErrorCode * err) {
    UConverterNamePieces stackPieces;
    UConverterLoadArgs stackArgs;
    UConverterSharedData *mySharedConverterData = NULL;
    UErrorCode internalErrorCode = U_ZERO_ERROR;
    UBool mayContainOption = TRUE;
    UBool checkForAlgorithmic = TRUE;

    if (U_FAILURE (*err)) {
        return NULL;
    }

    if(pPieces == NULL) {
        if(pArgs != NULL) {
            



            *err = U_INTERNAL_PROGRAM_ERROR;
            return NULL;
        }
        pPieces = &stackPieces;
    }
    if(pArgs == NULL) {
        uprv_memset(&stackArgs, 0, sizeof(stackArgs));
        stackArgs.size = (int32_t)sizeof(stackArgs);
        pArgs = &stackArgs;
    }

    pPieces->cnvName[0] = 0;
    pPieces->locale[0] = 0;
    pPieces->options = 0;

    pArgs->name = converterName;
    pArgs->locale = pPieces->locale;
    pArgs->options = pPieces->options;

    
    if (converterName == NULL) {
#if U_CHARSET_IS_UTF8
        pArgs->name = "UTF-8";
        return (UConverterSharedData *)converterData[UCNV_UTF8];
#else
        
        pArgs->name = ucnv_getDefaultName();
        if (pArgs->name == NULL) {
            *err = U_MISSING_RESOURCE_ERROR;
            return NULL;
        }
        mySharedConverterData = (UConverterSharedData *)gDefaultAlgorithmicSharedData;
        checkForAlgorithmic = FALSE;
        mayContainOption = gDefaultConverterContainsOption;
        
#endif
    }
    else if(UCNV_FAST_IS_UTF8(converterName)) {
        
        pArgs->name = "UTF-8";
        return (UConverterSharedData *)converterData[UCNV_UTF8];
    }
    else {
        
        parseConverterOptions(converterName, pPieces, pArgs, err);
        if (U_FAILURE(*err)) {
            
            return NULL;
        }

        
        pArgs->name = ucnv_io_getConverterName(pArgs->name, &mayContainOption, &internalErrorCode);
        if (U_FAILURE(internalErrorCode) || pArgs->name == NULL) {
            



            pArgs->name = pPieces->cnvName;
        }
    }

    
    if(mayContainOption && pArgs->name != pPieces->cnvName) {
        parseConverterOptions(pArgs->name, pPieces, pArgs, err);
    }

    
    if (checkForAlgorithmic) {
        mySharedConverterData = (UConverterSharedData *)getAlgorithmicTypeFromName(pArgs->name);
    }
    if (mySharedConverterData == NULL)
    {
        
        
        
        
        
        pArgs->nestedLoads=1;
        pArgs->pkg=NULL;

        umtx_lock(&cnvCacheMutex);
        mySharedConverterData = ucnv_load(pArgs, err);
        umtx_unlock(&cnvCacheMutex);
        if (U_FAILURE (*err) || (mySharedConverterData == NULL))
        {
            return NULL;
        }
    }

    return mySharedConverterData;
}

U_CAPI UConverter *
ucnv_createConverter(UConverter *myUConverter, const char *converterName, UErrorCode * err)
{
    UConverterNamePieces stackPieces;
    UConverterLoadArgs stackArgs=UCNV_LOAD_ARGS_INITIALIZER;
    UConverterSharedData *mySharedConverterData;

    UTRACE_ENTRY_OC(UTRACE_UCNV_OPEN);

    if(U_SUCCESS(*err)) {
        UTRACE_DATA1(UTRACE_OPEN_CLOSE, "open converter %s", converterName);

        mySharedConverterData = ucnv_loadSharedData(converterName, &stackPieces, &stackArgs, err);

        myUConverter = ucnv_createConverterFromSharedData(
            myUConverter, mySharedConverterData,
            &stackArgs,
            err);

        if(U_SUCCESS(*err)) {
            UTRACE_EXIT_PTR_STATUS(myUConverter, *err);
            return myUConverter;
        }
    }

    
    UTRACE_EXIT_STATUS(*err);
    return NULL;
}

U_CFUNC UBool
ucnv_canCreateConverter(const char *converterName, UErrorCode *err) {
    UConverter myUConverter;
    UConverterNamePieces stackPieces;
    UConverterLoadArgs stackArgs=UCNV_LOAD_ARGS_INITIALIZER;
    UConverterSharedData *mySharedConverterData;

    UTRACE_ENTRY_OC(UTRACE_UCNV_OPEN);

    if(U_SUCCESS(*err)) {
        UTRACE_DATA1(UTRACE_OPEN_CLOSE, "test if can open converter %s", converterName);

        stackArgs.onlyTestIsLoadable=TRUE;
        mySharedConverterData = ucnv_loadSharedData(converterName, &stackPieces, &stackArgs, err);
        ucnv_createConverterFromSharedData(
            &myUConverter, mySharedConverterData,
            &stackArgs,
            err);
        ucnv_unloadSharedDataIfReady(mySharedConverterData);
    }

    UTRACE_EXIT_STATUS(*err);
    return U_SUCCESS(*err);
}

UConverter *
ucnv_createAlgorithmicConverter(UConverter *myUConverter,
                                UConverterType type,
                                const char *locale, uint32_t options,
                                UErrorCode *err) {
    UConverter *cnv;
    const UConverterSharedData *sharedData;
    UConverterLoadArgs stackArgs=UCNV_LOAD_ARGS_INITIALIZER;

    UTRACE_ENTRY_OC(UTRACE_UCNV_OPEN_ALGORITHMIC);
    UTRACE_DATA1(UTRACE_OPEN_CLOSE, "open algorithmic converter type %d", (int32_t)type);

    if(type<0 || UCNV_NUMBER_OF_SUPPORTED_CONVERTER_TYPES<=type) {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
        UTRACE_EXIT_STATUS(U_ILLEGAL_ARGUMENT_ERROR);
        return NULL;
    }

    sharedData = converterData[type];
    




    if(sharedData == NULL || sharedData->referenceCounter != (uint32_t)~0) {
        
        *err = U_ILLEGAL_ARGUMENT_ERROR;
        UTRACE_EXIT_STATUS(U_ILLEGAL_ARGUMENT_ERROR);
        return NULL;
    }

    stackArgs.name = "";
    stackArgs.options = options;
    stackArgs.locale=locale;
    cnv = ucnv_createConverterFromSharedData(
            myUConverter, (UConverterSharedData *)sharedData,
            &stackArgs, err);

    UTRACE_EXIT_PTR_STATUS(cnv, *err);
    return cnv;
}

U_CFUNC UConverter*
ucnv_createConverterFromPackage(const char *packageName, const char *converterName, UErrorCode * err)
{
    UConverter *myUConverter;
    UConverterSharedData *mySharedConverterData;
    UConverterNamePieces stackPieces;
    UConverterLoadArgs stackArgs=UCNV_LOAD_ARGS_INITIALIZER;

    UTRACE_ENTRY_OC(UTRACE_UCNV_OPEN_PACKAGE);

    if(U_FAILURE(*err)) {
        UTRACE_EXIT_STATUS(*err);
        return NULL;
    }

    UTRACE_DATA2(UTRACE_OPEN_CLOSE, "open converter %s from package %s", converterName, packageName);

    
    stackPieces.cnvName[0] = 0;
    stackPieces.locale[0] = 0;
    stackPieces.options = 0;
    parseConverterOptions(converterName, &stackPieces, &stackArgs, err);
    if (U_FAILURE(*err)) {
        
        UTRACE_EXIT_STATUS(*err);
        return NULL;
    }
    stackArgs.nestedLoads=1;
    stackArgs.pkg=packageName;

    
    mySharedConverterData = createConverterFromFile(&stackArgs, err);

    if (U_FAILURE(*err)) {
        UTRACE_EXIT_STATUS(*err);
        return NULL;
    }

    
    myUConverter = ucnv_createConverterFromSharedData(NULL, mySharedConverterData, &stackArgs, err);

    if (U_FAILURE(*err)) {
        ucnv_close(myUConverter);
        UTRACE_EXIT_STATUS(*err);
        return NULL;
    }

    UTRACE_EXIT_PTR_STATUS(myUConverter, *err);
    return myUConverter;
}


U_CFUNC UConverter*
ucnv_createConverterFromSharedData(UConverter *myUConverter,
                                   UConverterSharedData *mySharedConverterData,
                                   UConverterLoadArgs *pArgs,
                                   UErrorCode *err)
{
    UBool isCopyLocal;

    if(U_FAILURE(*err)) {
        ucnv_unloadSharedDataIfReady(mySharedConverterData);
        return myUConverter;
    }
    if(myUConverter == NULL)
    {
        myUConverter = (UConverter *) uprv_malloc (sizeof (UConverter));
        if(myUConverter == NULL)
        {
            *err = U_MEMORY_ALLOCATION_ERROR;
            ucnv_unloadSharedDataIfReady(mySharedConverterData);
            return NULL;
        }
        isCopyLocal = FALSE;
    } else {
        isCopyLocal = TRUE;
    }

    
    uprv_memset(myUConverter, 0, sizeof(UConverter));
    myUConverter->isCopyLocal = isCopyLocal;
     
    myUConverter->sharedData = mySharedConverterData;
    myUConverter->options = pArgs->options;
    if(!pArgs->onlyTestIsLoadable) {
        myUConverter->preFromUFirstCP = U_SENTINEL;
        myUConverter->fromCharErrorBehaviour = UCNV_TO_U_DEFAULT_CALLBACK;
        myUConverter->fromUCharErrorBehaviour = UCNV_FROM_U_DEFAULT_CALLBACK;
        myUConverter->toUnicodeStatus = mySharedConverterData->toUnicodeStatus;
        myUConverter->maxBytesPerUChar = mySharedConverterData->staticData->maxBytesPerChar;
        myUConverter->subChar1 = mySharedConverterData->staticData->subChar1;
        myUConverter->subCharLen = mySharedConverterData->staticData->subCharLen;
        myUConverter->subChars = (uint8_t *)myUConverter->subUChars;
        uprv_memcpy(myUConverter->subChars, mySharedConverterData->staticData->subChar, myUConverter->subCharLen);
        myUConverter->toUCallbackReason = UCNV_ILLEGAL; 
    }

    if(mySharedConverterData->impl->open != NULL) {
        mySharedConverterData->impl->open(myUConverter, pArgs, err);
        if(U_FAILURE(*err) && !pArgs->onlyTestIsLoadable) {
            
            ucnv_close(myUConverter);
            return NULL;
        }
    }

    return myUConverter;
}



U_CAPI int32_t U_EXPORT2
ucnv_flushCache ()
{
    UConverterSharedData *mySharedData = NULL;
    int32_t pos;
    int32_t tableDeletedNum = 0;
    const UHashElement *e;
    
    int32_t i, remaining;

    UTRACE_ENTRY_OC(UTRACE_UCNV_FLUSH_CACHE);

    
    u_flushDefaultConverter();

    


    if (SHARED_DATA_HASHTABLE == NULL) {
        UTRACE_EXIT_VALUE((int32_t)0);
        return 0;
    }

    










    umtx_lock(&cnvCacheMutex);
    





    i = 0;
    do {
        remaining = 0;
        pos = -1;
        while ((e = uhash_nextElement (SHARED_DATA_HASHTABLE, &pos)) != NULL)
        {
            mySharedData = (UConverterSharedData *) e->value.pointer;
            
            if (mySharedData->referenceCounter == 0)
            {
                tableDeletedNum++;

                UCNV_DEBUG_LOG("del",mySharedData->staticData->name,mySharedData);

                uhash_removeElement(SHARED_DATA_HASHTABLE, e);
                mySharedData->sharedDataCached = FALSE;
                ucnv_deleteSharedConverterData (mySharedData);
            } else {
                ++remaining;
            }
        }
    } while(++i == 1 && remaining > 0);
    umtx_unlock(&cnvCacheMutex);

    UTRACE_DATA1(UTRACE_INFO, "ucnv_flushCache() exits with %d converters remaining", remaining);

    UTRACE_EXIT_VALUE(tableDeletedNum);
    return tableDeletedNum;
}



static UBool haveAvailableConverterList(UErrorCode *pErrorCode) {
    int needInit;
    UMTX_CHECK(&cnvCacheMutex, (gAvailableConverters == NULL), needInit);
    if (needInit) {
        UConverter tempConverter;
        UEnumeration *allConvEnum = NULL;
        uint16_t idx;
        uint16_t localConverterCount;
        uint16_t allConverterCount;
        UErrorCode localStatus;
        const char *converterName;
        const char **localConverterList;

        allConvEnum = ucnv_openAllNames(pErrorCode);
        allConverterCount = uenum_count(allConvEnum, pErrorCode);
        if (U_FAILURE(*pErrorCode)) {
            return FALSE;
        }

        
        localConverterList = (const char **) uprv_malloc(allConverterCount * sizeof(char*));
        if (!localConverterList) {
            *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
            return FALSE;
        }

        
        localStatus = U_ZERO_ERROR;
        ucnv_close(ucnv_createConverter(&tempConverter, NULL, &localStatus));

        localConverterCount = 0;

        for (idx = 0; idx < allConverterCount; idx++) {
            localStatus = U_ZERO_ERROR;
            converterName = uenum_next(allConvEnum, NULL, &localStatus);
            if (ucnv_canCreateConverter(converterName, &localStatus)) {
                localConverterList[localConverterCount++] = converterName;
            }
        }
        uenum_close(allConvEnum);

        umtx_lock(&cnvCacheMutex);
        if (gAvailableConverters == NULL) {
            gAvailableConverterCount = localConverterCount;
            gAvailableConverters = localConverterList;
            ucln_common_registerCleanup(UCLN_COMMON_UCNV, ucnv_cleanup);
        }
        else {
            uprv_free((char **)localConverterList);
        }
        umtx_unlock(&cnvCacheMutex);
    }
    return TRUE;
}

U_CFUNC uint16_t
ucnv_bld_countAvailableConverters(UErrorCode *pErrorCode) {
    if (haveAvailableConverterList(pErrorCode)) {
        return gAvailableConverterCount;
    }
    return 0;
}

U_CFUNC const char *
ucnv_bld_getAvailableConverter(uint16_t n, UErrorCode *pErrorCode) {
    if (haveAvailableConverterList(pErrorCode)) {
        if (n < gAvailableConverterCount) {
            return gAvailableConverters[n];
        }
        *pErrorCode = U_INDEX_OUTOFBOUNDS_ERROR;
    }
    return NULL;
}



#if !U_CHARSET_IS_UTF8












static inline void
internalSetName(const char *name, UErrorCode *status) {
    UConverterNamePieces stackPieces;
    UConverterLoadArgs stackArgs=UCNV_LOAD_ARGS_INITIALIZER;
    int32_t length=(int32_t)(uprv_strlen(name));
    UBool containsOption = (UBool)(uprv_strchr(name, UCNV_OPTION_SEP_CHAR) != NULL);
    const UConverterSharedData *algorithmicSharedData;

    stackArgs.name = name;
    if(containsOption) {
        stackPieces.cnvName[0] = 0;
        stackPieces.locale[0] = 0;
        stackPieces.options = 0;
        parseConverterOptions(name, &stackPieces, &stackArgs, status);
        if(U_FAILURE(*status)) {
            return;
        }
    }
    algorithmicSharedData = getAlgorithmicTypeFromName(stackArgs.name);

    umtx_lock(&cnvCacheMutex);

    gDefaultAlgorithmicSharedData = algorithmicSharedData;
    gDefaultConverterContainsOption = containsOption;
    uprv_memcpy(gDefaultConverterNameBuffer, name, length);
    gDefaultConverterNameBuffer[length]=0;

    
    
    gDefaultConverterName = gDefaultConverterNameBuffer;

    ucln_common_registerCleanup(UCLN_COMMON_UCNV, ucnv_cleanup);

    umtx_unlock(&cnvCacheMutex);
}
#endif









U_CAPI const char*  U_EXPORT2
ucnv_getDefaultName() {
#if U_CHARSET_IS_UTF8
    return "UTF-8";
#else
    
    const char *name;

    



    UMTX_CHECK(&cnvCacheMutex, gDefaultConverterName, name);
    if(name==NULL) {
        UErrorCode errorCode = U_ZERO_ERROR;
        UConverter *cnv = NULL;

        name = uprv_getDefaultCodepage();

        
        if(name != NULL) {
            cnv = ucnv_open(name, &errorCode);
            if(U_SUCCESS(errorCode) && cnv != NULL) {
                name = ucnv_getName(cnv, &errorCode);
            }
        }

        if(name == NULL || name[0] == 0
            || U_FAILURE(errorCode) || cnv == NULL
            || uprv_strlen(name)>=sizeof(gDefaultConverterNameBuffer))
        {
            
#if (U_CHARSET_FAMILY == U_ASCII_FAMILY)
            name = "US-ASCII";
            
#elif U_PLATFORM == U_PF_OS390
            name = "ibm-1047_P100-1995" UCNV_SWAP_LFNL_OPTION_STRING;
#else
            name = "ibm-37_P100-1995";
#endif
        }

        internalSetName(name, &errorCode);

        
        ucnv_close(cnv);
    }

    return name;
#endif
}

#if U_CHARSET_IS_UTF8
U_CAPI void U_EXPORT2 ucnv_setDefaultName(const char *) {}
#else




U_CAPI void U_EXPORT2
ucnv_setDefaultName(const char *converterName) {
    if(converterName==NULL) {
        
        gDefaultConverterName=NULL;
    } else {
        UErrorCode errorCode = U_ZERO_ERROR;
        UConverter *cnv = NULL;
        const char *name = NULL;

        
        cnv = ucnv_open(converterName, &errorCode);
        if(U_SUCCESS(errorCode) && cnv != NULL) {
            name = ucnv_getName(cnv, &errorCode);
        }

        if(U_SUCCESS(errorCode) && name!=NULL) {
            internalSetName(name, &errorCode);
        }
        

        
        ucnv_close(cnv);
  
        
        u_flushDefaultConverter();
    }
}
#endif





#if !UCONFIG_NO_LEGACY_CONVERSION

U_CAPI int32_t U_EXPORT2
ucnv_swap(const UDataSwapper *ds,
          const void *inData, int32_t length, void *outData,
          UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize;

    const uint8_t *inBytes;
    uint8_t *outBytes;

    uint32_t offset, count, staticDataSize;
    int32_t size;

    const UConverterStaticData *inStaticData;
    UConverterStaticData *outStaticData;

    const _MBCSHeader *inMBCSHeader;
    _MBCSHeader *outMBCSHeader;
    _MBCSHeader mbcsHeader;
    uint32_t mbcsHeaderLength;
    UBool noFromU=FALSE;

    uint8_t outputType;

    int32_t maxFastUChar, mbcsIndexLength;

    const int32_t *inExtIndexes;
    int32_t extOffset;

    
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x63 &&   
        pInfo->dataFormat[1]==0x6e &&
        pInfo->dataFormat[2]==0x76 &&
        pInfo->dataFormat[3]==0x74 &&
        pInfo->formatVersion[0]==6 &&
        pInfo->formatVersion[1]>=2
    )) {
        udata_printError(ds, "ucnv_swap(): data format %02x.%02x.%02x.%02x (format version %02x.%02x) is not recognized as an ICU .cnv conversion table\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0], pInfo->formatVersion[1]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData+headerSize;
    outBytes=(uint8_t *)outData+headerSize;

    
    inStaticData=(const UConverterStaticData *)inBytes;
    outStaticData=(UConverterStaticData *)outBytes;

    if(length<0) {
        staticDataSize=ds->readUInt32(inStaticData->structSize);
    } else {
        length-=headerSize;
        if( length<(int32_t)sizeof(UConverterStaticData) ||
            (uint32_t)length<(staticDataSize=ds->readUInt32(inStaticData->structSize))
        ) {
            udata_printError(ds, "ucnv_swap(): too few bytes (%d after header) for an ICU .cnv conversion table\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
    }

    if(length>=0) {
        
        if(inStaticData!=outStaticData) {
            uprv_memcpy(outStaticData, inStaticData, staticDataSize);
        }

        ds->swapArray32(ds, &inStaticData->structSize, 4,
                           &outStaticData->structSize, pErrorCode);
        ds->swapArray32(ds, &inStaticData->codepage, 4,
                           &outStaticData->codepage, pErrorCode);

        ds->swapInvChars(ds, inStaticData->name, (int32_t)uprv_strlen(inStaticData->name),
                            outStaticData->name, pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            udata_printError(ds, "ucnv_swap(): error swapping converter name\n");
            return 0;
        }
    }

    inBytes+=staticDataSize;
    outBytes+=staticDataSize;
    if(length>=0) {
        length-=(int32_t)staticDataSize;
    }

    
    if(inStaticData->conversionType==UCNV_MBCS) {
        
        inMBCSHeader=(const _MBCSHeader *)inBytes;
        outMBCSHeader=(_MBCSHeader *)outBytes;

        if(0<=length && length<(int32_t)sizeof(_MBCSHeader)) {
            udata_printError(ds, "ucnv_swap(): too few bytes (%d after headers) for an ICU MBCS .cnv conversion table\n",
                                length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
        if(inMBCSHeader->version[0]==4 && inMBCSHeader->version[1]>=1) {
            mbcsHeaderLength=MBCS_HEADER_V4_LENGTH;
        } else if(inMBCSHeader->version[0]==5 && inMBCSHeader->version[1]>=3 &&
                  ((mbcsHeader.options=ds->readUInt32(inMBCSHeader->options))&
                   MBCS_OPT_UNKNOWN_INCOMPATIBLE_MASK)==0
        ) {
            mbcsHeaderLength=mbcsHeader.options&MBCS_OPT_LENGTH_MASK;
            noFromU=(UBool)((mbcsHeader.options&MBCS_OPT_NO_FROM_U)!=0);
        } else {
            udata_printError(ds, "ucnv_swap(): unsupported _MBCSHeader.version %d.%d\n",
                             inMBCSHeader->version[0], inMBCSHeader->version[1]);
            *pErrorCode=U_UNSUPPORTED_ERROR;
            return 0;
        }

        uprv_memcpy(mbcsHeader.version, inMBCSHeader->version, 4);
        mbcsHeader.countStates=         ds->readUInt32(inMBCSHeader->countStates);
        mbcsHeader.countToUFallbacks=   ds->readUInt32(inMBCSHeader->countToUFallbacks);
        mbcsHeader.offsetToUCodeUnits=  ds->readUInt32(inMBCSHeader->offsetToUCodeUnits);
        mbcsHeader.offsetFromUTable=    ds->readUInt32(inMBCSHeader->offsetFromUTable);
        mbcsHeader.offsetFromUBytes=    ds->readUInt32(inMBCSHeader->offsetFromUBytes);
        mbcsHeader.flags=               ds->readUInt32(inMBCSHeader->flags);
        mbcsHeader.fromUBytesLength=    ds->readUInt32(inMBCSHeader->fromUBytesLength);
        

        extOffset=(int32_t)(mbcsHeader.flags>>8);
        outputType=(uint8_t)mbcsHeader.flags;
        if(noFromU && outputType==MBCS_OUTPUT_1) {
            udata_printError(ds, "ucnv_swap(): unsupported combination of makeconv --small with SBCS\n");
            *pErrorCode=U_UNSUPPORTED_ERROR;
            return 0;
        }

        
        switch(outputType) {
        case MBCS_OUTPUT_1:
        case MBCS_OUTPUT_2:
        case MBCS_OUTPUT_3:
        case MBCS_OUTPUT_4:
        case MBCS_OUTPUT_3_EUC:
        case MBCS_OUTPUT_4_EUC:
        case MBCS_OUTPUT_2_SISO:
        case MBCS_OUTPUT_EXT_ONLY:
            
            break;
        default:
            udata_printError(ds, "ucnv_swap(): unsupported MBCS output type 0x%x\n",
                             outputType);
            *pErrorCode=U_UNSUPPORTED_ERROR;
            return 0;
        }

        

        





        maxFastUChar=0;
        mbcsIndexLength=0;
        if( outputType!=MBCS_OUTPUT_EXT_ONLY && outputType!=MBCS_OUTPUT_1 &&
            mbcsHeader.version[1]>=3 && (maxFastUChar=mbcsHeader.version[2])!=0
        ) {
            maxFastUChar=(maxFastUChar<<8)|0xff;
            mbcsIndexLength=((maxFastUChar+1)>>6)*2;  
        }

        if(extOffset==0) {
            size=(int32_t)(mbcsHeader.offsetFromUBytes+mbcsIndexLength);
            if(!noFromU) {
                size+=(int32_t)mbcsHeader.fromUBytesLength;
            }

            
            inExtIndexes=NULL;
        } else {
            
            if(length>=0 && length<(extOffset+UCNV_EXT_INDEXES_MIN_LENGTH*4)) {
                udata_printError(ds, "ucnv_swap(): too few bytes (%d after headers) for an ICU MBCS .cnv conversion table with extension data\n",
                                 length);
                *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
                return 0;
            }

            inExtIndexes=(const int32_t *)(inBytes+extOffset);
            size=extOffset+udata_readInt32(ds, inExtIndexes[UCNV_EXT_SIZE]);
        }

        if(length>=0) {
            if(length<size) {
                udata_printError(ds, "ucnv_swap(): too few bytes (%d after headers) for an ICU MBCS .cnv conversion table\n",
                                 length);
                *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
                return 0;
            }

            
            if(inBytes!=outBytes) {
                uprv_memcpy(outBytes, inBytes, size);
            }

            
            count=mbcsHeaderLength*4;
            ds->swapArray32(ds, &inMBCSHeader->countStates, count-4,
                               &outMBCSHeader->countStates, pErrorCode);

            if(outputType==MBCS_OUTPUT_EXT_ONLY) {
                




                
                const char *inBaseName=(const char *)inBytes+count;
                char *outBaseName=(char *)outBytes+count;
                ds->swapInvChars(ds, inBaseName, (int32_t)uprv_strlen(inBaseName),
                                    outBaseName, pErrorCode);
            } else {
                

                
                offset=count;
                count=mbcsHeader.countStates*1024;
                ds->swapArray32(ds, inBytes+offset, (int32_t)count,
                                   outBytes+offset, pErrorCode);

                
                offset+=count;
                count=mbcsHeader.countToUFallbacks*8;
                ds->swapArray32(ds, inBytes+offset, (int32_t)count,
                                   outBytes+offset, pErrorCode);

                
                offset=mbcsHeader.offsetToUCodeUnits;
                count=mbcsHeader.offsetFromUTable-offset;
                ds->swapArray16(ds, inBytes+offset, (int32_t)count,
                                   outBytes+offset, pErrorCode);

                
                offset=mbcsHeader.offsetFromUTable;

                if(outputType==MBCS_OUTPUT_1) {
                    
                    count=(mbcsHeader.offsetFromUBytes-offset)+mbcsHeader.fromUBytesLength;
                    ds->swapArray16(ds, inBytes+offset, (int32_t)count,
                                       outBytes+offset, pErrorCode);
                } else {
                    

                    
                    if(inStaticData->unicodeMask&UCNV_HAS_SUPPLEMENTARY) {
                        count=0x440*2; 
                    } else {
                        count=0x40*2; 
                    }
                    ds->swapArray16(ds, inBytes+offset, (int32_t)count,
                                       outBytes+offset, pErrorCode);

                    
                    offset+=count;
                    count=mbcsHeader.offsetFromUBytes-offset;
                    ds->swapArray32(ds, inBytes+offset, (int32_t)count,
                                       outBytes+offset, pErrorCode);

                    
                    offset=mbcsHeader.offsetFromUBytes;
                    count= noFromU ? 0 : mbcsHeader.fromUBytesLength;
                    switch(outputType) {
                    case MBCS_OUTPUT_2:
                    case MBCS_OUTPUT_3_EUC:
                    case MBCS_OUTPUT_2_SISO:
                        ds->swapArray16(ds, inBytes+offset, (int32_t)count,
                                           outBytes+offset, pErrorCode);
                        break;
                    case MBCS_OUTPUT_4:
                        ds->swapArray32(ds, inBytes+offset, (int32_t)count,
                                           outBytes+offset, pErrorCode);
                        break;
                    default:
                        
                        break;
                    }

                    if(mbcsIndexLength!=0) {
                        offset+=count;
                        count=mbcsIndexLength;
                        ds->swapArray16(ds, inBytes+offset, (int32_t)count,
                                           outBytes+offset, pErrorCode);
                    }
                }
            }

            if(extOffset!=0) {
                
                inBytes+=extOffset;
                outBytes+=extOffset;

                
                offset=udata_readInt32(ds, inExtIndexes[UCNV_EXT_TO_U_INDEX]);
                length=udata_readInt32(ds, inExtIndexes[UCNV_EXT_TO_U_LENGTH]);
                ds->swapArray32(ds, inBytes+offset, length*4, outBytes+offset, pErrorCode);

                
                offset=udata_readInt32(ds, inExtIndexes[UCNV_EXT_TO_U_UCHARS_INDEX]);
                length=udata_readInt32(ds, inExtIndexes[UCNV_EXT_TO_U_UCHARS_LENGTH]);
                ds->swapArray16(ds, inBytes+offset, length*2, outBytes+offset, pErrorCode);

                
                offset=udata_readInt32(ds, inExtIndexes[UCNV_EXT_FROM_U_UCHARS_INDEX]);
                length=udata_readInt32(ds, inExtIndexes[UCNV_EXT_FROM_U_LENGTH]);
                ds->swapArray16(ds, inBytes+offset, length*2, outBytes+offset, pErrorCode);

                
                offset=udata_readInt32(ds, inExtIndexes[UCNV_EXT_FROM_U_VALUES_INDEX]);
                
                ds->swapArray32(ds, inBytes+offset, length*4, outBytes+offset, pErrorCode);

                

                
                offset=udata_readInt32(ds, inExtIndexes[UCNV_EXT_FROM_U_STAGE_12_INDEX]);
                length=udata_readInt32(ds, inExtIndexes[UCNV_EXT_FROM_U_STAGE_12_LENGTH]);
                ds->swapArray16(ds, inBytes+offset, length*2, outBytes+offset, pErrorCode);

                
                offset=udata_readInt32(ds, inExtIndexes[UCNV_EXT_FROM_U_STAGE_3_INDEX]);
                length=udata_readInt32(ds, inExtIndexes[UCNV_EXT_FROM_U_STAGE_3_LENGTH]);
                ds->swapArray16(ds, inBytes+offset, length*2, outBytes+offset, pErrorCode);

                
                offset=udata_readInt32(ds, inExtIndexes[UCNV_EXT_FROM_U_STAGE_3B_INDEX]);
                length=udata_readInt32(ds, inExtIndexes[UCNV_EXT_FROM_U_STAGE_3B_LENGTH]);
                ds->swapArray32(ds, inBytes+offset, length*4, outBytes+offset, pErrorCode);

                
                length=udata_readInt32(ds, inExtIndexes[UCNV_EXT_INDEXES_LENGTH]);
                ds->swapArray32(ds, inBytes, length*4, outBytes, pErrorCode);
            }
        }
    } else {
        udata_printError(ds, "ucnv_swap(): unknown conversionType=%d!=UCNV_MBCS\n",
                         inStaticData->conversionType);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    return headerSize+(int32_t)staticDataSize+size;
}

#endif 

#endif
