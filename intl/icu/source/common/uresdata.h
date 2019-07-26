
















#ifndef __RESDATA_H__
#define __RESDATA_H__

#include "unicode/utypes.h"
#include "unicode/udata.h"
#include "unicode/ures.h"
#include "putilimp.h"
#include "udataswp.h"







typedef enum {
    
    URES_INTERNAL_NONE=-1,

    
    URES_TABLE32=4,

    



    URES_TABLE16=5,

    
    URES_STRING_V2=6,

    



    URES_ARRAY16=9
} UResInternalType;






typedef uint32_t Resource;

#define RES_BOGUS 0xffffffff

#define RES_GET_TYPE(res) ((int32_t)((res)>>28UL))
#define RES_GET_OFFSET(res) ((res)&0x0fffffff)
#define RES_GET_POINTER(pRoot, res) ((pRoot)+RES_GET_OFFSET(res))


#if U_SIGNED_RIGHT_SHIFT_IS_ARITHMETIC
#   define RES_GET_INT(res) (((int32_t)((res)<<4L))>>4L)
#else
#   define RES_GET_INT(res) (int32_t)(((res)&0x08000000) ? (res)|0xf0000000 : (res)&0x07ffffff)
#endif

#define RES_GET_UINT(res) ((res)&0x0fffffff)

#define URES_IS_ARRAY(type) ((int32_t)(type)==URES_ARRAY || (int32_t)(type)==URES_ARRAY16)
#define URES_IS_TABLE(type) ((int32_t)(type)==URES_TABLE || (int32_t)(type)==URES_TABLE16 || (int32_t)(type)==URES_TABLE32)
#define URES_IS_CONTAINER(type) (URES_IS_TABLE(type) || URES_IS_ARRAY(type))

#define URES_MAKE_RESOURCE(type, offset) (((Resource)(type)<<28)|(Resource)(offset))
#define URES_MAKE_EMPTY_RESOURCE(type) ((Resource)(type)<<28)


enum {
    URES_INDEX_LENGTH,          





    URES_INDEX_KEYS_TOP,        
                                
    URES_INDEX_RESOURCES_TOP,   
    URES_INDEX_BUNDLE_TOP,      
                                
    URES_INDEX_MAX_TABLE_LENGTH,
    URES_INDEX_ATTRIBUTES,      
    URES_INDEX_16BIT_TOP,       

    URES_INDEX_POOL_CHECKSUM,   
    URES_INDEX_TOP
};












#define URES_ATT_NO_FALLBACK 1







#define URES_ATT_IS_POOL_BUNDLE 2
#define URES_ATT_USES_POOL_BUNDLE 4


























































































































































































typedef struct {
    UDataMemory *data;
    const int32_t *pRoot;
    const uint16_t *p16BitUnits;
    const char *poolBundleKeys;
    Resource rootRes;
    int32_t localKeyLimit;
    UBool noFallback; 
    UBool isPoolBundle;
    UBool usesPoolBundle;
    UBool useNativeStrcmp;
} ResourceData;




U_INTERNAL void U_EXPORT2
res_read(ResourceData *pResData,
         const UDataInfo *pInfo, const void *inBytes, int32_t length,
         UErrorCode *errorCode);





U_CFUNC void
res_load(ResourceData *pResData,
         const char *path, const char *name, UErrorCode *errorCode);





U_CFUNC void
res_unload(ResourceData *pResData);

U_INTERNAL UResType U_EXPORT2
res_getPublicType(Resource res);






U_INTERNAL const UChar * U_EXPORT2
res_getString(const ResourceData *pResData, Resource res, int32_t *pLength);

U_INTERNAL const UChar * U_EXPORT2
res_getAlias(const ResourceData *pResData, Resource res, int32_t *pLength);

U_INTERNAL const uint8_t * U_EXPORT2
res_getBinary(const ResourceData *pResData, Resource res, int32_t *pLength);

U_INTERNAL const int32_t * U_EXPORT2
res_getIntVector(const ResourceData *pResData, Resource res, int32_t *pLength);

U_INTERNAL Resource U_EXPORT2
res_getResource(const ResourceData *pResData, const char *key);

U_INTERNAL int32_t U_EXPORT2
res_countArrayItems(const ResourceData *pResData, Resource res);

U_INTERNAL Resource U_EXPORT2
res_getArrayItem(const ResourceData *pResData, Resource array, int32_t indexS);

U_INTERNAL Resource U_EXPORT2
res_getTableItemByIndex(const ResourceData *pResData, Resource table, int32_t indexS, const char ** key);

U_INTERNAL Resource U_EXPORT2
res_getTableItemByKey(const ResourceData *pResData, Resource table, int32_t *indexS, const char* * key);





U_CFUNC Resource res_findResource(const ResourceData *pResData, Resource r, char** path, const char** key);





U_CAPI int32_t U_EXPORT2
ures_swap(const UDataSwapper *ds,
          const void *inData, int32_t length, void *outData,
          UErrorCode *pErrorCode);

#endif
