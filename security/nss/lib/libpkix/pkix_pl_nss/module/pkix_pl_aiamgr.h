









#ifndef _PKIX_PL_AIAMGR_H
#define _PKIX_PL_AIAMGR_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_PL_AIAMgrStruct {
        
        
        PKIX_UInt32 method;
        PKIX_UInt32 aiaIndex;
        PKIX_UInt32 numAias;
        PKIX_List *aia;
        PKIX_PL_GeneralName *location;
        PKIX_List *results;
	union {
	        PKIX_PL_LdapClient *ldapClient;
		struct {
		        const SEC_HttpClientFcn *httpClient;
			SEC_HTTP_SERVER_SESSION serverSession;
			SEC_HTTP_REQUEST_SESSION requestSession;
			char *path;
		} hdata;
	} client;
};



PKIX_Error *pkix_pl_AIAMgr_RegisterSelf(void *plContext);

PKIX_Error *PKIX_PL_LdapClient_InitiateRequest(
        PKIX_PL_LdapClient *client,
        LDAPRequestParams *requestParams,
        void **pPollDesc,
        PKIX_List **pResponse,
        void *plContext);

PKIX_Error *PKIX_PL_LdapClient_ResumeRequest(
        PKIX_PL_LdapClient *client,
        void **pPollDesc,
        PKIX_List **pResponse,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
