









#ifndef _PKIX_PL_X500NAME_H
#define _PKIX_PL_X500NAME_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif


struct PKIX_PL_X500NameStruct{
        PRArenaPool *arena; 

        CERTName nssDN;
        SECItem derName;    


};



PKIX_Error *pkix_pl_X500Name_RegisterSelf(void *plContext);

PKIX_Error *pkix_pl_X500Name_GetDERName(
        PKIX_PL_X500Name *xname,
        PRArenaPool *arena,
        SECItem **pSECName,
        void *plContext);

#ifdef BUILD_LIBPKIX_TESTS
PKIX_Error * pkix_pl_X500Name_CreateFromUtf8(
        char *stringRep,
        PKIX_PL_X500Name **pName,
        void *plContext);
#endif 

PKIX_Error *pkix_pl_X500Name_GetCommonName(
        PKIX_PL_X500Name *xname,
        unsigned char **pCommonName,
        void *plContext);

PKIX_Error *
pkix_pl_X500Name_GetCountryName(
        PKIX_PL_X500Name *xname,
        unsigned char **pCountryName,
        void *plContext);

PKIX_Error *
pkix_pl_X500Name_GetOrgName(
        PKIX_PL_X500Name *xname,
        unsigned char **pOrgName,
        void *plContext);

PKIX_Error *
pkix_pl_X500Name_GetCERTName(
        PKIX_PL_X500Name *xname,
        CERTName **pCERTName,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
