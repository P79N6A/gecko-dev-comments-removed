



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: bsession.c,v $ $Revision: 1.4 $ $Date: 2012/04/25 14:49:29 $";
#endif 

#include "builtins.h"








static NSSCKMDFindObjects *
builtins_mdSession_FindObjectsInit
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
  return nss_builtins_FindObjectsInit(fwSession, pTemplate, ulAttributeCount, pError);
}

NSS_IMPLEMENT NSSCKMDSession *
nss_builtins_CreateSession
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
  
  
  
  
  
  
  
  
  
  
  
  rv->FindObjectsInit = builtins_mdSession_FindObjectsInit;
  
  
  

  return rv;
}
