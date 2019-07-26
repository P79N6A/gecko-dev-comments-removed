









#ifndef _PKIX_PL_INFOACCESS_H
#define _PKIX_PL_INFOACCESS_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_PL_InfoAccessStruct{
        PKIX_UInt32 method;
        PKIX_PL_GeneralName *location;
};



PKIX_Error *pkix_pl_InfoAccess_RegisterSelf(void *plContext);

PKIX_Error *
pkix_pl_InfoAccess_CreateList(
        CERTAuthInfoAccess **authInfoAccess,
        PKIX_List **pAiaList, 
        void *plContext);

PKIX_Error *
pkix_pl_InfoAccess_ParseLocation(
        PKIX_PL_GeneralName *generalName,
        PRArenaPool *arena,
        LDAPRequestParams *request,
        char **pDomainName,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
