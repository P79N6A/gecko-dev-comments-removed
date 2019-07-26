















#ifndef __UDATA_H__
#define __UDATA_H__

#include "unicode/utypes.h"
#include "unicode/localpointer.h"

U_CDECL_BEGIN















 
#ifndef U_HIDE_INTERNAL_API




#define U_TREE_SEPARATOR '-'





#define U_TREE_SEPARATOR_STRING "-"





#define U_TREE_ENTRY_SEP_CHAR '/'





#define U_TREE_ENTRY_SEP_STRING "/"





#define U_ICUDATA_ALIAS "ICUDATA"

#endif 


































typedef struct {
    

    uint16_t size;

    

    uint16_t reservedWord;

    
    

    uint8_t isBigEndian;

    

    uint8_t charsetFamily;

    

    uint8_t sizeofUChar;

    

    uint8_t reservedByte;

    

    uint8_t dataFormat[4];

    

    uint8_t formatVersion[4];

    

    uint8_t dataVersion[4];
} UDataInfo;







typedef struct UDataMemory UDataMemory;














typedef UBool U_CALLCONV
UDataMemoryIsAcceptable(void *context,
                        const char *type, const char *name,
                        const UDataInfo *pInfo);























U_STABLE UDataMemory * U_EXPORT2
udata_open(const char *path, const char *type, const char *name,
           UErrorCode *pErrorCode);

















































U_STABLE UDataMemory * U_EXPORT2
udata_openChoice(const char *path, const char *type, const char *name,
                 UDataMemoryIsAcceptable *isAcceptable, void *context,
                 UErrorCode *pErrorCode);








U_STABLE void U_EXPORT2
udata_close(UDataMemory *pData);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUDataMemoryPointer, UDataMemory, udata_close);

U_NAMESPACE_END

#endif







U_STABLE const void * U_EXPORT2
udata_getMemory(UDataMemory *pData);



















U_STABLE void U_EXPORT2
udata_getInfo(UDataMemory *pData, UDataInfo *pInfo);









































U_STABLE void U_EXPORT2
udata_setCommonData(const void *data, UErrorCode *err);


























U_STABLE void U_EXPORT2
udata_setAppData(const char *packageName, const void *data, UErrorCode *err);






typedef enum UDataFileAccess {
    
    UDATA_FILES_FIRST,
    
    UDATA_DEFAULT_ACCESS = UDATA_FILES_FIRST,
    
    UDATA_ONLY_PACKAGES,
    

    UDATA_PACKAGES_FIRST,
    
    UDATA_NO_FILES,
    
    UDATA_FILE_ACCESS_COUNT
} UDataFileAccess;











U_STABLE void U_EXPORT2
udata_setFileAccess(UDataFileAccess access, UErrorCode *status);

U_CDECL_END

#endif
