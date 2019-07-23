









































#ifndef _PKIX_CERTSEL_H
#define _PKIX_CERTSEL_H

#include "pkixt.h"

#ifdef __cplusplus
extern "C" {
#endif





























































































typedef PKIX_Error *
(*PKIX_CertSelector_MatchCallback)(
        PKIX_CertSelector *selector,
        PKIX_PL_Cert *cert,
        void *plContext);





























PKIX_Error *
PKIX_CertSelector_Create(
        PKIX_CertSelector_MatchCallback callback,
        PKIX_PL_Object *certSelectorContext,
        PKIX_CertSelector **pSelector,
        void *plContext);























PKIX_Error *
PKIX_CertSelector_GetMatchCallback(
        PKIX_CertSelector *selector,
        PKIX_CertSelector_MatchCallback *pCallback,
        void *plContext);
























PKIX_Error *
PKIX_CertSelector_GetCertSelectorContext(
        PKIX_CertSelector *selector,
        PKIX_PL_Object **pCertSelectorContext,
        void *plContext);



























PKIX_Error *
PKIX_CertSelector_GetCommonCertSelectorParams(
        PKIX_CertSelector *selector,
        PKIX_ComCertSelParams **pCommonCertSelectorParams,
        void *plContext);
























PKIX_Error *
PKIX_CertSelector_SetCommonCertSelectorParams(
        PKIX_CertSelector *selector,
        PKIX_ComCertSelParams *commonCertSelectorParams,
        void *plContext);





























PKIX_Error *
PKIX_ComCertSelParams_Create(
        PKIX_ComCertSelParams **pParams,
        void *plContext);




































PKIX_Error *
PKIX_ComCertSelParams_GetSubjAltNames(
        PKIX_ComCertSelParams *params,
        PKIX_List **pNames, 
        void *plContext);

































PKIX_Error *
PKIX_ComCertSelParams_SetSubjAltNames(
        PKIX_ComCertSelParams *params,
        PKIX_List *names,  
        void *plContext);





























PKIX_Error *
PKIX_ComCertSelParams_AddSubjAltName(
        PKIX_ComCertSelParams *params,
        PKIX_PL_GeneralName *name,
        void *plContext);

































PKIX_Error *
PKIX_ComCertSelParams_GetPathToNames(
        PKIX_ComCertSelParams *params,
        PKIX_List **pNames,  
        void *plContext);































PKIX_Error *
PKIX_ComCertSelParams_SetPathToNames(
        PKIX_ComCertSelParams *params,
        PKIX_List *names,    
        void *plContext);



























PKIX_Error *
PKIX_ComCertSelParams_AddPathToName(
        PKIX_ComCertSelParams *params,
        PKIX_PL_GeneralName *pathToName,
        void *plContext);
































PKIX_Error *
PKIX_ComCertSelParams_GetAuthorityKeyIdentifier(
        PKIX_ComCertSelParams *params,
        PKIX_PL_ByteArray **pAuthKeyId,
        void *plContext);



























PKIX_Error *
PKIX_ComCertSelParams_SetAuthorityKeyIdentifier(
        PKIX_ComCertSelParams *params,
        PKIX_PL_ByteArray *authKeyId,
        void *plContext);































PKIX_Error *
PKIX_ComCertSelParams_GetSubjKeyIdentifier(
        PKIX_ComCertSelParams *params,
        PKIX_PL_ByteArray **pSubjKeyId,
        void *plContext);



























PKIX_Error *
PKIX_ComCertSelParams_SetSubjKeyIdentifier(
        PKIX_ComCertSelParams *params,
        PKIX_PL_ByteArray *subKeyId,
        void *plContext);































PKIX_Error *
PKIX_ComCertSelParams_GetSubjPubKey(
        PKIX_ComCertSelParams *params,
        PKIX_PL_PublicKey **pPubKey,
        void *plContext);


























PKIX_Error *
PKIX_ComCertSelParams_SetSubjPubKey(
        PKIX_ComCertSelParams *params,
        PKIX_PL_PublicKey *pubKey,
        void *plContext);































PKIX_Error *
PKIX_ComCertSelParams_GetSubjPKAlgId(
        PKIX_ComCertSelParams *params,
        PKIX_PL_OID **pAlgId,
        void *plContext);































PKIX_Error *
PKIX_ComCertSelParams_SetSubjPKAlgId(
        PKIX_ComCertSelParams *params,
        PKIX_PL_OID *algId,
        void *plContext);











































PKIX_Error *
PKIX_ComCertSelParams_GetBasicConstraints(
        PKIX_ComCertSelParams *params,
        PKIX_Int32 *pMinPathLength,
        void *plContext);







































PKIX_Error *
PKIX_ComCertSelParams_SetBasicConstraints(
        PKIX_ComCertSelParams *params,
        PKIX_Int32 minPathLength,
        void *plContext);
































PKIX_Error *
PKIX_ComCertSelParams_GetCertificate(
        PKIX_ComCertSelParams *params,
        PKIX_PL_Cert **pCert,
        void *plContext);































PKIX_Error *
PKIX_ComCertSelParams_SetCertificate(
        PKIX_ComCertSelParams *params,
        PKIX_PL_Cert *cert,
        void *plContext);































PKIX_Error *
PKIX_ComCertSelParams_GetCertificateValid(
        PKIX_ComCertSelParams *params,
        PKIX_PL_Date **pDate,
        void *plContext);





























PKIX_Error *
PKIX_ComCertSelParams_SetCertificateValid(
        PKIX_ComCertSelParams *params,
        PKIX_PL_Date *date,
        void *plContext);































PKIX_Error *
PKIX_ComCertSelParams_GetSerialNumber(
        PKIX_ComCertSelParams *params,
        PKIX_PL_BigInt **pSerialNumber,
        void *plContext);






























PKIX_Error *
PKIX_ComCertSelParams_SetSerialNumber(
        PKIX_ComCertSelParams *params,
        PKIX_PL_BigInt *serialNumber,
        void *plContext);

































PKIX_Error *
PKIX_ComCertSelParams_GetVersion(
        PKIX_ComCertSelParams *params,
        PKIX_UInt32 *pVersion,
        void *plContext);




























PKIX_Error *
PKIX_ComCertSelParams_SetVersion(
        PKIX_ComCertSelParams *params,
        PKIX_Int32 version,
        void *plContext);

































PKIX_Error *
PKIX_ComCertSelParams_GetKeyUsage(
        PKIX_ComCertSelParams *params,
        PKIX_UInt32 *pKeyUsage,
        void *plContext);
































PKIX_Error *
PKIX_ComCertSelParams_SetKeyUsage(
        PKIX_ComCertSelParams *params,
        PKIX_UInt32 keyUsage,
        void *plContext);


































PKIX_Error *
PKIX_ComCertSelParams_GetExtendedKeyUsage(
        PKIX_ComCertSelParams *params,
        PKIX_List **pExtKeyUsage, 
        void *plContext);
































PKIX_Error *
PKIX_ComCertSelParams_SetExtendedKeyUsage(
        PKIX_ComCertSelParams *params,
        PKIX_List *extKeyUsage,  
        void *plContext);



































PKIX_Error *
PKIX_ComCertSelParams_GetPolicy(
        PKIX_ComCertSelParams *params,
        PKIX_List **pPolicy,  
        void *plContext);
































PKIX_Error *
PKIX_ComCertSelParams_SetPolicy(
        PKIX_ComCertSelParams *params,
        PKIX_List *policy,    
        void *plContext);































PKIX_Error *
PKIX_ComCertSelParams_GetIssuer(
        PKIX_ComCertSelParams *params,
        PKIX_PL_X500Name **pIssuer,
        void *plContext);






























PKIX_Error *
PKIX_ComCertSelParams_SetIssuer(
        PKIX_ComCertSelParams *params,
        PKIX_PL_X500Name *issuer,
        void *plContext);































PKIX_Error *
PKIX_ComCertSelParams_GetSubject(
        PKIX_ComCertSelParams *params,
        PKIX_PL_X500Name **pSubject,
        void *plContext);






























PKIX_Error *
PKIX_ComCertSelParams_SetSubject(
        PKIX_ComCertSelParams *params,
        PKIX_PL_X500Name *subject,
        void *plContext);































PKIX_Error *
PKIX_ComCertSelParams_GetSubjectAsByteArray(
        PKIX_ComCertSelParams *params,
        PKIX_PL_ByteArray **pSubject,
        void *plContext);






























PKIX_Error *
PKIX_ComCertSelParams_SetSubjectAsByteArray(
        PKIX_ComCertSelParams *params,
        PKIX_PL_ByteArray *subject,
        void *plContext);































PKIX_Error *
PKIX_ComCertSelParams_GetNameConstraints(
        PKIX_ComCertSelParams *params,
        PKIX_PL_CertNameConstraints **pConstraints,
        void *plContext);































PKIX_Error *
PKIX_ComCertSelParams_SetNameConstraints(
        PKIX_ComCertSelParams *params,
        PKIX_PL_CertNameConstraints *constraints,
        void *plContext);

































PKIX_Error *
PKIX_ComCertSelParams_GetMatchAllSubjAltNames(
        PKIX_ComCertSelParams *params,
        PKIX_Boolean *pMatch,
        void *plContext);































PKIX_Error *
PKIX_ComCertSelParams_SetMatchAllSubjAltNames(
        PKIX_ComCertSelParams *params,
        PKIX_Boolean match,
        void *plContext);

























PKIX_Error*
PKIX_ComCertSelParams_GetLeafCertFlag(
        PKIX_ComCertSelParams *params,
        PKIX_Boolean *pLeafFlag,
        void *plContext);

























PKIX_Error *
PKIX_ComCertSelParams_SetLeafCertFlag(
        PKIX_ComCertSelParams *params,
        PKIX_Boolean leafFlag,
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif
