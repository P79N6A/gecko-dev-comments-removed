










































#ifndef _PKIX_H
#define _PKIX_H

#include "pkixt.h"
#include "pkix_util.h"
#include "pkix_params.h"
#include "pkix_results.h"
#include "pkix_certstore.h"
#include "pkix_certsel.h"
#include "pkix_crlsel.h"
#include "pkix_checker.h"
#include "pkix_revchecker.h"
#include "pkix_pl_system.h"
#include "pkix_pl_pki.h"

#ifdef __cplusplus
extern "C" {
#endif













































































PKIX_Error *
PKIX_Initialize(
        PKIX_Boolean platformInitNeeded,
        PKIX_UInt32 desiredMajorVersion,
        PKIX_UInt32 minDesiredMinorVersion,
        PKIX_UInt32 maxDesiredMinorVersion,
        PKIX_UInt32 *pActualMinorVersion,
        void **pPlContext);



















PKIX_Error *
PKIX_Shutdown(void *plContext);

































PKIX_Error *
PKIX_ValidateChain(
        PKIX_ValidateParams *params,
        PKIX_ValidateResult **pResult,
	PKIX_VerifyNode **pVerifyTree,
        void *plContext);




















































PKIX_Error *
PKIX_ValidateChain_NB(
	PKIX_ValidateParams *params,
	PKIX_UInt32 *pCertIndex,
	PKIX_UInt32 *pAnchorIndex,
	PKIX_UInt32 *pCheckerIndex,
	PKIX_Boolean *pRevChecking,
	PKIX_List **pCheckers,
	void **pNBIOContext,
	PKIX_ValidateResult **pResult,
	PKIX_VerifyNode **pVerifyTree,
	void *plContext);


















































PKIX_Error *
PKIX_BuildChain(
        PKIX_ProcessingParams *params,
        void **pNBIOContext,
        void **pState,
        PKIX_BuildResult **pResult,
	PKIX_VerifyNode **pVerifyNode,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
