



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: msession.c,v $ $Revision: 1.2 $ $Date: 2012/04/25 14:49:40 $";
#endif 

#include "ckmk.h"








static NSSCKMDFindObjects *
ckmk_mdSession_FindObjectsInit
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
  return nss_ckmk_FindObjectsInit(fwSession, pTemplate, ulAttributeCount, pError);
}

static NSSCKMDObject *
ckmk_mdSession_CreateObject
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
  return nss_ckmk_CreateObject(fwSession, pTemplate, ulAttributeCount, pError);
}

NSS_IMPLEMENT NSSCKMDSession *
nss_ckmk_CreateSession
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
  
  
  
  
  
  
  
  
  
  rv->CreateObject = ckmk_mdSession_CreateObject;
  
  rv->FindObjectsInit = ckmk_mdSession_FindObjectsInit;
  
  
  

  return rv;
}
