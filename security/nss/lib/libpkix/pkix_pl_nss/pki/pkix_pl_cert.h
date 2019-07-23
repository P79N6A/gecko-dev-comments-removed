










































#ifndef _PKIX_PL_CERT_H
#define _PKIX_PL_CERT_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_PL_CertStruct {
        CERTCertificate *nssCert;
        CERTGeneralName *nssSubjAltNames;
        PLArenaPool *arenaNameConstraints;
        PKIX_PL_X500Name *issuer;
        PKIX_PL_X500Name *subject;
        PKIX_List *subjAltNames;
        PKIX_Boolean subjAltNamesAbsent;
        PKIX_PL_OID *publicKeyAlgId;
        PKIX_PL_PublicKey *publicKey;
        PKIX_PL_BigInt *serialNumber;
        PKIX_List *critExtOids;
        PKIX_PL_ByteArray *subjKeyId;
        PKIX_Boolean subjKeyIdAbsent;
        PKIX_PL_ByteArray *authKeyId;
        PKIX_Boolean authKeyIdAbsent;
        PKIX_List *extKeyUsages;
        PKIX_Boolean extKeyUsagesAbsent;
        PKIX_PL_CertBasicConstraints *certBasicConstraints;
        PKIX_Boolean basicConstraintsAbsent;
        PKIX_List *certPolicyInfos;
        PKIX_Boolean policyInfoAbsent;
        PKIX_Boolean policyMappingsAbsent;
        PKIX_List *certPolicyMappings; 
        PKIX_Boolean policyConstraintsProcessed;
        PKIX_Int32 policyConstraintsExplicitPolicySkipCerts;
        PKIX_Int32 policyConstraintsInhibitMappingSkipCerts;
        PKIX_Boolean inhibitAnyPolicyProcessed;
        PKIX_Int32 inhibitAnySkipCerts;
        PKIX_PL_CertNameConstraints *nameConstraints;
        PKIX_Boolean nameConstraintsAbsent;
        PKIX_Boolean cacheFlag;
        PKIX_CertStore *store;
        PKIX_List *authorityInfoAccess; 
        PKIX_List *subjectInfoAccess; 
        PKIX_Boolean isUserTrustAnchor;
        PKIX_List *crldpList; 


 
};



PKIX_Error *
pkix_pl_Cert_RegisterSelf(void *plContext);

PKIX_Error *
pkix_pl_Cert_CreateWithNSSCert(
        CERTCertificate *nssCert,
        PKIX_PL_Cert **pCert,
        void *plContext);

PKIX_Error *
pkix_pl_Cert_CreateToList(
        SECItem *derCertItem,
        PKIX_List *certList,
        void *plContext);

PKIX_Error *
pkix_pl_Cert_CheckSubjectAltNameConstraints(
        PKIX_PL_Cert *cert,
        PKIX_PL_CertNameConstraints *nameConstraints,
        PKIX_Boolean matchAll,
        void *plContext);

PKIX_Error *
pkix_pl_Cert_ToString_Helper(
        PKIX_PL_Cert *cert,
        PKIX_Boolean partialString,
        PKIX_PL_String **pString,
        void *plContext);

PKIX_Error *
pkix_pl_Cert_CheckExtendedKeyUsage(
        PKIX_PL_Cert *cert,
        PKIX_UInt32 requiredExtendedKeyUsages,
        PKIX_Boolean *pPass,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
