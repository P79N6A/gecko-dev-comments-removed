









#ifndef _PKIX_PL_CRLENTRY_H
#define _PKIX_PL_CRLENTRY_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PKIX_PL_CRL_REASONCODE_NOTSET (-1)

struct PKIX_PL_CRLEntryStruct {
        CERTCrlEntry *nssCrlEntry;
        PKIX_PL_BigInt *serialNumber;
        PKIX_List *critExtOids;
        PKIX_Int32 userReasonCode;
        PKIX_Boolean userReasonCodeAbsent;
};



PKIX_Error *pkix_pl_CRLEntry_RegisterSelf(void *plContext);



PKIX_Error *
pkix_pl_CRLEntry_Create(
        CERTCrlEntry **nssCrlEntry, 
        PKIX_List **pCrlEntryList,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
