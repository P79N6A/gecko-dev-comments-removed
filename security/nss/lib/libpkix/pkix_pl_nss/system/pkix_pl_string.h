









#ifndef _PKIX_PL_STRING_H
#define _PKIX_PL_STRING_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_PL_StringStruct {
        void* utf16String;
        PKIX_UInt32 utf16Length;
        
        char* escAsciiString;
        PKIX_UInt32 escAsciiLength;
};



PKIX_Error *
pkix_pl_String_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
