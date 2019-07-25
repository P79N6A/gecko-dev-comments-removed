



































#ifndef NSSPKI_H
#define NSSPKI_H

#ifdef DEBUG
static const char NSSPKI_CVS_ID[] = "@(#) $RCSfile: nsspki.h,v $ $Revision: 1.13 $ $Date: 2010/05/21 00:02:48 $";
#endif 







#ifndef NSSDEVT_H
#include "nssdevt.h"
#endif 

#ifndef NSSPKIT_H
#include "nsspkit.h"
#endif 

#ifndef BASE_H
#include "base.h"
#endif 

PR_BEGIN_EXTERN_C



























































NSS_EXTERN PRStatus
NSSCertificate_Destroy
(
  NSSCertificate *c
);










NSS_EXTERN PRStatus
NSSCertificate_DeleteStoredObject
(
  NSSCertificate *c,
  NSSCallback *uhh
);








NSS_EXTERN PRStatus
NSSCertificate_Validate
(
  NSSCertificate *c,
  NSSTime *timeOpt, 
  NSSUsage *usage,
  NSSPolicies *policiesOpt 
);


















NSS_EXTERN void ** 
NSSCertificate_ValidateCompletely
(
  NSSCertificate *c,
  NSSTime *timeOpt, 
  NSSUsage *usage,
  NSSPolicies *policiesOpt, 
  void **rvOpt, 
  PRUint32 rvLimit, 
  NSSArena *arenaOpt 
);







NSS_EXTERN PRStatus
NSSCertificate_ValidateAndDiscoverUsagesAndPolicies
(
  NSSCertificate *c,
  NSSTime **notBeforeOutOpt,
  NSSTime **notAfterOutOpt,
  void *allowedUsages,
  void *disallowedUsages,
  void *allowedPolicies,
  void *disallowedPolicies,
  
  NSSArena *arenaOpt
);






NSS_EXTERN NSSDER *
NSSCertificate_Encode
(
  NSSCertificate *c,
  NSSDER *rvOpt,
  NSSArena *arenaOpt
);
















extern const NSSError NSS_ERROR_CERTIFICATE_ISSUER_NOT_FOUND;

NSS_EXTERN NSSCertificate **
NSSCertificate_BuildChain
(
  NSSCertificate *c,
  NSSTime *timeOpt,
  NSSUsage *usage,
  NSSPolicies *policiesOpt,
  NSSCertificate **rvOpt,
  PRUint32 rvLimit, 
  NSSArena *arenaOpt,
  PRStatus *statusOpt,
  NSSTrustDomain *td,
  NSSCryptoContext *cc 
);






NSS_EXTERN NSSTrustDomain *
NSSCertificate_GetTrustDomain
(
  NSSCertificate *c
);







NSS_EXTERN NSSToken *
NSSCertificate_GetToken
(
  NSSCertificate *c,
  PRStatus *statusOpt
);







NSS_EXTERN NSSSlot *
NSSCertificate_GetSlot
(
  NSSCertificate *c,
  PRStatus *statusOpt
);







NSS_EXTERN NSSModule *
NSSCertificate_GetModule
(
  NSSCertificate *c,
  PRStatus *statusOpt
);








NSS_EXTERN NSSItem *
NSSCertificate_Encrypt
(
  NSSCertificate *c,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *data,
  NSSTime *timeOpt,
  NSSUsage *usage,
  NSSPolicies *policiesOpt,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN PRStatus
NSSCertificate_Verify
(
  NSSCertificate *c,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *data,
  NSSItem *signature,
  NSSTime *timeOpt,
  NSSUsage *usage,
  NSSPolicies *policiesOpt,
  NSSCallback *uhh
);






NSS_EXTERN NSSItem *
NSSCertificate_VerifyRecover
(
  NSSCertificate *c,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *signature,
  NSSTime *timeOpt,
  NSSUsage *usage,
  NSSPolicies *policiesOpt,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);









NSS_EXTERN NSSItem *
NSSCertificate_WrapSymmetricKey
(
  NSSCertificate *c,
  NSSAlgorithmAndParameters *apOpt,
  NSSSymmetricKey *keyToWrap,
  NSSTime *timeOpt,
  NSSUsage *usage,
  NSSPolicies *policiesOpt,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);








NSS_EXTERN NSSCryptoContext *
NSSCertificate_CreateCryptoContext
(
  NSSCertificate *c,
  NSSAlgorithmAndParameters *apOpt,
  NSSTime *timeOpt,
  NSSUsage *usage,
  NSSPolicies *policiesOpt,
  NSSCallback *uhh  
);







NSS_EXTERN NSSPublicKey *
NSSCertificate_GetPublicKey
(
  NSSCertificate *c
);










NSS_EXTERN NSSPrivateKey *
NSSCertificate_FindPrivateKey
(
  NSSCertificate *c,
  NSSCallback *uhh
);










NSS_EXTERN PRBool
NSSCertificate_IsPrivateKeyAvailable
(
  NSSCertificate *c,
  NSSCallback *uhh,
  PRStatus *statusOpt
);






























NSS_EXTERN PRBool
NSSUserCertificate_IsStillPresent
(
  NSSUserCertificate *uc,
  PRStatus *statusOpt
);








NSS_EXTERN NSSItem *
NSSUserCertificate_Decrypt
(
  NSSUserCertificate *uc,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *data,
  NSSTime *timeOpt,
  NSSUsage *usage,
  NSSPolicies *policiesOpt,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSItem *
NSSUserCertificate_Sign
(
  NSSUserCertificate *uc,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *data,
  NSSTime *timeOpt,
  NSSUsage *usage,
  NSSPolicies *policiesOpt,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSItem *
NSSUserCertificate_SignRecover
(
  NSSUserCertificate *uc,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *data,
  NSSTime *timeOpt,
  NSSUsage *usage,
  NSSPolicies *policiesOpt,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSSymmetricKey *
NSSUserCertificate_UnwrapSymmetricKey
(
  NSSUserCertificate *uc,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *wrappedKey,
  NSSTime *timeOpt,
  NSSUsage *usage,
  NSSPolicies *policiesOpt,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSSymmetricKey *
NSSUserCertificate_DeriveSymmetricKey
(
  NSSUserCertificate *uc, 
  NSSCertificate *c, 
  NSSAlgorithmAndParameters *apOpt,
  NSSOID *target,
  PRUint32 keySizeOpt, 
  NSSOperations operations,
  NSSCallback *uhh
);


















NSS_EXTERN PRStatus
NSSPrivateKey_Destroy
(
  NSSPrivateKey *vk
);








NSS_EXTERN PRStatus
NSSPrivateKey_DeleteStoredObject
(
  NSSPrivateKey *vk,
  NSSCallback *uhh
);






NSS_EXTERN PRUint32
NSSPrivateKey_GetSignatureLength
(
  NSSPrivateKey *vk
);






NSS_EXTERN PRUint32
NSSPrivateKey_GetPrivateModulusLength
(
  NSSPrivateKey *vk
);






NSS_EXTERN PRBool
NSSPrivateKey_IsStillPresent
(
  NSSPrivateKey *vk,
  PRStatus *statusOpt
);






NSS_EXTERN NSSItem *
NSSPrivateKey_Encode
(
  NSSPrivateKey *vk,
  NSSAlgorithmAndParameters *ap,
  NSSItem *passwordOpt, 
  NSSCallback *uhhOpt,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);







NSS_EXTERN NSSTrustDomain *
NSSPrivateKey_GetTrustDomain
(
  NSSPrivateKey *vk,
  PRStatus *statusOpt
);






NSS_EXTERN NSSToken *
NSSPrivateKey_GetToken
(
  NSSPrivateKey *vk
);






NSS_EXTERN NSSSlot *
NSSPrivateKey_GetSlot
(
  NSSPrivateKey *vk
);






NSS_EXTERN NSSModule *
NSSPrivateKey_GetModule
(
  NSSPrivateKey *vk
);






NSS_EXTERN NSSItem *
NSSPrivateKey_Decrypt
(
  NSSPrivateKey *vk,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *encryptedData,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSItem *
NSSPrivateKey_Sign
(
  NSSPrivateKey *vk,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *data,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSItem *
NSSPrivateKey_SignRecover
(
  NSSPrivateKey *vk,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *data,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSSymmetricKey *
NSSPrivateKey_UnwrapSymmetricKey
(
  NSSPrivateKey *vk,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *wrappedKey,
  NSSCallback *uhh
);






NSS_EXTERN NSSSymmetricKey *
NSSPrivateKey_DeriveSymmetricKey
(
  NSSPrivateKey *vk,
  NSSPublicKey *bk,
  NSSAlgorithmAndParameters *apOpt,
  NSSOID *target,
  PRUint32 keySizeOpt, 
  NSSOperations operations,
  NSSCallback *uhh
);






NSS_EXTERN NSSPublicKey *
NSSPrivateKey_FindPublicKey
(
  NSSPrivateKey *vk
  
);








NSS_EXTERN NSSCryptoContext *
NSSPrivateKey_CreateCryptoContext
(
  NSSPrivateKey *vk,
  NSSAlgorithmAndParameters *apOpt,
  NSSCallback *uhh
);









NSS_EXTERN NSSCertificate **
NSSPrivateKey_FindCertificates
(
  NSSPrivateKey *vk,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt, 
  NSSArena *arenaOpt
);








NSS_EXTERN NSSCertificate *
NSSPrivateKey_FindBestCertificate
(
  NSSPrivateKey *vk,
  NSSTime *timeOpt,
  NSSUsage *usageOpt,
  NSSPolicies *policiesOpt
);
















NSS_EXTERN PRStatus
NSSPublicKey_Destroy
(
  NSSPublicKey *bk
);








NSS_EXTERN PRStatus
NSSPublicKey_DeleteStoredObject
(
  NSSPublicKey *bk,
  NSSCallback *uhh
);






NSS_EXTERN NSSItem *
NSSPublicKey_Encode
(
  NSSPublicKey *bk,
  NSSAlgorithmAndParameters *ap,
  NSSCallback *uhhOpt,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);







NSS_EXTERN NSSTrustDomain *
NSSPublicKey_GetTrustDomain
(
  NSSPublicKey *bk,
  PRStatus *statusOpt
);







NSS_EXTERN NSSToken *
NSSPublicKey_GetToken
(
  NSSPublicKey *bk,
  PRStatus *statusOpt
);







NSS_EXTERN NSSSlot *
NSSPublicKey_GetSlot
(
  NSSPublicKey *bk,
  PRStatus *statusOpt
);







NSS_EXTERN NSSModule *
NSSPublicKey_GetModule
(
  NSSPublicKey *bk,
  PRStatus *statusOpt
);








NSS_EXTERN NSSItem *
NSSPublicKey_Encrypt
(
  NSSPublicKey *bk,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *data,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN PRStatus
NSSPublicKey_Verify
(
  NSSPublicKey *bk,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *data,
  NSSItem *signature,
  NSSCallback *uhh
);






NSS_EXTERN NSSItem *
NSSPublicKey_VerifyRecover
(
  NSSPublicKey *bk,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *signature,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSItem *
NSSPublicKey_WrapSymmetricKey
(
  NSSPublicKey *bk,
  NSSAlgorithmAndParameters *apOpt,
  NSSSymmetricKey *keyToWrap,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);








NSS_EXTERN NSSCryptoContext *
NSSPublicKey_CreateCryptoContext
(
  NSSPublicKey *bk,
  NSSAlgorithmAndParameters *apOpt,
  NSSCallback *uhh
);












NSS_EXTERN NSSCertificate **
NSSPublicKey_FindCertificates
(
  NSSPublicKey *bk,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt, 
  NSSArena *arenaOpt
);








NSS_EXTERN NSSCertificate *
NSSPublicKey_FindBestCertificate
(
  NSSPublicKey *bk,
  NSSTime *timeOpt,
  NSSUsage *usageOpt,
  NSSPolicies *policiesOpt
);






NSS_EXTERN NSSPrivateKey *
NSSPublicKey_FindPrivateKey
(
  NSSPublicKey *bk,
  NSSCallback *uhh
);












NSS_EXTERN PRStatus
NSSSymmetricKey_Destroy
(
  NSSSymmetricKey *mk
);







NSS_EXTERN PRStatus
NSSSymmetricKey_DeleteStoredObject
(
  NSSSymmetricKey *mk,
  NSSCallback *uhh
);






NSS_EXTERN PRUint32
NSSSymmetricKey_GetKeyLength
(
  NSSSymmetricKey *mk
);






NSS_EXTERN PRUint32
NSSSymmetricKey_GetKeyStrength
(
  NSSSymmetricKey *mk
);






NSS_EXTERN PRStatus
NSSSymmetricKey_IsStillPresent
(
  NSSSymmetricKey *mk
);







NSS_EXTERN NSSTrustDomain *
NSSSymmetricKey_GetTrustDomain
(
  NSSSymmetricKey *mk,
  PRStatus *statusOpt
);







NSS_EXTERN NSSToken *
NSSSymmetricKey_GetToken
(
  NSSSymmetricKey *mk,
  PRStatus *statusOpt
);







NSS_EXTERN NSSSlot *
NSSSymmetricKey_GetSlot
(
  NSSSymmetricKey *mk,
  PRStatus *statusOpt
);







NSS_EXTERN NSSModule *
NSSSymmetricKey_GetModule
(
  NSSSymmetricKey *mk,
  PRStatus *statusOpt
);






NSS_EXTERN NSSItem *
NSSSymmetricKey_Encrypt
(
  NSSSymmetricKey *mk,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *data,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSItem *
NSSSymmetricKey_Decrypt
(
  NSSSymmetricKey *mk,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *encryptedData,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSItem *
NSSSymmetricKey_Sign
(
  NSSSymmetricKey *mk,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *data,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSItem *
NSSSymmetricKey_SignRecover
(
  NSSSymmetricKey *mk,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *data,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN PRStatus
NSSSymmetricKey_Verify
(
  NSSSymmetricKey *mk,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *data,
  NSSItem *signature,
  NSSCallback *uhh
);






NSS_EXTERN NSSItem *
NSSSymmetricKey_VerifyRecover
(
  NSSSymmetricKey *mk,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *signature,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSItem *
NSSSymmetricKey_WrapSymmetricKey
(
  NSSSymmetricKey *wrappingKey,
  NSSAlgorithmAndParameters *apOpt,
  NSSSymmetricKey *keyToWrap,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSItem *
NSSSymmetricKey_WrapPrivateKey
(
  NSSSymmetricKey *wrappingKey,
  NSSAlgorithmAndParameters *apOpt,
  NSSPrivateKey *keyToWrap,
  NSSCallback *uhh,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSSymmetricKey *
NSSSymmetricKey_UnwrapSymmetricKey
(
  NSSSymmetricKey *wrappingKey,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *wrappedKey,
  NSSOID *target,
  PRUint32 keySizeOpt,
  NSSOperations operations,
  NSSCallback *uhh
);






NSS_EXTERN NSSPrivateKey *
NSSSymmetricKey_UnwrapPrivateKey
(
  NSSSymmetricKey *wrappingKey,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *wrappedKey,
  NSSUTF8 *labelOpt,
  NSSItem *keyIDOpt,
  PRBool persistant,
  PRBool sensitive,
  NSSToken *destinationOpt,
  NSSCallback *uhh
);






NSS_EXTERN NSSSymmetricKey *
NSSSymmetricKey_DeriveSymmetricKey
(
  NSSSymmetricKey *originalKey,
  NSSAlgorithmAndParameters *apOpt,
  NSSOID *target,
  PRUint32 keySizeOpt,
  NSSOperations operations,
  NSSCallback *uhh
);








NSS_EXTERN NSSCryptoContext *
NSSSymmetricKey_CreateCryptoContext
(
  NSSSymmetricKey *mk,
  NSSAlgorithmAndParameters *apOpt,
  NSSCallback *uhh
);

























NSS_EXTERN NSSTrustDomain *
NSSTrustDomain_Create
(
  NSSUTF8 *moduleOpt,
  NSSUTF8 *uriOpt,
  NSSUTF8 *opaqueOpt,
  void *reserved
);






NSS_EXTERN PRStatus
NSSTrustDomain_Destroy
(
  NSSTrustDomain *td
);






NSS_EXTERN PRStatus
NSSTrustDomain_SetDefaultCallback
(
  NSSTrustDomain *td,
  NSSCallback *newCallback,
  NSSCallback **oldCallbackOpt
);






NSS_EXTERN NSSCallback *
NSSTrustDomain_GetDefaultCallback
(
  NSSTrustDomain *td,
  PRStatus *statusOpt
);












NSS_EXTERN PRStatus
NSSTrustDomain_LoadModule
(
  NSSTrustDomain *td,
  NSSUTF8 *moduleOpt,
  NSSUTF8 *uriOpt,
  NSSUTF8 *opaqueOpt,
  void *reserved
);















NSS_EXTERN PRStatus
NSSTrustDomain_DisableToken
(
  NSSTrustDomain *td,
  NSSToken *token,
  NSSError why
);






NSS_EXTERN PRStatus
NSSTrustDomain_EnableToken
(
  NSSTrustDomain *td,
  NSSToken *token
);








NSS_EXTERN PRStatus
NSSTrustDomain_IsTokenEnabled
(
  NSSTrustDomain *td,
  NSSToken *token,
  NSSError *whyOpt
);






NSS_EXTERN NSSSlot *
NSSTrustDomain_FindSlotByName
(
  NSSTrustDomain *td,
  NSSUTF8 *slotName
);






NSS_EXTERN NSSToken *
NSSTrustDomain_FindTokenByName
(
  NSSTrustDomain *td,
  NSSUTF8 *tokenName
);






NSS_EXTERN NSSToken *
NSSTrustDomain_FindTokenBySlotName
(
  NSSTrustDomain *td,
  NSSUTF8 *slotName
);






NSS_EXTERN NSSToken *
NSSTrustDomain_FindTokenForAlgorithm
(
  NSSTrustDomain *td,
  NSSOID *algorithm
);






NSS_EXTERN NSSToken *
NSSTrustDomain_FindBestTokenForAlgorithms
(
  NSSTrustDomain *td,
  NSSOID *algorithms[], 
  PRUint32 nAlgorithmsOpt 
);






NSS_EXTERN PRStatus
NSSTrustDomain_Login
(
  NSSTrustDomain *td,
  NSSCallback *uhhOpt
);






NSS_EXTERN PRStatus
NSSTrustDomain_Logout
(
  NSSTrustDomain *td
);










NSS_EXTERN NSSCertificate *
NSSTrustDomain_ImportCertificate
(
  NSSTrustDomain *td,
  NSSCertificate *c
);






NSS_EXTERN NSSCertificate *
NSSTrustDomain_ImportPKIXCertificate
(
  NSSTrustDomain *td,
  
  struct NSSPKIXCertificateStr *pc
);







NSS_EXTERN NSSCertificate *
NSSTrustDomain_ImportEncodedCertificate
(
  NSSTrustDomain *td,
  NSSBER *ber
);







NSS_EXTERN NSSCertificate **
NSSTrustDomain_ImportEncodedCertificateChain
(
  NSSTrustDomain *td,
  NSSBER *ber,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt, 
  NSSArena *arenaOpt
);






NSS_EXTERN NSSPrivateKey *
NSSTrustDomain_ImportEncodedPrivateKey
(
  NSSTrustDomain *td,
  NSSBER *ber,
  NSSItem *passwordOpt, 
  NSSCallback *uhhOpt,
  NSSToken *destination
);






NSS_EXTERN NSSPublicKey *
NSSTrustDomain_ImportEncodedPublicKey
(
  NSSTrustDomain *td,
  NSSBER *ber
);








NSS_EXTERN NSSCertificate *
NSSTrustDomain_FindBestCertificateByNickname
(
  NSSTrustDomain *td,
  const NSSUTF8 *name,
  NSSTime *timeOpt, 
  NSSUsage *usage,
  NSSPolicies *policiesOpt 
);






NSS_EXTERN NSSCertificate **
NSSTrustDomain_FindCertificatesByNickname
(
  NSSTrustDomain *td,
  NSSUTF8 *name,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt, 
  NSSArena *arenaOpt
);






NSS_EXTERN NSSCertificate *
NSSTrustDomain_FindCertificateByIssuerAndSerialNumber
(
  NSSTrustDomain *td,
  NSSDER *issuer,
  NSSDER *serialNumber
);















NSS_EXTERN NSSCertificate *
NSSTrustDomain_FindBestCertificateBySubject
(
  NSSTrustDomain *td,
  NSSDER  *subject,
  NSSTime *timeOpt,
  NSSUsage *usage,
  NSSPolicies *policiesOpt
);







NSS_EXTERN NSSCertificate **
NSSTrustDomain_FindCertificatesBySubject
(
  NSSTrustDomain *td,
  NSSDER  *subject,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt, 
  NSSArena *arenaOpt
);










NSS_EXTERN NSSCertificate *
NSSTrustDomain_FindBestCertificateByNameComponents
(
  NSSTrustDomain *td,
  NSSUTF8 *nameComponents,
  NSSTime *timeOpt,
  NSSUsage *usage,
  NSSPolicies *policiesOpt
);









NSS_EXTERN NSSCertificate **
NSSTrustDomain_FindCertificatesByNameComponents
(
  NSSTrustDomain *td,
  NSSUTF8 *nameComponents,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt, 
  NSSArena *arenaOpt
);






NSS_EXTERN NSSCertificate *
NSSTrustDomain_FindCertificateByEncodedCertificate
(
  NSSTrustDomain *td,
  NSSBER *encodedCertificate
);






NSS_EXTERN NSSCertificate *
NSSTrustDomain_FindCertificateByEmail
(
  NSSTrustDomain *td,
  NSSASCII7 *email,
  NSSTime *timeOpt,
  NSSUsage *usage,
  NSSPolicies *policiesOpt
);






NSS_EXTERN NSSCertificate **
NSSTrustDomain_FindCertificatesByEmail
(
  NSSTrustDomain *td,
  NSSASCII7 *email,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt, 
  NSSArena *arenaOpt
);







NSS_EXTERN NSSCertificate *
NSSTrustDomain_FindCertificateByOCSPHash
(
  NSSTrustDomain *td,
  NSSItem *hash
);













 





NSS_EXTERN PRStatus *
NSSTrustDomain_TraverseCertificates
(
  NSSTrustDomain *td,
  PRStatus (*callback)(NSSCertificate *c, void *arg),
  void *arg
);






NSS_EXTERN NSSCertificate *
NSSTrustDomain_FindBestUserCertificate
(
  NSSTrustDomain *td,
  NSSTime *timeOpt,
  NSSUsage *usage,
  NSSPolicies *policiesOpt
);






NSS_EXTERN NSSCertificate **
NSSTrustDomain_FindUserCertificates
(
  NSSTrustDomain *td,
  NSSTime *timeOpt,
  NSSUsage *usageOpt,
  NSSPolicies *policiesOpt,
  NSSCertificate **rvOpt,
  PRUint32 rvLimit, 
  NSSArena *arenaOpt
);






NSS_EXTERN NSSCertificate *
NSSTrustDomain_FindBestUserCertificateForSSLClientAuth
(
  NSSTrustDomain *td,
  NSSUTF8 *sslHostOpt,
  NSSDER *rootCAsOpt[], 
  PRUint32 rootCAsMaxOpt, 
  NSSAlgorithmAndParameters *apOpt,
  NSSPolicies *policiesOpt
);






NSS_EXTERN NSSCertificate **
NSSTrustDomain_FindUserCertificatesForSSLClientAuth
(
  NSSTrustDomain *td,
  NSSUTF8 *sslHostOpt,
  NSSDER *rootCAsOpt[], 
  PRUint32 rootCAsMaxOpt, 
  NSSAlgorithmAndParameters *apOpt,
  NSSPolicies *policiesOpt,
  NSSCertificate **rvOpt,
  PRUint32 rvLimit, 
  NSSArena *arenaOpt
);






NSS_EXTERN NSSCertificate *
NSSTrustDomain_FindBestUserCertificateForEmailSigning
(
  NSSTrustDomain *td,
  NSSASCII7 *signerOpt,
  NSSASCII7 *recipientOpt,
  
  NSSAlgorithmAndParameters *apOpt,
  NSSPolicies *policiesOpt
);






NSS_EXTERN NSSCertificate **
NSSTrustDomain_FindUserCertificatesForEmailSigning
(
  NSSTrustDomain *td,
  NSSASCII7 *signerOpt,
  NSSASCII7 *recipientOpt,
  
  NSSAlgorithmAndParameters *apOpt,
  NSSPolicies *policiesOpt,
  NSSCertificate **rvOpt,
  PRUint32 rvLimit, 
  NSSArena *arenaOpt
);


















NSS_EXTERN PRStatus
NSSTrustDomain_GenerateKeyPair
(
  NSSTrustDomain *td,
  NSSAlgorithmAndParameters *ap,
  NSSPrivateKey **pvkOpt,
  NSSPublicKey **pbkOpt,
  PRBool privateKeyIsSensitive,
  NSSToken *destination,
  NSSCallback *uhhOpt
);





















NSS_EXTERN NSSSymmetricKey *
NSSTrustDomain_GenerateSymmetricKey
(
  NSSTrustDomain *td,
  NSSAlgorithmAndParameters *ap,
  PRUint32 keysize,
  NSSToken *destination,
  NSSCallback *uhhOpt
);






NSS_EXTERN NSSSymmetricKey *
NSSTrustDomain_GenerateSymmetricKeyFromPassword
(
  NSSTrustDomain *td,
  NSSAlgorithmAndParameters *ap,
  NSSUTF8 *passwordOpt, 
  NSSToken *destinationOpt,
  NSSCallback *uhhOpt
);




















NSS_EXTERN NSSSymmetricKey *
NSSTrustDomain_FindSymmetricKeyByAlgorithmAndKeyID
(
  NSSTrustDomain *td,
  NSSOID *algorithm,
  NSSItem *keyID,
  NSSCallback *uhhOpt
);






















NSS_EXTERN NSSCryptoContext *
NSSTrustDomain_CreateCryptoContext
(
  NSSTrustDomain *td,
  NSSCallback *uhhOpt
);






NSS_EXTERN NSSCryptoContext *
NSSTrustDomain_CreateCryptoContextForAlgorithm
(
  NSSTrustDomain *td,
  NSSOID *algorithm
);






NSS_EXTERN NSSCryptoContext *
NSSTrustDomain_CreateCryptoContextForAlgorithmAndParameters
(
  NSSTrustDomain *td,
  NSSAlgorithmAndParameters *ap
);



























NSS_EXTERN PRStatus
NSSCryptoContext_Destroy
(
  NSSCryptoContext *cc
);








NSS_EXTERN PRStatus
NSSCryptoContext_SetDefaultCallback
(
  NSSCryptoContext *cc,
  NSSCallback *newCallback,
  NSSCallback **oldCallbackOpt
);






NSS_EXTERN NSSCallback *
NSSCryptoContext_GetDefaultCallback
(
  NSSCryptoContext *cc,
  PRStatus *statusOpt
);






NSS_EXTERN NSSTrustDomain *
NSSCryptoContext_GetTrustDomain
(
  NSSCryptoContext *cc
);


























NSS_EXTERN NSSCertificate *
NSSCryptoContext_FindOrImportCertificate (
  NSSCryptoContext *cc,
  NSSCertificate *c
);






NSS_EXTERN NSSCertificate *
NSSCryptoContext_ImportPKIXCertificate
(
  NSSCryptoContext *cc,
  struct NSSPKIXCertificateStr *pc
);






NSS_EXTERN NSSCertificate *
NSSCryptoContext_ImportEncodedCertificate
(
  NSSCryptoContext *cc,
  NSSBER *ber
);






NSS_EXTERN PRStatus
NSSCryptoContext_ImportEncodedPKIXCertificateChain
(
  NSSCryptoContext *cc,
  NSSBER *ber
);









NSS_EXTERN NSSCertificate *
NSSCryptoContext_FindBestCertificateByNickname
(
  NSSCryptoContext *cc,
  const NSSUTF8 *name,
  NSSTime *timeOpt, 
  NSSUsage *usage,
  NSSPolicies *policiesOpt 
);






NSS_EXTERN NSSCertificate **
NSSCryptoContext_FindCertificatesByNickname
(
  NSSCryptoContext *cc,
  NSSUTF8 *name,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt, 
  NSSArena *arenaOpt
);






NSS_EXTERN NSSCertificate *
NSSCryptoContext_FindCertificateByIssuerAndSerialNumber
(
  NSSCryptoContext *cc,
  NSSDER *issuer,
  NSSDER *serialNumber
);







NSS_EXTERN NSSCertificate *
NSSCryptoContext_FindBestCertificateBySubject
(
  NSSCryptoContext *cc,
  NSSDER  *subject,
  NSSTime *timeOpt,
  NSSUsage *usage,
  NSSPolicies *policiesOpt
);







NSS_EXTERN NSSCertificate **
NSSCryptoContext_FindCertificatesBySubject
(
  NSSCryptoContext *cc,
  NSSDER  *subject,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt, 
  NSSArena *arenaOpt
);










NSS_EXTERN NSSCertificate *
NSSCryptoContext_FindBestCertificateByNameComponents
(
  NSSCryptoContext *cc,
  NSSUTF8 *nameComponents,
  NSSTime *timeOpt,
  NSSUsage *usage,
  NSSPolicies *policiesOpt
);









NSS_EXTERN NSSCertificate **
NSSCryptoContext_FindCertificatesByNameComponents
(
  NSSCryptoContext *cc,
  NSSUTF8 *nameComponents,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt, 
  NSSArena *arenaOpt
);






NSS_EXTERN NSSCertificate *
NSSCryptoContext_FindCertificateByEncodedCertificate
(
  NSSCryptoContext *cc,
  NSSBER *encodedCertificate
);






NSS_EXTERN NSSCertificate *
NSSCryptoContext_FindBestCertificateByEmail
(
  NSSCryptoContext *cc,
  NSSASCII7 *email,
  NSSTime *timeOpt,
  NSSUsage *usage,
  NSSPolicies *policiesOpt
);






NSS_EXTERN NSSCertificate **
NSSCryptoContext_FindCertificatesByEmail
(
  NSSCryptoContext *cc,
  NSSASCII7 *email,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt, 
  NSSArena *arenaOpt
);






NSS_EXTERN NSSCertificate *
NSSCryptoContext_FindCertificateByOCSPHash
(
  NSSCryptoContext *cc,
  NSSItem *hash
);



















NSS_EXTERN NSSCertificate *
NSSCryptoContext_FindBestUserCertificate
(
  NSSCryptoContext *cc,
  NSSTime *timeOpt,
  NSSUsage *usage,
  NSSPolicies *policiesOpt
);






NSS_EXTERN NSSCertificate **
NSSCryptoContext_FindUserCertificates
(
  NSSCryptoContext *cc,
  NSSTime *timeOpt,
  NSSUsage *usageOpt,
  NSSPolicies *policiesOpt,
  NSSCertificate **rvOpt,
  PRUint32 rvLimit, 
  NSSArena *arenaOpt
);






NSS_EXTERN NSSCertificate *
NSSCryptoContext_FindBestUserCertificateForSSLClientAuth
(
  NSSCryptoContext *cc,
  NSSUTF8 *sslHostOpt,
  NSSDER *rootCAsOpt[], 
  PRUint32 rootCAsMaxOpt, 
  NSSAlgorithmAndParameters *apOpt,
  NSSPolicies *policiesOpt
);






NSS_EXTERN NSSCertificate **
NSSCryptoContext_FindUserCertificatesForSSLClientAuth
(
  NSSCryptoContext *cc,
  NSSUTF8 *sslHostOpt,
  NSSDER *rootCAsOpt[], 
  PRUint32 rootCAsMaxOpt, 
  NSSAlgorithmAndParameters *apOpt,
  NSSPolicies *policiesOpt,
  NSSCertificate **rvOpt,
  PRUint32 rvLimit, 
  NSSArena *arenaOpt
);






NSS_EXTERN NSSCertificate *
NSSCryptoContext_FindBestUserCertificateForEmailSigning
(
  NSSCryptoContext *cc,
  NSSASCII7 *signerOpt,
  NSSASCII7 *recipientOpt,
  
  NSSAlgorithmAndParameters *apOpt,
  NSSPolicies *policiesOpt
);






NSS_EXTERN NSSCertificate *
NSSCryptoContext_FindUserCertificatesForEmailSigning
(
  NSSCryptoContext *cc,
  NSSASCII7 *signerOpt, 
  NSSASCII7 *recipientOpt,
  
  NSSAlgorithmAndParameters *apOpt,
  NSSPolicies *policiesOpt,
  NSSCertificate **rvOpt,
  PRUint32 rvLimit, 
  NSSArena *arenaOpt
);













NSS_EXTERN PRStatus
NSSCryptoContext_GenerateKeyPair
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *ap,
  NSSPrivateKey **pvkOpt,
  NSSPublicKey **pbkOpt,
  PRBool privateKeyIsSensitive,
  NSSToken *destination,
  NSSCallback *uhhOpt
);





















NSS_EXTERN NSSSymmetricKey *
NSSCryptoContext_GenerateSymmetricKey
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *ap,
  PRUint32 keysize,
  NSSToken *destination,
  NSSCallback *uhhOpt
);






NSS_EXTERN NSSSymmetricKey *
NSSCryptoContext_GenerateSymmetricKeyFromPassword
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *ap,
  NSSUTF8 *passwordOpt, 
  NSSToken *destinationOpt,
  NSSCallback *uhhOpt
);



















NSS_EXTERN NSSSymmetricKey *
NSSCryptoContext_FindSymmetricKeyByAlgorithmAndKeyID
(
  NSSCryptoContext *cc,
  NSSOID *algorithm,
  NSSItem *keyID,
  NSSCallback *uhhOpt
);





















NSS_EXTERN NSSItem *
NSSCryptoContext_Decrypt
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *encryptedData,
  NSSCallback *uhhOpt,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN PRStatus
NSSCryptoContext_BeginDecrypt
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *apOpt,
  NSSCallback *uhhOpt
);



























NSS_EXTERN NSSItem *
NSSCryptoContext_ContinueDecrypt
(
  NSSCryptoContext *cc,
  NSSItem *data,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSItem *
NSSCryptoContext_FinishDecrypt
(
  NSSCryptoContext *cc,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSItem *
NSSCryptoContext_Sign
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *data,
  NSSCallback *uhhOpt,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN PRStatus
NSSCryptoContext_BeginSign
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *apOpt,
  NSSCallback *uhhOpt
);






NSS_EXTERN PRStatus
NSSCryptoContext_ContinueSign
(
  NSSCryptoContext *cc,
  NSSItem *data
);






NSS_EXTERN NSSItem *
NSSCryptoContext_FinishSign
(
  NSSCryptoContext *cc,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSItem *
NSSCryptoContext_SignRecover
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *data,
  NSSCallback *uhhOpt,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN PRStatus
NSSCryptoContext_BeginSignRecover
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *apOpt,
  NSSCallback *uhhOpt
);






NSS_EXTERN NSSItem *
NSSCryptoContext_ContinueSignRecover
(
  NSSCryptoContext *cc,
  NSSItem *data,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSItem *
NSSCryptoContext_FinishSignRecover
(
  NSSCryptoContext *cc,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSSymmetricKey *
NSSCryptoContext_UnwrapSymmetricKey
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *wrappedKey,
  NSSCallback *uhhOpt
);






NSS_EXTERN NSSSymmetricKey *
NSSCryptoContext_DeriveSymmetricKey
(
  NSSCryptoContext *cc,
  NSSPublicKey *bk,
  NSSAlgorithmAndParameters *apOpt,
  NSSOID *target,
  PRUint32 keySizeOpt, 
  NSSOperations operations,
  NSSCallback *uhhOpt
);








NSS_EXTERN NSSItem *
NSSCryptoContext_Encrypt
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *data,
  NSSCallback *uhhOpt,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN PRStatus
NSSCryptoContext_BeginEncrypt
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *apOpt,
  NSSCallback *uhhOpt
);






NSS_EXTERN NSSItem *
NSSCryptoContext_ContinueEncrypt
(
  NSSCryptoContext *cc,
  NSSItem *data,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSItem *
NSSCryptoContext_FinishEncrypt
(
  NSSCryptoContext *cc,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN PRStatus
NSSCryptoContext_Verify
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *data,
  NSSItem *signature,
  NSSCallback *uhhOpt
);






NSS_EXTERN PRStatus
NSSCryptoContext_BeginVerify
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *signature,
  NSSCallback *uhhOpt
);






NSS_EXTERN PRStatus
NSSCryptoContext_ContinueVerify
(
  NSSCryptoContext *cc,
  NSSItem *data
);






NSS_EXTERN PRStatus
NSSCryptoContext_FinishVerify
(
  NSSCryptoContext *cc
);






NSS_EXTERN NSSItem *
NSSCryptoContext_VerifyRecover
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *signature,
  NSSCallback *uhhOpt,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN PRStatus
NSSCryptoContext_BeginVerifyRecover
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *apOpt,
  NSSCallback *uhhOpt
);






NSS_EXTERN NSSItem *
NSSCryptoContext_ContinueVerifyRecover
(
  NSSCryptoContext *cc,
  NSSItem *data,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSItem *
NSSCryptoContext_FinishVerifyRecover
(
  NSSCryptoContext *cc,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN NSSItem *
NSSCryptoContext_WrapSymmetricKey
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *apOpt,
  NSSSymmetricKey *keyToWrap,
  NSSCallback *uhhOpt,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);








NSS_EXTERN NSSItem *
NSSCryptoContext_Digest
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *data,
  NSSCallback *uhhOpt,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);






NSS_EXTERN PRStatus
NSSCryptoContext_BeginDigest
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *apOpt,
  NSSCallback *uhhOpt
);






NSS_EXTERN PRStatus
NSSCryptoContext_ContinueDigest
(
  NSSCryptoContext *cc,
  NSSAlgorithmAndParameters *apOpt,
  NSSItem *item
);






NSS_EXTERN NSSItem *
NSSCryptoContext_FinishDigest
(
  NSSCryptoContext *cc,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);










NSS_EXTERN NSSCryptoContext *
NSSCryptoContext_Clone
(
  NSSCryptoContext *cc
);









































PR_END_EXTERN_C

#endif 
