



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: instance.c,v $ $Revision: 1.5 $ $Date: 2012/04/25 14:49:30 $";
#endif 

#include "ckdbm.h"

static CK_RV
nss_dbm_mdInstance_Initialize
(
  NSSCKMDInstance *mdInstance,                                    
  NSSCKFWInstance *fwInstance,
  NSSUTF8 *configurationData
)
{
  CK_RV rv = CKR_OK;
  NSSArena *arena;
  nss_dbm_instance_t *instance;

  arena = NSSCKFWInstance_GetArena(fwInstance, &rv);
  if( ((NSSArena *)NULL == arena) && (CKR_OK != rv) ) {
    return rv;
  }

  instance = nss_ZNEW(arena, nss_dbm_instance_t);
  if( (nss_dbm_instance_t *)NULL == instance ) {
    return CKR_HOST_MEMORY;
  }

  instance->arena = arena;

  






  instance->nSlots = 1;
  instance->filenames = nss_ZNEWARRAY(arena, char *, instance->nSlots);
  if( (char **)NULL == instance->filenames ) {
    return CKR_HOST_MEMORY;
  }

  instance->flags = nss_ZNEWARRAY(arena, int, instance->nSlots);
  if( (int *)NULL == instance->flags ) {
    return CKR_HOST_MEMORY;
  }

  instance->filenames[0] = "cert8.db";
  instance->flags[0] = O_RDWR|O_CREAT;

  mdInstance->etc = (void *)instance;
  return CKR_OK;
}



static CK_ULONG
nss_dbm_mdInstance_GetNSlots
(
  NSSCKMDInstance *mdInstance,                                    
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  nss_dbm_instance_t *instance = (nss_dbm_instance_t *)mdInstance->etc;
  return instance->nSlots;
}

static CK_VERSION
nss_dbm_mdInstance_GetCryptokiVersion
(
  NSSCKMDInstance *mdInstance,                                    
  NSSCKFWInstance *fwInstance
)
{
  static CK_VERSION rv = { 2, 1 };
  return rv;
}

static NSSUTF8 *
nss_dbm_mdInstance_GetManufacturerID
(
  NSSCKMDInstance *mdInstance,                                    
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return "Mozilla Foundation";
}

static NSSUTF8 *
nss_dbm_mdInstance_GetLibraryDescription
(
  NSSCKMDInstance *mdInstance,                                    
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return "Berkeley Database Module";
}

static CK_VERSION
nss_dbm_mdInstance_GetLibraryVersion
(
  NSSCKMDInstance *mdInstance,                                    
  NSSCKFWInstance *fwInstance
)
{
  static CK_VERSION rv = { 1, 0 }; 
  return rv;
}

static CK_BBOOL
nss_dbm_mdInstance_ModuleHandlesSessionObjects
(
  NSSCKMDInstance *mdInstance,                                    
  NSSCKFWInstance *fwInstance
)
{
  return CK_TRUE;
}

static CK_RV
nss_dbm_mdInstance_GetSlots
(
  NSSCKMDInstance *mdInstance,                                    
  NSSCKFWInstance *fwInstance,
  NSSCKMDSlot *slots[]
)
{
  nss_dbm_instance_t *instance = (nss_dbm_instance_t *)mdInstance->etc;
  CK_ULONG i;
  CK_RV rv = CKR_OK;

  for( i = 0; i < instance->nSlots; i++ ) {
    slots[i] = nss_dbm_mdSlot_factory(instance, instance->filenames[i], 
                                      instance->flags[i], &rv);
    if( (NSSCKMDSlot *)NULL == slots[i] ) {
      return rv;
    }
  }

  return rv;
}



NSS_IMPLEMENT_DATA NSSCKMDInstance 
nss_dbm_mdInstance = {
  NULL, 
  nss_dbm_mdInstance_Initialize,
  NULL, 
  nss_dbm_mdInstance_GetNSlots,
  nss_dbm_mdInstance_GetCryptokiVersion,
  nss_dbm_mdInstance_GetManufacturerID,
  nss_dbm_mdInstance_GetLibraryDescription,
  nss_dbm_mdInstance_GetLibraryVersion,
  nss_dbm_mdInstance_ModuleHandlesSessionObjects,
  nss_dbm_mdInstance_GetSlots,
  NULL, 
  NULL 
};
