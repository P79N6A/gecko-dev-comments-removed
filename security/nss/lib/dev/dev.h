



































#ifndef DEV_H
#define DEV_H







#ifdef DEBUG
static const char DEV_CVS_ID[] = "@(#) $RCSfile: dev.h,v $ $Revision: 1.41 $ $Date: 2008/05/29 17:24:15 $";
#endif 

#ifndef NSSCKT_H
#include "nssckt.h"
#endif 

#ifndef NSSDEV_H
#include "nssdev.h"
#endif 

#ifndef DEVT_H
#include "devt.h"
#endif 

PR_BEGIN_EXTERN_C



















NSS_EXTERN PRStatus
nss_InitializeGlobalModuleList
(
  void
);

NSS_EXTERN PRStatus
nss_DestroyGlobalModuleList
(
  void
);

NSS_EXTERN NSSModule **
nss_GetLoadedModules
(
  void
);

NSS_EXTERN PRStatus
nssGlobalModuleList_Add
(
  NSSModule *module
);

NSS_EXTERN PRStatus
nssGlobalModuleList_Remove
(
  NSSModule *module
);

NSS_EXTERN NSSModule *
nssGlobalModuleList_FindModuleByName
(
  NSSUTF8 *moduleName
);

NSS_EXTERN NSSSlot *
nssGlobalModuleList_FindSlotByName
(
  NSSUTF8 *slotName
);

NSS_EXTERN NSSToken *
nssGlobalModuleList_FindTokenByName
(
  NSSUTF8 *tokenName
);

NSS_EXTERN NSSToken *
nss_GetDefaultCryptoToken
(
  void
);

NSS_EXTERN NSSToken *
nss_GetDefaultDatabaseToken
(
  void
);



















NSS_EXTERN NSSModule *
nssModule_Create
(
  NSSUTF8 *moduleOpt,
  NSSUTF8 *uriOpt,
  NSSUTF8 *opaqueOpt,
  void    *reserved
);


NSS_EXTERN NSSModule *
nssModule_CreateFromSpec
(
  NSSUTF8 *moduleSpec,
  NSSModule *parent,
  PRBool loadSubModules
);

NSS_EXTERN PRStatus
nssModule_Destroy
(
  NSSModule *mod
);

NSS_EXTERN NSSModule *
nssModule_AddRef
(
  NSSModule *mod
);

NSS_EXTERN NSSUTF8 *
nssModule_GetName
(
  NSSModule *mod
);

NSS_EXTERN NSSSlot **
nssModule_GetSlots
(
  NSSModule *mod
);

NSS_EXTERN NSSSlot *
nssModule_FindSlotByName
(
  NSSModule *mod,
  NSSUTF8 *slotName
);

NSS_EXTERN NSSToken *
nssModule_FindTokenByName
(
  NSSModule *mod,
  NSSUTF8 *tokenName
);

NSS_EXTERN PRInt32
nssModule_GetCertOrder
(
  NSSModule *module
);




















NSS_EXTERN PRStatus
nssSlot_Destroy
(
  NSSSlot *slot
);

NSS_EXTERN NSSSlot *
nssSlot_AddRef
(
  NSSSlot *slot
);

NSS_EXTERN void
nssSlot_ResetDelay
(
  NSSSlot *slot
);

NSS_EXTERN NSSUTF8 *
nssSlot_GetName
(
  NSSSlot *slot
);

NSS_EXTERN NSSUTF8 *
nssSlot_GetTokenName
(
  NSSSlot *slot
);

NSS_EXTERN NSSModule *
nssSlot_GetModule
(
  NSSSlot *slot
);

NSS_EXTERN NSSToken *
nssSlot_GetToken
(
  NSSSlot *slot
);

NSS_EXTERN PRBool
nssSlot_IsTokenPresent
(
  NSSSlot *slot
);

NSS_EXTERN PRBool
nssSlot_IsPermanent
(
  NSSSlot *slot
);

NSS_EXTERN PRBool
nssSlot_IsFriendly
(
  NSSSlot *slot
);

NSS_EXTERN PRBool
nssSlot_IsHardware
(
  NSSSlot *slot
);

NSS_EXTERN PRBool
nssSlot_IsLoggedIn
(
  NSSSlot *slot
);

NSS_EXTERN PRStatus
nssSlot_Refresh
(
  NSSSlot *slot
);

NSS_EXTERN PRStatus
nssSlot_Login
(
  NSSSlot *slot,
  NSSCallback *pwcb
);
extern const NSSError NSS_ERROR_INVALID_PASSWORD;
extern const NSSError NSS_ERROR_USER_CANCELED;

NSS_EXTERN PRStatus
nssSlot_Logout
(
  NSSSlot *slot,
  nssSession *sessionOpt
);

NSS_EXTERN void
nssSlot_EnterMonitor
(
  NSSSlot *slot
);

NSS_EXTERN void
nssSlot_ExitMonitor
(
  NSSSlot *slot
);

#define NSSSLOT_ASK_PASSWORD_FIRST_TIME -1
#define NSSSLOT_ASK_PASSWORD_EVERY_TIME  0
NSS_EXTERN void
nssSlot_SetPasswordDefaults
(
  NSSSlot *slot,
  PRInt32 askPasswordTimeout
);

NSS_EXTERN PRStatus
nssSlot_SetPassword
(
  NSSSlot *slot,
  NSSUTF8 *oldPasswordOpt,
  NSSUTF8 *newPassword
);
extern const NSSError NSS_ERROR_INVALID_PASSWORD;
extern const NSSError NSS_ERROR_USER_CANCELED;





NSS_EXTERN nssSession *
nssSlot_CreateSession
(
  NSSSlot *slot,
  NSSArena *arenaOpt,
  PRBool readWrite 
);































NSS_EXTERN PRStatus
nssToken_Destroy
(
  NSSToken *tok
);

NSS_EXTERN NSSToken *
nssToken_AddRef
(
  NSSToken *tok
);

NSS_EXTERN NSSUTF8 *
nssToken_GetName
(
  NSSToken *tok
);

NSS_EXTERN NSSModule *
nssToken_GetModule
(
  NSSToken *token
);

NSS_EXTERN NSSSlot *
nssToken_GetSlot
(
  NSSToken *tok
);

NSS_EXTERN PRBool
nssToken_NeedsPINInitialization
(
  NSSToken *token
);

NSS_EXTERN nssCryptokiObject *
nssToken_ImportCertificate
(
  NSSToken *tok,
  nssSession *sessionOpt,
  NSSCertificateType certType,
  NSSItem *id,
  const NSSUTF8 *nickname,
  NSSDER *encoding,
  NSSDER *issuer,
  NSSDER *subject,
  NSSDER *serial,
  NSSASCII7 *emailAddr,
  PRBool asTokenObject
);

NSS_EXTERN nssCryptokiObject *
nssToken_ImportTrust
(
  NSSToken *tok,
  nssSession *sessionOpt,
  NSSDER *certEncoding,
  NSSDER *certIssuer,
  NSSDER *certSerial,
  nssTrustLevel serverAuth,
  nssTrustLevel clientAuth,
  nssTrustLevel codeSigning,
  nssTrustLevel emailProtection,
  PRBool stepUpApproved,
  PRBool asTokenObject
);

NSS_EXTERN nssCryptokiObject *
nssToken_ImportCRL
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSDER *subject,
  NSSDER *encoding,
  PRBool isKRL,
  NSSUTF8 *url,
  PRBool asTokenObject
);


NSS_EXTERN PRStatus
nssToken_DeleteStoredObject
(
  nssCryptokiObject *instance
);

NSS_EXTERN nssCryptokiObject **
nssToken_FindObjects
(
  NSSToken *token,
  nssSession *sessionOpt,
  CK_OBJECT_CLASS objclass,
  nssTokenSearchType searchType,
  PRUint32 maximumOpt,
  PRStatus *statusOpt
);

NSS_EXTERN nssCryptokiObject **
nssToken_FindCertificatesBySubject
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSDER *subject,
  nssTokenSearchType searchType,
  PRUint32 maximumOpt,
  PRStatus *statusOpt
);

NSS_EXTERN nssCryptokiObject **
nssToken_FindCertificatesByNickname
(
  NSSToken *token,
  nssSession *sessionOpt,
  const NSSUTF8 *name,
  nssTokenSearchType searchType,
  PRUint32 maximumOpt,
  PRStatus *statusOpt
);

NSS_EXTERN nssCryptokiObject **
nssToken_FindCertificatesByEmail
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSASCII7 *email,
  nssTokenSearchType searchType,
  PRUint32 maximumOpt,
  PRStatus *statusOpt
);

NSS_EXTERN nssCryptokiObject **
nssToken_FindCertificatesByID
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSItem *id,
  nssTokenSearchType searchType,
  PRUint32 maximumOpt,
  PRStatus *statusOpt
);

NSS_EXTERN nssCryptokiObject *
nssToken_FindCertificateByIssuerAndSerialNumber
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSDER *issuer,
  NSSDER *serial,
  nssTokenSearchType searchType,
  PRStatus *statusOpt
);

NSS_EXTERN nssCryptokiObject *
nssToken_FindCertificateByEncodedCertificate
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSBER *encodedCertificate,
  nssTokenSearchType searchType,
  PRStatus *statusOpt
);

NSS_EXTERN nssCryptokiObject *
nssToken_FindTrustForCertificate
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSDER *certEncoding,
  NSSDER *certIssuer,
  NSSDER *certSerial,
  nssTokenSearchType searchType
);

NSS_EXTERN nssCryptokiObject **
nssToken_FindCRLsBySubject
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSDER *subject,
  nssTokenSearchType searchType,
  PRUint32 maximumOpt,
  PRStatus *statusOpt
);

NSS_EXTERN nssCryptokiObject **
nssToken_FindPrivateKeys
(
  NSSToken *token,
  nssSession *sessionOpt,
  nssTokenSearchType searchType,
  PRUint32 maximumOpt,
  PRStatus *statusOpt
);

NSS_EXTERN nssCryptokiObject *
nssToken_FindPrivateKeyByID
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSItem *keyID
);

NSS_EXTERN nssCryptokiObject *
nssToken_FindPublicKeyByID
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSItem *keyID
);

NSS_EXTERN NSSItem *
nssToken_Digest
(
  NSSToken *tok,
  nssSession *sessionOpt,
  NSSAlgorithmAndParameters *ap,
  NSSItem *data,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);

NSS_EXTERN PRStatus
nssToken_BeginDigest
(
  NSSToken *tok,
  nssSession *sessionOpt,
  NSSAlgorithmAndParameters *ap
);

NSS_EXTERN PRStatus
nssToken_ContinueDigest
(
  NSSToken *tok,
  nssSession *sessionOpt,
  NSSItem *item
);

NSS_EXTERN NSSItem *
nssToken_FinishDigest
(
  NSSToken *tok,
  nssSession *sessionOpt,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
);









NSS_EXTERN PRStatus
nssSession_Destroy
(
  nssSession *s
);


NSS_EXTERN PRStatus
nssSession_EnterMonitor
(
  nssSession *s
);


NSS_EXTERN PRStatus
nssSession_ExitMonitor
(
  nssSession *s
);


NSS_EXTERN PRBool
nssSession_IsReadWrite
(
  nssSession *s
);

















NSS_EXTERN void
nssCryptokiObject_Destroy
(
  nssCryptokiObject *object
);

NSS_EXTERN PRBool
nssCryptokiObject_Equal
(
  nssCryptokiObject *object1,
  nssCryptokiObject *object2
);

NSS_EXTERN nssCryptokiObject *
nssCryptokiObject_Clone
(
  nssCryptokiObject *object
);

NSS_EXTERN PRStatus
nssCryptokiCertificate_GetAttributes
(
  nssCryptokiObject *object,
  nssSession *sessionOpt,
  NSSArena *arenaOpt,
  NSSCertificateType *certTypeOpt,
  NSSItem *idOpt,
  NSSDER *encodingOpt,
  NSSDER *issuerOpt,
  NSSDER *serialOpt,
  NSSDER *subjectOpt
);

NSS_EXTERN PRStatus
nssCryptokiTrust_GetAttributes
(
  nssCryptokiObject *trustObject,
  nssSession *sessionOpt,
  NSSItem *sha1_hash,
  nssTrustLevel *serverAuth,
  nssTrustLevel *clientAuth,
  nssTrustLevel *codeSigning,
  nssTrustLevel *emailProtection,
  PRBool *stepUpApproved
);

NSS_EXTERN PRStatus
nssCryptokiCRL_GetAttributes
(
  nssCryptokiObject *crlObject,
  nssSession *sessionOpt,
  NSSArena *arenaOpt,
  NSSItem *encodingOpt,
  NSSItem * subjectOpt,
  CK_ULONG * crl_class,
  NSSUTF8 **urlOpt,
  PRBool *isKRLOpt
);





NSS_EXTERN PRStatus
nssCryptokiPrivateKey_SetCertificate
(
  nssCryptokiObject *keyObject,
  nssSession *sessionOpt,
  const NSSUTF8 *nickname,
  NSSItem *id,
  NSSDER *subject
);

NSS_EXTERN void
nssModuleArray_Destroy
(
  NSSModule **modules
);






NSS_EXTERN void
nssSlotArray_Destroy
(
  NSSSlot **slots
);






NSS_EXTERN void
nssTokenArray_Destroy
(
  NSSToken **tokens
);





NSS_EXTERN void
nssCryptokiObjectArray_Destroy
(
  nssCryptokiObject **object
);




















NSS_EXTERN nssSlotList *
nssSlotList_Create
(
  NSSArena *arenaOpt
);



NSS_EXTERN void
nssSlotList_Destroy
(
  nssSlotList *slotList
);





NSS_EXTERN PRStatus
nssSlotList_Add
(
  nssSlotList *slotList,
  NSSSlot *slot,
  PRUint32 order
);






NSS_EXTERN PRStatus
nssSlotList_AddModuleSlots
(
  nssSlotList *slotList,
  NSSModule *module,
  PRUint32 order
);



NSS_EXTERN NSSSlot **
nssSlotList_GetSlots
(
  nssSlotList *slotList
);



NSS_EXTERN NSSSlot *
nssSlotList_FindSlotByName
(
  nssSlotList *slotList,
  NSSUTF8 *slotName
);



NSS_EXTERN NSSToken *
nssSlotList_FindTokenByName
(
  nssSlotList *slotList,
  NSSUTF8 *tokenName
);






NSS_EXTERN NSSSlot *
nssSlotList_GetBestSlot
(
  nssSlotList *slotList
);





NSS_EXTERN NSSSlot *
nssSlotList_GetBestSlotForAlgorithmAndParameters
(
  nssSlotList *slotList,
  NSSAlgorithmAndParameters *ap
);





NSS_EXTERN NSSSlot *
nssSlotList_GetBestSlotForAlgorithmsAndParameters
(
  nssSlotList *slotList,
  NSSAlgorithmAndParameters **ap
);

NSS_EXTERN PRBool
nssToken_IsPresent
(
  NSSToken *token
);

NSS_EXTERN nssSession *
nssToken_GetDefaultSession
(
  NSSToken *token
);

NSS_EXTERN PRStatus
nssToken_GetTrustOrder
(
  NSSToken *tok
);

NSS_EXTERN PRStatus
nssToken_NotifyCertsNotVisible
(
  NSSToken *tok
);

NSS_EXTERN PRStatus
nssToken_TraverseCertificates
(
  NSSToken *token,
  nssSession *sessionOpt,
  nssTokenSearchType searchType,
  PRStatus (* callback)(nssCryptokiObject *instance, void *arg),
  void *arg
);

NSS_EXTERN PRBool
nssToken_IsPrivateKeyAvailable
(
  NSSToken *token,
  NSSCertificate *c,
  nssCryptokiObject *instance
);

PR_END_EXTERN_C

#endif 
