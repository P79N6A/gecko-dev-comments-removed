




#ifndef _FLAGCB
#define _FLAGCB

#include "unicode/utypes.h"
#include "unicode/ucnv.h"




typedef struct
{
  UConverterFromUCallback  subCallback;
  const void               *subContext;
  UBool                    flag;
} FromUFLAGContext;





U_CAPI FromUFLAGContext* U_EXPORT2  flagCB_fromU_openContext();




U_CAPI void U_EXPORT2 flagCB_fromU(
                  const void *context,
                  UConverterFromUnicodeArgs *fromUArgs,
                  const UChar* codeUnits,
                  int32_t length,
                  UChar32 codePoint,
                  UConverterCallbackReason reason,
				  UErrorCode * err);



typedef struct
{
    UConverterFromUCallback  subCallback;
    const void               *subContext;
    uint32_t       magic;      
    uint32_t       serial;     
} debugCBContext;

U_CAPI void debugCB_fromU(const void *context,
                   UConverterFromUnicodeArgs *fromUArgs,
                   const UChar* codeUnits,
                   int32_t length,
                   UChar32 codePoint,
                   UConverterCallbackReason reason,
                   UErrorCode * err);

U_CAPI debugCBContext *debugCB_openContext();

#endif
