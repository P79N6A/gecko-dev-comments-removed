









































#ifndef _PKIX_PL_PKI_H
#define _PKIX_PL_PKI_H

#include "pkixt.h"
#include "seccomon.h"
#include "certt.h"

#ifdef __cplusplus
extern "C" {
#endif















































































































PKIX_Error *
PKIX_PL_Cert_Create(
        PKIX_PL_ByteArray *byteArray,
        PKIX_PL_Cert **pCert,
        void *plContext);





















PKIX_Error *
PKIX_PL_Cert_CreateFromCERTCertificate(
        const CERTCertificate *nssCert,
        PKIX_PL_Cert **pCert,
        void *plContext);






















PKIX_Error *
PKIX_PL_Cert_GetCERTCertificate(
        PKIX_PL_Cert *cert,
        CERTCertificate **pnssCert, 
        void *plContext);

























PKIX_Error *
PKIX_PL_Cert_GetVersion(
        PKIX_PL_Cert *cert,
        PKIX_UInt32 *pVersion,
        void *plContext);
























PKIX_Error *
PKIX_PL_Cert_GetSerialNumber(
        PKIX_PL_Cert *cert,
        PKIX_PL_BigInt **pSerial,
        void *plContext);






















PKIX_Error *
PKIX_PL_Cert_GetIssuer(
        PKIX_PL_Cert *cert,
        PKIX_PL_X500Name **pIssuer,
        void *plContext);























PKIX_Error *
PKIX_PL_Cert_GetSubject(
        PKIX_PL_Cert *cert,
        PKIX_PL_X500Name **pSubject,
        void *plContext);
































PKIX_Error *
PKIX_PL_Cert_GetSubjectPublicKeyAlgId(
        PKIX_PL_Cert *cert,
        PKIX_PL_OID **pSubjKeyAlgId,
        void *plContext);



























PKIX_Error *
PKIX_PL_Cert_GetSubjectPublicKey(
        PKIX_PL_Cert *cert,
        PKIX_PL_PublicKey **pPublicKey,
        void *plContext);






















PKIX_Error *
PKIX_PL_PublicKey_NeedsDSAParameters(
        PKIX_PL_PublicKey *pubKey,
        PKIX_Boolean *pNeedsParams,
        void *plContext);







































PKIX_Error *
PKIX_PL_PublicKey_MakeInheritedDSAPublicKey(
        PKIX_PL_PublicKey *firstKey,
        PKIX_PL_PublicKey *secondKey,
        PKIX_PL_PublicKey **pResultKey,
        void *plContext);



























PKIX_Error *
PKIX_PL_Cert_GetCriticalExtensionOIDs(
        PKIX_PL_Cert *cert,
        PKIX_List **pExtensions,  
        void *plContext);




































PKIX_Error *
PKIX_PL_Cert_GetAuthorityKeyIdentifier(
        PKIX_PL_Cert *cert,
        PKIX_PL_ByteArray **pAuthKeyId,
        void *plContext);



























PKIX_Error *
PKIX_PL_Cert_GetSubjectKeyIdentifier(
        PKIX_PL_Cert *cert,
        PKIX_PL_ByteArray **pSubjKeyId,
        void *plContext);



















































PKIX_Error *
PKIX_PL_Cert_GetSubjectAltNames(
        PKIX_PL_Cert *cert,
        PKIX_List **pSubjectAltNames,  
        void *plContext);





























PKIX_Error *
PKIX_PL_Cert_GetAllSubjectNames(
        PKIX_PL_Cert *cert,
        PKIX_List **pAllSubjectNames,  
        void *plContext);































PKIX_Error *
PKIX_PL_Cert_GetExtendedKeyUsage(
        PKIX_PL_Cert *cert,
        PKIX_List **pKeyUsage,  
        void *plContext);








































PKIX_Error *
PKIX_PL_Cert_GetNameConstraints(
        PKIX_PL_Cert *cert,
        PKIX_PL_CertNameConstraints **pNameConstraints,
        void *plContext);
































PKIX_Error *
PKIX_PL_Cert_GetBasicConstraints(
        PKIX_PL_Cert *cert,
        PKIX_PL_CertBasicConstraints **pBasicConstraints,
        void *plContext);




























PKIX_Error *
PKIX_PL_BasicConstraints_GetCAFlag(
        PKIX_PL_CertBasicConstraints *basicConstraints,
        PKIX_Boolean *pResult,
        void *plContext);


























PKIX_Error *
PKIX_PL_BasicConstraints_GetPathLenConstraint(
        PKIX_PL_CertBasicConstraints *basicConstraints,
        PKIX_Int32 *pPathLenConstraint,
        void *plContext);



































PKIX_Error *
PKIX_PL_Cert_GetPolicyInformation(
        PKIX_PL_Cert *cert,
        PKIX_List **pPolicyInfo, 
        void *plContext);
































PKIX_Error *
PKIX_PL_CertPolicyInfo_GetPolicyId(
        PKIX_PL_CertPolicyInfo *policyInfo,
        PKIX_PL_OID **pCertPolicyId,
        void *plContext);







































PKIX_Error *
PKIX_PL_CertPolicyInfo_GetPolQualifiers(
        PKIX_PL_CertPolicyInfo *policyInfo,
        PKIX_List **pPolicyQualifiers, 
        void *plContext);































PKIX_Error *
PKIX_PL_PolicyQualifier_GetPolicyQualifierId(
        PKIX_PL_CertPolicyQualifier *policyQualifier,
        PKIX_PL_OID **pPolicyQualifierId,
        void *plContext);




























PKIX_Error *
PKIX_PL_PolicyQualifier_GetQualifier(
        PKIX_PL_CertPolicyQualifier *policyQualifier,
        PKIX_PL_ByteArray **pQualifier,
        void *plContext);
































PKIX_Error *
PKIX_PL_Cert_GetPolicyMappings(
        PKIX_PL_Cert *cert,
        PKIX_List **pPolicyMappings, 
        void *plContext);




























PKIX_Error *
PKIX_PL_CertPolicyMap_GetIssuerDomainPolicy(
        PKIX_PL_CertPolicyMap *policyMapping,
        PKIX_PL_OID **pIssuerDomainPolicy,
        void *plContext);




























PKIX_Error *
PKIX_PL_CertPolicyMap_GetSubjectDomainPolicy(
        PKIX_PL_CertPolicyMap *policyMapping,
        PKIX_PL_OID **pSubjectDomainPolicy,
        void *plContext);
































PKIX_Error *
PKIX_PL_Cert_GetRequireExplicitPolicy(
        PKIX_PL_Cert *cert,
        PKIX_Int32 *pSkipCerts,
        void *plContext);
































PKIX_Error *
PKIX_PL_Cert_GetPolicyMappingInhibited(
        PKIX_PL_Cert *cert,
        PKIX_Int32 *pSkipCerts,
        void *plContext);




























PKIX_Error *
PKIX_PL_Cert_GetInhibitAnyPolicy(
        PKIX_PL_Cert *cert,
        PKIX_Int32 *pSkipCerts,
        void *plContext);





























PKIX_Error *
PKIX_PL_Cert_AreCertPoliciesCritical(
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pCritical,
        void *plContext);


























PKIX_Error *
PKIX_PL_Cert_CheckNameConstraints(
        PKIX_PL_Cert *cert,
        PKIX_PL_CertNameConstraints *nameConstraints,
        void *plContext);




























PKIX_Error *
PKIX_PL_Cert_MergeNameConstraints(
        PKIX_PL_CertNameConstraints *firstNC,
        PKIX_PL_CertNameConstraints *secondNC,
        PKIX_PL_CertNameConstraints **pResultNC,
        void *plContext);





































PKIX_Error *
PKIX_PL_Cert_VerifyKeyUsage(
        PKIX_PL_Cert *cert,
        PKIX_UInt32 keyUsage,
        void *plContext);























PKIX_Error *
PKIX_PL_Cert_VerifyCertAndKeyType(
        PKIX_PL_Cert *cert,
        PKIX_Boolean isChainCert,
        void *plContext);


































PKIX_Error *
PKIX_PL_Cert_CheckValidity(
        PKIX_PL_Cert *cert,
        PKIX_PL_Date *date,
        void *plContext);




























PKIX_Error *
PKIX_PL_Cert_GetValidityNotAfter(
        PKIX_PL_Cert *cert,
        PKIX_PL_Date **pDate,
        void *plContext);























PKIX_Error *
PKIX_PL_Cert_VerifySignature(
        PKIX_PL_Cert *cert,
        PKIX_PL_PublicKey *pubKey,
        void *plContext);
































PKIX_Error *
PKIX_PL_Cert_IsCertTrusted(
        PKIX_PL_Cert *cert,
        PKIX_Boolean trustOnlyUserAnchors,
        PKIX_Boolean *pTrusted,
        void *plContext);


PKIX_Error*
PKIX_PL_Cert_SetAsTrustAnchor(PKIX_PL_Cert *cert, 
                              void *plContext);
























PKIX_Error *
PKIX_PL_Cert_GetCacheFlag(
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pCacheFlag,
        void *plContext);























PKIX_Error *
PKIX_PL_Cert_SetCacheFlag(
        PKIX_PL_Cert *cert,
        PKIX_Boolean cacheFlag,
        void *plContext);






















PKIX_Error *
PKIX_PL_Cert_GetTrustCertStore(
        PKIX_PL_Cert *cert,
        PKIX_CertStore **pTrustCertStore,
        void *plContext);





















PKIX_Error *
PKIX_PL_Cert_SetTrustCertStore(
        PKIX_PL_Cert *cert,
        PKIX_CertStore *trustCertStore,
        void *plContext);
































PKIX_Error *
PKIX_PL_Cert_GetAuthorityInfoAccess(
        PKIX_PL_Cert *cert,
        PKIX_List **pAiaList, 
        void *plContext);
































PKIX_Error *
PKIX_PL_Cert_GetSubjectInfoAccess(
        PKIX_PL_Cert *cert,
        PKIX_List **pSiaList, 
        void *plContext);


























PKIX_Error *
PKIX_PL_Cert_GetCrlDp(PKIX_PL_Cert *cert,
                      PKIX_List **pDpList,
                      void *plContext);









#define PKIX_INFOACCESS_OCSP          1
#define PKIX_INFOACCESS_CA_ISSUERS    2
#define PKIX_INFOACCESS_TIMESTAMPING  3
#define PKIX_INFOACCESS_CA_REPOSITORY 5

#define PKIX_INFOACCESS_LOCATION_UNKNOWN 0
#define PKIX_INFOACCESS_LOCATION_HTTP    1
#define PKIX_INFOACCESS_LOCATION_LDAP    2






























PKIX_Error *
PKIX_PL_InfoAccess_GetMethod(
        PKIX_PL_InfoAccess *infoAccess,
        PKIX_UInt32 *pMethod,
        void *plContext);






























PKIX_Error *
PKIX_PL_InfoAccess_GetLocation(
        PKIX_PL_InfoAccess *infoAccess,
        PKIX_PL_GeneralName **pLocation,
        void *plContext);






























PKIX_Error *
PKIX_PL_InfoAccess_GetLocationType(
        PKIX_PL_InfoAccess *infoAccess,
        PKIX_UInt32 *pType,
        void *plContext);

PKIX_Error *
pkix_pl_InfoAccess_GetAIACerts(
        PKIX_PL_InfoAccess *ia,
        void **pNBIOContext,
        void **pHandle,
        PKIX_List **pCerts,
        void *plContext);

























































PKIX_Error *
PKIX_PL_CRL_Create(
        PKIX_PL_ByteArray *byteArray,
        PKIX_PL_CRL **pCRL,
        void *plContext);






















PKIX_Error *
PKIX_PL_CRL_GetIssuer(
        PKIX_PL_CRL *crl,
        PKIX_PL_X500Name **pCRLIssuer,
        void *plContext);



























PKIX_Error *
PKIX_PL_CRL_GetCriticalExtensionOIDs(
        PKIX_PL_CRL *crl,
        PKIX_List **pExtensions,   
        void *plContext);



























PKIX_Error *
PKIX_PL_CRL_GetCRLEntryForSerialNumber(
        PKIX_PL_CRL *crl,
        PKIX_PL_BigInt *serialNumber,
        PKIX_PL_CRLEntry **pCRLEntry,
        void *plContext);




















PKIX_Error *
PKIX_PL_CRL_GetCRLNumber(
        PKIX_PL_CRL *crl,
        PKIX_PL_BigInt **pCrlNumber,
        void *plContext);






























PKIX_Error *
PKIX_PL_CRL_VerifyUpdateTime(
        PKIX_PL_CRL *crl,
        PKIX_PL_Date *date,
        PKIX_Boolean *pResult,
        void *plContext);























PKIX_Error *
PKIX_PL_CRL_VerifySignature(
        PKIX_PL_CRL *crl,
        PKIX_PL_PublicKey *pubKey,
        void *plContext);























PKIX_Error *
PKIX_PL_CRL_ReleaseDerCrl(PKIX_PL_CRL *crl,
                         SECItem **derCrl,
                         void *plContext);





















PKIX_Error *
PKIX_PL_CRL_AdoptDerCrl(PKIX_PL_CRL *crl,
                        SECItem *derCrl,
                        void *plContext);




































PKIX_Error *
PKIX_PL_CRLEntry_GetCRLEntryReasonCode(
        PKIX_PL_CRLEntry *crlEntry,
        PKIX_Int32 *pReason,
        void *plContext);



























PKIX_Error *
PKIX_PL_CRLEntry_GetCriticalExtensionOIDs(
        PKIX_PL_CRLEntry *crlEntry,
        PKIX_List **pExtensions,  
        void *plContext);

#ifdef BUILD_LIBPKIX_TESTS













































PKIX_Error *
PKIX_PL_X500Name_Create (
        PKIX_PL_String *stringRep,
        PKIX_PL_X500Name **pName,
        void *plContext);

#endif 



























PKIX_Error *
PKIX_PL_X500Name_CreateFromCERTName(
        SECItem *derName,
        CERTName *name,
        PKIX_PL_X500Name **pName,
        void *plContext);












































PKIX_Error *
PKIX_PL_X500Name_Match(
        PKIX_PL_X500Name *firstX500Name,
        PKIX_PL_X500Name *secondX500Name,
        PKIX_Boolean *pResult,
        void *plContext);








































PKIX_Error *
PKIX_PL_Date_Create_UTCTime (
        PKIX_PL_String *stringRep,
        PKIX_PL_Date **pDate,
        void *plContext);




















PKIX_Error *
PKIX_PL_Date_CreateFromPRTime(
        PRTime time,
        PKIX_PL_Date **pDate,
        void *plContext);






















PKIX_Error *
PKIX_PL_Date_Create_CurrentOffBySeconds(
        PKIX_Int32 secondsOffset,
        PKIX_PL_Date **pDate,
        void *plContext);

#ifdef BUILD_LIBPKIX_TESTS















































PKIX_Error *
PKIX_PL_GeneralName_Create (
        PKIX_UInt32 nameType,
        PKIX_PL_String *stringRep,
        PKIX_PL_GeneralName **pGName,
        void *plContext);
#endif 




























PKIX_Error *
PKIX_PL_CertNameConstraints_CheckNamesInNameSpace(
        PKIX_List *nameList, 
        PKIX_PL_CertNameConstraints *nameConstraints,
        PKIX_Boolean *pCheckPass,
        void *plContext);



















PKIX_Error *
PKIX_PL_AIAMgr_Create(
        PKIX_PL_AIAMgr **pAIAMgr,
        void *plContext);














































PKIX_Error *
PKIX_PL_AIAMgr_GetAIACerts(
        PKIX_PL_AIAMgr *aiaMgr,
        PKIX_PL_Cert *prevCert,
        void **pNBIOContext,
        PKIX_List **pCerts,
        void *plContext);

typedef PKIX_Error *
(*PKIX_PL_VerifyCallback)(
        PKIX_PL_Object *signedObject,
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
