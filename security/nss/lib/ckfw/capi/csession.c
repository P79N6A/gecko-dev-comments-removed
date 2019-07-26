



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile$ $Revision$ $Date$";
#endif 

#include "ckcapi.h"








static NSSCKMDFindObjects *
ckcapi_mdSession_FindObjectsInit
(
  NSSCKMDSession *mdSession,
  NSSCKFWSession *fwSession,
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulAttributeCount,
  CK_RV *pError
)
{
  return nss_ckcapi_FindObjectsInit(fwSession, pTemplate, ulAttributeCount, pError);
}

static NSSCKMDObject *
ckcapi_mdSession_CreateObject
(
  NSSCKMDSession *mdSession,
  NSSCKFWSession *fwSession,
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  NSSArena        *arena,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulAttributeCount,
  CK_RV *pError
)
{
  return nss_ckcapi_CreateObject(fwSession, pTemplate, ulAttributeCount, pError);
}

NSS_IMPLEMENT NSSCKMDSession *
nss_ckcapi_CreateSession
(
  NSSCKFWSession *fwSession,
  CK_RV *pError
)
{
  NSSArena *arena;
  NSSCKMDSession *rv;

  arena = NSSCKFWSession_GetArena(fwSession, pError);
  if( (NSSArena *)NULL == arena ) {
    return (NSSCKMDSession *)NULL;
  }

  rv = nss_ZNEW(arena, NSSCKMDSession);
  if( (NSSCKMDSession *)NULL == rv ) {
    *pError = CKR_HOST_MEMORY;
    return (NSSCKMDSession *)NULL;
  }

  




  rv->etc = (void *)fwSession;
  
  
  
  
  
  
  
  
  
  rv->CreateObject = ckcapi_mdSession_CreateObject;
  
  rv->FindObjectsInit = ckcapi_mdSession_FindObjectsInit;
  
  
  

  return rv;
}
