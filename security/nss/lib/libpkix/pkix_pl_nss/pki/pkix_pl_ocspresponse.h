










































#ifndef _PKIX_PL_OCSPRESPONSE_H
#define _PKIX_PL_OCSPRESPONSE_H

#include "pkix_pl_common.h"
#include "pkix_pl_ocspcertid.h"
#include "hasht.h"
#include "cryptohi.h"
#include "ocspti.h"
#include "ocspi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_OCSP_RESPONSE_LEN (64*1024)

struct PKIX_PL_OcspResponseStruct{
        PRArenaPool *arena;
        const PKIX_PL_OcspRequest *request;
        const SEC_HttpClientFcn *httpClient;
        SEC_HTTP_SERVER_SESSION serverSession;
        SEC_HTTP_REQUEST_SESSION requestSession;
        PKIX_PL_OcspResponse_VerifyCallback verifyFcn;
        SECItem *encodedResponse;
        CERTCertDBHandle *handle;
        int64 producedAt;
        PKIX_PL_Date *producedAtDate;
        PKIX_PL_Cert *pkixSignerCert;
        CERTOCSPResponse *nssOCSPResponse;
        CERTCertificate *signerCert;
};



PKIX_Error *pkix_pl_OcspResponse_RegisterSelf(void *plContext);

PKIX_Error *
PKIX_PL_OcspResponse_UseBuildChain(
        PKIX_PL_Cert *signerCert,
	PKIX_PL_Date *producedAt,
        PKIX_ProcessingParams *procParams,
        void **pNBIOContext,
        void **pState,
        PKIX_BuildResult **pBuildResult,
        PKIX_VerifyNode **pVerifyTree,
	void *plContext);

#ifdef __cplusplus
}
#endif

#endif
