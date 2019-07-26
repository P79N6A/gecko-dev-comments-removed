



#ifndef DEVM_H
#define DEVM_H

#ifdef DEBUG
static const char DEVM_CVS_ID[] = "@(#) $RCSfile: devm.h,v $ $Revision: 1.13 $ $Date: 2012/04/25 14:49:42 $";
#endif 

#ifndef BASE_H
#include "base.h"
#endif 

#ifndef DEV_H
#include "dev.h"
#endif 

#ifndef DEVTM_H
#include "devtm.h"
#endif 

PR_BEGIN_EXTERN_C


#define CKAPI(epv) \
    ((CK_FUNCTION_LIST_PTR)(epv))

NSS_EXTERN void
nssDevice_AddRef
(
 struct nssDeviceBaseStr *device
);

NSS_EXTERN PRBool
nssDevice_Destroy
(
 struct nssDeviceBaseStr *device
);

NSS_EXTERN PRBool
nssModule_IsThreadSafe
(
  NSSModule *module
);

NSS_EXTERN PRBool
nssModule_IsInternal
(
  NSSModule *mod
);

NSS_EXTERN PRBool
nssModule_IsModuleDBOnly
(
  NSSModule *mod
);

NSS_EXTERN void *
nssModule_GetCryptokiEPV
(
  NSSModule *mod
);

NSS_EXTERN NSSSlot *
nssSlot_Create
(
  CK_SLOT_ID slotId,
  NSSModule *parent
);

NSS_EXTERN void *
nssSlot_GetCryptokiEPV
(
  NSSSlot *slot
);

NSS_EXTERN NSSToken *
nssToken_Create
(
  CK_SLOT_ID slotID,
  NSSSlot *peer
);

NSS_EXTERN void *
nssToken_GetCryptokiEPV
(
  NSSToken *token
);

NSS_EXTERN nssSession *
nssToken_GetDefaultSession
(
  NSSToken *token
);

NSS_EXTERN PRBool
nssToken_IsLoginRequired
(
  NSSToken *token
);

NSS_EXTERN void
nssToken_Remove
(
  NSSToken *token
);

NSS_EXTERN nssCryptokiObject *
nssCryptokiObject_Create
(
  NSSToken *t, 
  nssSession *session,
  CK_OBJECT_HANDLE h
);

NSS_EXTERN nssTokenObjectCache *
nssTokenObjectCache_Create
(
  NSSToken *token,
  PRBool cacheCerts,
  PRBool cacheTrust,
  PRBool cacheCRLs
);

NSS_EXTERN void
nssTokenObjectCache_Destroy
(
  nssTokenObjectCache *cache
);

NSS_EXTERN void
nssTokenObjectCache_Clear
(
  nssTokenObjectCache *cache
);

NSS_EXTERN PRBool
nssTokenObjectCache_HaveObjectClass
(
  nssTokenObjectCache *cache,
  CK_OBJECT_CLASS objclass
);

NSS_EXTERN nssCryptokiObject **
nssTokenObjectCache_FindObjectsByTemplate
(
  nssTokenObjectCache *cache,
  CK_OBJECT_CLASS objclass,
  CK_ATTRIBUTE_PTR otemplate,
  CK_ULONG otlen,
  PRUint32 maximumOpt,
  PRStatus *statusOpt
);

NSS_EXTERN PRStatus
nssTokenObjectCache_GetObjectAttributes
(
  nssTokenObjectCache *cache,
  NSSArena *arenaOpt,
  nssCryptokiObject *object,
  CK_OBJECT_CLASS objclass,
  CK_ATTRIBUTE_PTR atemplate,
  CK_ULONG atlen
);

NSS_EXTERN PRStatus
nssTokenObjectCache_ImportObject
(
  nssTokenObjectCache *cache,
  nssCryptokiObject *object,
  CK_OBJECT_CLASS objclass,
  CK_ATTRIBUTE_PTR ot,
  CK_ULONG otlen
);

NSS_EXTERN void
nssTokenObjectCache_RemoveObject
(
  nssTokenObjectCache *cache,
  nssCryptokiObject *object
);


NSS_EXTERN PRStatus
nssToken_GetCachedObjectAttributes
(
  NSSToken *token,
  NSSArena *arenaOpt,
  nssCryptokiObject *object,
  CK_OBJECT_CLASS objclass,
  CK_ATTRIBUTE_PTR atemplate,
  CK_ULONG atlen
);




NSS_EXTERN PRUint32
nssPKCS11String_Length
(
  CK_CHAR *pkcs11str, 
  PRUint32 bufLen
);

PR_END_EXTERN_C

#endif 
