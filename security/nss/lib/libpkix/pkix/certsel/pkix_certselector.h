









#ifndef _PKIX_CERTSELECTOR_H
#define _PKIX_CERTSELECTOR_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_CertSelectorStruct {
        PKIX_CertSelector_MatchCallback matchCallback;
        PKIX_ComCertSelParams *params;
        PKIX_PL_Object *context;
};



PKIX_Error *
pkix_CertSelector_Select(
	PKIX_CertSelector *selector,
	PKIX_List *before,
	PKIX_List **pAfter,
	void *plContext);

PKIX_Error *pkix_CertSelector_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
