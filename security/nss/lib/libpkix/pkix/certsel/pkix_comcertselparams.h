










































#ifndef _PKIX_COMCERTSELPARAMS_H
#define _PKIX_COMCERTSELPARAMS_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif







struct PKIX_ComCertSelParamsStruct {
        PKIX_Int32 version;
        PKIX_Int32 minPathLength;
        PKIX_Boolean matchAllSubjAltNames;
        PKIX_PL_X500Name *subject;
        PKIX_List *policies; 
        PKIX_PL_Cert *cert;
        PKIX_PL_CertNameConstraints *nameConstraints;
        PKIX_List *pathToNames; 
        PKIX_List *subjAltNames; 
        PKIX_List *extKeyUsage; 
        PKIX_UInt32 keyUsage;
        PKIX_PL_Date *date;
        PKIX_PL_Date *certValid;
        PKIX_PL_X500Name *issuer;
        PKIX_PL_BigInt *serialNumber;
        PKIX_PL_ByteArray *authKeyId;
        PKIX_PL_ByteArray *subjKeyId;
        PKIX_PL_PublicKey *subjPubKey;
        PKIX_PL_OID *subjPKAlgId;
        PKIX_Boolean leafCertFlag;
};



PKIX_Error *pkix_ComCertSelParams_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
