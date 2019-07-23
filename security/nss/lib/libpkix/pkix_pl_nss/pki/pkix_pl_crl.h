










































#ifndef _PKIX_PL_CRL_H
#define _PKIX_PL_CRL_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_PL_CRLStruct {
        CERTSignedCrl *nssSignedCrl;
        PKIX_PL_X500Name *issuer;
        PKIX_PL_OID *signatureAlgId;
        PKIX_PL_BigInt *crlNumber;
        PKIX_Boolean crlNumberAbsent;
        PKIX_List *crlEntryList; 
        PKIX_List *critExtOids;
        SECItem *adoptedDerCrl;
        SECItem *derGenName; 

};



PKIX_Error *pkix_pl_CRL_RegisterSelf(void *plContext);

PKIX_Error *
pkix_pl_CRL_CreateWithSignedCRL(CERTSignedCrl *nssSignedCrl,
                                SECItem *derCrl,
                                SECItem *derGenName,
                                PKIX_PL_CRL **pCrl,
                                void *plContext);

#ifdef __cplusplus
}
#endif

#endif
