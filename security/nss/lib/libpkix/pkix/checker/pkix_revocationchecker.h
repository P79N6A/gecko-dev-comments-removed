










































#ifndef _PKIX_REVOCATIONCHECKER_H
#define _PKIX_REVOCATIONCHECKER_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_RevocationCheckerStruct {
        PKIX_RevocationChecker_RevCallback checkCallback;
        PKIX_PL_Object *revCheckerContext;
};



PKIX_Error *pkix_RevocationChecker_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
