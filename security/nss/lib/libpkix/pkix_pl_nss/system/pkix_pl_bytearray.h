









#ifndef _PKIX_PL_BYTEARRAY_H
#define _PKIX_PL_BYTEARRAY_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_PL_ByteArrayStruct {
        void *array;
        PKIX_UInt32 length;
};



PKIX_Error *
pkix_pl_ByteArray_ToHexString(
        PKIX_PL_ByteArray *array,
        PKIX_PL_String **pString,
        void *plContext);

PKIX_Error *
pkix_pl_ByteArray_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
