



































#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: find.c,v $ $Revision: 1.9 $ $Date: 2009/02/09 07:55:52 $";
#endif 







#ifndef CK_H
#include "ck.h"
#endif 




















struct NSSCKFWFindObjectsStr {
  NSSCKFWMutex *mutex; 
  NSSCKMDFindObjects *mdfo1;
  NSSCKMDFindObjects *mdfo2;
  NSSCKFWSession *fwSession;
  NSSCKMDSession *mdSession;
  NSSCKFWToken *fwToken;
  NSSCKMDToken *mdToken;
  NSSCKFWInstance *fwInstance;
  NSSCKMDInstance *mdInstance;

  NSSCKMDFindObjects *mdFindObjects; 
};

#ifdef DEBUG











static CK_RV
findObjects_add_pointer
(
  const NSSCKFWFindObjects *fwFindObjects
)
{
  return CKR_OK;
}

static CK_RV
findObjects_remove_pointer
(
  const NSSCKFWFindObjects *fwFindObjects
)
{
  return CKR_OK;
}

NSS_IMPLEMENT CK_RV
nssCKFWFindObjects_verifyPointer
(
  const NSSCKFWFindObjects *fwFindObjects
)
{
  return CKR_OK;
}

#endif 





NSS_EXTERN NSSCKFWFindObjects *
nssCKFWFindObjects_Create
(
  NSSCKFWSession *fwSession,
  NSSCKFWToken *fwToken,
  NSSCKFWInstance *fwInstance,
  NSSCKMDFindObjects *mdFindObjects1,
  NSSCKMDFindObjects *mdFindObjects2,
  CK_RV *pError
)
{
  NSSCKFWFindObjects *fwFindObjects = NULL;
  NSSCKMDSession *mdSession;
  NSSCKMDToken *mdToken;
  NSSCKMDInstance *mdInstance;

  mdSession = nssCKFWSession_GetMDSession(fwSession);
  mdToken = nssCKFWToken_GetMDToken(fwToken);
  mdInstance = nssCKFWInstance_GetMDInstance(fwInstance);

  fwFindObjects = nss_ZNEW(NULL, NSSCKFWFindObjects);
  if (!fwFindObjects) {
    *pError = CKR_HOST_MEMORY;
    goto loser;
  }

  fwFindObjects->mdfo1 = mdFindObjects1;
  fwFindObjects->mdfo2 = mdFindObjects2;
  fwFindObjects->fwSession = fwSession;
  fwFindObjects->mdSession = mdSession;
  fwFindObjects->fwToken = fwToken;
  fwFindObjects->mdToken = mdToken;
  fwFindObjects->fwInstance = fwInstance;
  fwFindObjects->mdInstance = mdInstance;

  fwFindObjects->mutex = nssCKFWInstance_CreateMutex(fwInstance, NULL, pError);
  if (!fwFindObjects->mutex) {
    goto loser;
  }

#ifdef DEBUG
  *pError = findObjects_add_pointer(fwFindObjects);
  if( CKR_OK != *pError ) {
    goto loser;
  }
#endif 

  return fwFindObjects;

 loser:
  if( fwFindObjects ) {
    if( NULL != mdFindObjects1 ) {
      if( NULL != mdFindObjects1->Final ) {
        fwFindObjects->mdFindObjects = mdFindObjects1;
        mdFindObjects1->Final(mdFindObjects1, fwFindObjects, mdSession, 
          fwSession, mdToken, fwToken, mdInstance, fwInstance);
      }
    }

    if( NULL != mdFindObjects2 ) {
      if( NULL != mdFindObjects2->Final ) {
        fwFindObjects->mdFindObjects = mdFindObjects2;
        mdFindObjects2->Final(mdFindObjects2, fwFindObjects, mdSession, 
          fwSession, mdToken, fwToken, mdInstance, fwInstance);
      }
    }

    nss_ZFreeIf(fwFindObjects);
  }

  if( CKR_OK == *pError ) {
    *pError = CKR_GENERAL_ERROR;
  }

  return (NSSCKFWFindObjects *)NULL;
}






NSS_EXTERN void
nssCKFWFindObjects_Destroy
(
  NSSCKFWFindObjects *fwFindObjects
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWFindObjects_verifyPointer(fwFindObjects) ) {
    return;
  }
#endif 

  (void)nssCKFWMutex_Destroy(fwFindObjects->mutex);

  if (fwFindObjects->mdfo1) {
    if (fwFindObjects->mdfo1->Final) {
      fwFindObjects->mdFindObjects = fwFindObjects->mdfo1;
      fwFindObjects->mdfo1->Final(fwFindObjects->mdfo1, fwFindObjects,
        fwFindObjects->mdSession, fwFindObjects->fwSession, 
        fwFindObjects->mdToken, fwFindObjects->fwToken,
        fwFindObjects->mdInstance, fwFindObjects->fwInstance);
    }
  }

  if (fwFindObjects->mdfo2) {
    if (fwFindObjects->mdfo2->Final) {
      fwFindObjects->mdFindObjects = fwFindObjects->mdfo2;
      fwFindObjects->mdfo2->Final(fwFindObjects->mdfo2, fwFindObjects,
        fwFindObjects->mdSession, fwFindObjects->fwSession, 
        fwFindObjects->mdToken, fwFindObjects->fwToken,
        fwFindObjects->mdInstance, fwFindObjects->fwInstance);
    }
  }

  nss_ZFreeIf(fwFindObjects);

#ifdef DEBUG
  (void)findObjects_remove_pointer(fwFindObjects);
#endif 

  return;
}





NSS_EXTERN NSSCKMDFindObjects *
nssCKFWFindObjects_GetMDFindObjects
(
  NSSCKFWFindObjects *fwFindObjects
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWFindObjects_verifyPointer(fwFindObjects) ) {
    return (NSSCKMDFindObjects *)NULL;
  }
#endif 

  return fwFindObjects->mdFindObjects;
}





NSS_EXTERN NSSCKFWObject *
nssCKFWFindObjects_Next
(
  NSSCKFWFindObjects *fwFindObjects,
  NSSArena *arenaOpt,
  CK_RV *pError
)
{
  NSSCKMDObject *mdObject;
  NSSCKFWObject *fwObject = (NSSCKFWObject *)NULL;
  NSSArena *objArena;

#ifdef NSSDEBUG
  if (!pError) {
    return (NSSCKFWObject *)NULL;
  }

  *pError = nssCKFWFindObjects_verifyPointer(fwFindObjects);
  if( CKR_OK != *pError ) {
    return (NSSCKFWObject *)NULL;
  }
#endif 

  *pError = nssCKFWMutex_Lock(fwFindObjects->mutex);
  if( CKR_OK != *pError ) {
    return (NSSCKFWObject *)NULL;
  }

  if (fwFindObjects->mdfo1) {
    if (fwFindObjects->mdfo1->Next) {
      fwFindObjects->mdFindObjects = fwFindObjects->mdfo1;
      mdObject = fwFindObjects->mdfo1->Next(fwFindObjects->mdfo1,
        fwFindObjects, fwFindObjects->mdSession, fwFindObjects->fwSession,
        fwFindObjects->mdToken, fwFindObjects->fwToken, 
        fwFindObjects->mdInstance, fwFindObjects->fwInstance,
        arenaOpt, pError);
      if (!mdObject) {
        if( CKR_OK != *pError ) {
          goto done;
        }

        
        fwFindObjects->mdfo1->Final(fwFindObjects->mdfo1, fwFindObjects,
          fwFindObjects->mdSession, fwFindObjects->fwSession,
          fwFindObjects->mdToken, fwFindObjects->fwToken, 
          fwFindObjects->mdInstance, fwFindObjects->fwInstance);
        fwFindObjects->mdfo1 = (NSSCKMDFindObjects *)NULL;
      } else {
        goto wrap;
      }
    }
  }

  if (fwFindObjects->mdfo2) {
    if (fwFindObjects->mdfo2->Next) {
      fwFindObjects->mdFindObjects = fwFindObjects->mdfo2;
      mdObject = fwFindObjects->mdfo2->Next(fwFindObjects->mdfo2,
        fwFindObjects, fwFindObjects->mdSession, fwFindObjects->fwSession,
        fwFindObjects->mdToken, fwFindObjects->fwToken, 
        fwFindObjects->mdInstance, fwFindObjects->fwInstance,
        arenaOpt, pError);
      if (!mdObject) {
        if( CKR_OK != *pError ) {
          goto done;
        }

        
        fwFindObjects->mdfo2->Final(fwFindObjects->mdfo2, fwFindObjects,
          fwFindObjects->mdSession, fwFindObjects->fwSession,
          fwFindObjects->mdToken, fwFindObjects->fwToken, 
          fwFindObjects->mdInstance, fwFindObjects->fwInstance);
        fwFindObjects->mdfo2 = (NSSCKMDFindObjects *)NULL;
      } else {
        goto wrap;
      }
    }
  }
  
  
  *pError = CKR_OK;
  goto done;

 wrap:
  














  objArena = nssCKFWToken_GetArena(fwFindObjects->fwToken, pError);
  if (!objArena) {
    if( CKR_OK == *pError ) {
      *pError = CKR_HOST_MEMORY;
    }
    goto done;
  }

  fwObject = nssCKFWObject_Create(objArena, mdObject,
               NULL, fwFindObjects->fwToken, 
               fwFindObjects->fwInstance, pError);
  if (!fwObject) {
    if( CKR_OK == *pError ) {
      *pError = CKR_GENERAL_ERROR;
    }
  }

 done:
  (void)nssCKFWMutex_Unlock(fwFindObjects->mutex);
  return fwObject;
}






NSS_EXTERN NSSCKMDFindObjects *
NSSCKFWFindObjects_GetMDFindObjects
(
  NSSCKFWFindObjects *fwFindObjects
)
{
#ifdef DEBUG
  if( CKR_OK != nssCKFWFindObjects_verifyPointer(fwFindObjects) ) {
    return (NSSCKMDFindObjects *)NULL;
  }
#endif 

  return nssCKFWFindObjects_GetMDFindObjects(fwFindObjects);
}
