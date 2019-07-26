



#ifndef PKIM_H
#define PKIM_H

#ifdef DEBUG
static const char PKIM_CVS_ID[] = "@(#) $RCSfile: pkim.h,v $ $Revision: 1.31 $ $Date: 2012/04/25 14:50:07 $";
#endif 

#ifndef BASE_H
#include "base.h"
#endif 

#ifndef PKI_H
#include "pki.h"
#endif 

#ifndef PKITM_H
#include "pkitm.h"
#endif 

PR_BEGIN_EXTERN_C


















NSS_EXTERN void     nssPKIObject_Lock       (nssPKIObject * object);
NSS_EXTERN void     nssPKIObject_Unlock     (nssPKIObject * object);
NSS_EXTERN PRStatus nssPKIObject_NewLock    (nssPKIObject * object,
                                             nssPKILockType lockType);
NSS_EXTERN void     nssPKIObject_DestroyLock(nssPKIObject * object);






NSS_EXTERN nssPKIObject *
nssPKIObject_Create
(
  NSSArena *arenaOpt,
  nssCryptokiObject *instanceOpt,
  NSSTrustDomain *td,
  NSSCryptoContext *ccOpt,
  nssPKILockType lockType
);



NSS_EXTERN nssPKIObject *
nssPKIObject_AddRef
(
  nssPKIObject *object
);






NSS_EXTERN PRBool
nssPKIObject_Destroy
(
  nssPKIObject *object
);





NSS_EXTERN PRStatus
nssPKIObject_AddInstance
(
  nssPKIObject *object,
  nssCryptokiObject *instance
);





NSS_EXTERN PRBool
nssPKIObject_HasInstance
(
  nssPKIObject *object,
  nssCryptokiObject *instance
);





NSS_EXTERN NSSToken **
nssPKIObject_GetTokens
(
  nssPKIObject *object,
  PRStatus *statusOpt
);






NSS_EXTERN NSSUTF8 *
nssPKIObject_GetNicknameForToken
(
  nssPKIObject *object,
  NSSToken *tokenOpt
);





NSS_EXTERN PRStatus
nssPKIObject_RemoveInstanceForToken
(
  nssPKIObject *object,
  NSSToken *token
);











NSS_EXTERN PRStatus
nssPKIObject_DeleteStoredObject
(
  nssPKIObject *object,
  NSSCallback *uhh,
  PRBool isFriendly
);

NSS_EXTERN nssCryptokiObject **
nssPKIObject_GetInstances
(
  nssPKIObject *object
);

NSS_EXTERN NSSCertificate **
nssTrustDomain_FindCertificatesByID
(
  NSSTrustDomain *td,
  NSSItem *id,
  NSSCertificate **rvOpt,
  PRUint32 maximumOpt,
  NSSArena *arenaOpt
);

NSS_EXTERN NSSCRL **
nssTrustDomain_FindCRLsBySubject
(
  NSSTrustDomain *td,
  NSSDER *subject
);



NSS_EXTERN NSSCryptoContext *
nssCryptoContext_Create
(
  NSSTrustDomain *td,
  NSSCallback *uhhOpt
);


NSS_EXTERN NSSCertificate *
nssCertificate_Create
(
  nssPKIObject *object
);

NSS_EXTERN PRStatus
nssCertificate_SetCertTrust
(
  NSSCertificate *c,
  NSSTrust *trust
);

NSS_EXTERN nssDecodedCert *
nssCertificate_GetDecoding
(
  NSSCertificate *c
);

extern PRIntn
nssCertificate_SubjectListSort
(
  void *v1,
  void *v2
);

NSS_EXTERN nssDecodedCert *
nssDecodedCert_Create
(
  NSSArena *arenaOpt,
  NSSDER *encoding,
  NSSCertificateType type
);

NSS_EXTERN PRStatus
nssDecodedCert_Destroy
(
  nssDecodedCert *dc
);

NSS_EXTERN NSSTrust *
nssTrust_Create
(
  nssPKIObject *object,
  NSSItem *certData
);

NSS_EXTERN NSSCRL *
nssCRL_Create
(
  nssPKIObject *object
);

NSS_EXTERN NSSCRL *
nssCRL_AddRef
(
  NSSCRL *crl
);

NSS_EXTERN PRStatus
nssCRL_Destroy
(
  NSSCRL *crl
);

NSS_EXTERN PRStatus
nssCRL_DeleteStoredObject
(
  NSSCRL *crl,
  NSSCallback *uhh
);

NSS_EXTERN NSSPrivateKey *
nssPrivateKey_Create
(
  nssPKIObject *o
);

NSS_EXTERN NSSDER *
nssCRL_GetEncoding
(
  NSSCRL *crl
);

NSS_EXTERN NSSPublicKey *
nssPublicKey_Create
(
  nssPKIObject *object
);


















NSS_EXTERN void
nssCertificateArray_Destroy
(
  NSSCertificate **certs
);









NSS_EXTERN NSSCertificate **
nssCertificateArray_Join
(
  NSSCertificate **certs1,
  NSSCertificate **certs2
);






NSS_EXTERN NSSCertificate * 
nssCertificateArray_FindBestCertificate
(
  NSSCertificate **certs, 
  NSSTime *timeOpt,
  const NSSUsage *usage,
  NSSPolicies *policiesOpt
);






NSS_EXTERN PRStatus
nssCertificateArray_Traverse
(
  NSSCertificate **certs,
  PRStatus (* callback)(NSSCertificate *c, void *arg),
  void *arg
);

NSS_EXTERN void
nssCRLArray_Destroy
(
  NSSCRL **crls
);







































NSS_EXTERN nssPKIObjectCollection *
nssCertificateCollection_Create
(
  NSSTrustDomain *td,
  NSSCertificate **certsOpt
);






NSS_EXTERN nssPKIObjectCollection *
nssCRLCollection_Create
(
  NSSTrustDomain *td,
  NSSCRL **crlsOpt
);






NSS_EXTERN nssPKIObjectCollection *
nssPrivateKeyCollection_Create
(
  NSSTrustDomain *td,
  NSSPrivateKey **pvkOpt
);






NSS_EXTERN nssPKIObjectCollection *
nssPublicKeyCollection_Create
(
  NSSTrustDomain *td,
  NSSPublicKey **pvkOpt
);



NSS_EXTERN void
nssPKIObjectCollection_Destroy
(
  nssPKIObjectCollection *collection
);



NSS_EXTERN PRUint32
nssPKIObjectCollection_Count
(
  nssPKIObjectCollection *collection
);

NSS_EXTERN PRStatus
nssPKIObjectCollection_AddObject
(
  nssPKIObjectCollection *collection,
  nssPKIObject *object
);












NSS_EXTERN PRStatus
nssPKIObjectCollection_AddInstances
(
  nssPKIObjectCollection *collection,
  nssCryptokiObject **instances,
  PRUint32 numInstances
);



NSS_EXTERN PRStatus
nssPKIObjectCollection_Traverse
(
  nssPKIObjectCollection *collection,
  nssPKIObjectCallback *callback
);






NSS_EXTERN PRStatus
nssPKIObjectCollection_AddInstanceAsObject
(
  nssPKIObjectCollection *collection,
  nssCryptokiObject *instance
);





NSS_EXTERN NSSCertificate **
nssPKIObjectCollection_GetCertificates
(
  nssPKIObjectCollection *collection,
  NSSCertificate **rvOpt,
  PRUint32 maximumOpt,
  NSSArena *arenaOpt
);

NSS_EXTERN NSSCRL **
nssPKIObjectCollection_GetCRLs
(
  nssPKIObjectCollection *collection,
  NSSCRL **rvOpt,
  PRUint32 maximumOpt,
  NSSArena *arenaOpt
);

NSS_EXTERN NSSPrivateKey **
nssPKIObjectCollection_GetPrivateKeys
(
  nssPKIObjectCollection *collection,
  NSSPrivateKey **rvOpt,
  PRUint32 maximumOpt,
  NSSArena *arenaOpt
);

NSS_EXTERN NSSPublicKey **
nssPKIObjectCollection_GetPublicKeys
(
  nssPKIObjectCollection *collection,
  NSSPublicKey **rvOpt,
  PRUint32 maximumOpt,
  NSSArena *arenaOpt
);

NSS_EXTERN NSSTime *
NSSTime_Now
(
  NSSTime *timeOpt
);

NSS_EXTERN NSSTime *
NSSTime_SetPRTime
(
  NSSTime *timeOpt,
  PRTime prTime
);

NSS_EXTERN PRTime
NSSTime_GetPRTime
(
  NSSTime *time
);

NSS_EXTERN nssHash *
nssHash_CreateCertificate
(
  NSSArena *arenaOpt,
  PRUint32 numBuckets
);



NSS_EXTERN PRStatus
nssTrustDomain_InitializeCache
(
  NSSTrustDomain *td,
  PRUint32 cacheSize
);

NSS_EXTERN PRStatus
nssTrustDomain_AddCertsToCache
(
  NSSTrustDomain *td,
  NSSCertificate **certs,
  PRUint32 numCerts
);

NSS_EXTERN void
nssTrustDomain_RemoveCertFromCacheLOCKED (
  NSSTrustDomain *td,
  NSSCertificate *cert
);

NSS_EXTERN void
nssTrustDomain_LockCertCache (
  NSSTrustDomain *td
);

NSS_EXTERN void
nssTrustDomain_UnlockCertCache (
  NSSTrustDomain *td
);

NSS_IMPLEMENT PRStatus
nssTrustDomain_DestroyCache
(
  NSSTrustDomain *td
);





NSS_EXTERN PRStatus
nssTrustDomain_RemoveTokenCertsFromCache
(
  NSSTrustDomain *td,
  NSSToken *token
);

NSS_EXTERN PRStatus
nssTrustDomain_UpdateCachedTokenCerts
(
  NSSTrustDomain *td,
  NSSToken *token
);




NSS_EXTERN NSSCertificate **
nssTrustDomain_GetCertsForNicknameFromCache
(
  NSSTrustDomain *td,
  const NSSUTF8 *nickname,
  nssList *certListOpt
);




NSS_EXTERN NSSCertificate **
nssTrustDomain_GetCertsForEmailAddressFromCache
(
  NSSTrustDomain *td,
  NSSASCII7 *email,
  nssList *certListOpt
);




NSS_EXTERN NSSCertificate **
nssTrustDomain_GetCertsForSubjectFromCache
(
  NSSTrustDomain *td,
  NSSDER *subject,
  nssList *certListOpt
);




NSS_EXTERN NSSCertificate *
nssTrustDomain_GetCertForIssuerAndSNFromCache
(
  NSSTrustDomain *td,
  NSSDER *issuer,
  NSSDER *serialNum
);




NSS_EXTERN NSSCertificate *
nssTrustDomain_GetCertByDERFromCache
(
  NSSTrustDomain *td,
  NSSDER *der
);





NSS_EXTERN NSSCertificate **
nssTrustDomain_GetCertsFromCache
(
  NSSTrustDomain *td,
  nssList *certListOpt
);

NSS_EXTERN void
nssTrustDomain_DumpCacheInfo
(
  NSSTrustDomain *td,
  void (* cert_dump_iter)(const void *, void *, void *),
  void *arg
);

NSS_EXTERN void
nssCertificateList_AddReferences
(
  nssList *certList
);

PR_END_EXTERN_C

#endif 
