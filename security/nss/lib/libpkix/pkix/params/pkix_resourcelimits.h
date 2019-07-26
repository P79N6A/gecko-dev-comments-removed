









#ifndef _PKIX_RESOURCELIMITS_H
#define _PKIX_RESOURCELIMITS_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_ResourceLimitsStruct {
        PKIX_UInt32 maxTime;
        PKIX_UInt32 maxFanout;
        PKIX_UInt32 maxDepth;
        PKIX_UInt32 maxCertsNumber;
        PKIX_UInt32 maxCrlsNumber;
};



PKIX_Error *pkix_ResourceLimits_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
