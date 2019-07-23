










































#ifndef _PKIX_CRLSELECTOR_H
#define _PKIX_CRLSELECTOR_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_CRLSelectorStruct {
        PKIX_CRLSelector_MatchCallback matchCallback;
        PKIX_ComCRLSelParams *params;
        PKIX_PL_Object *context;
};



PKIX_Error *pkix_CRLSelector_RegisterSelf(void *plContext);

PKIX_Error *
pkix_CRLSelector_Select(
	PKIX_CRLSelector *selector,
	PKIX_List *before,
	PKIX_List **pAfter,
	void *plContext);
#ifdef __cplusplus
}
#endif

#endif
