



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: slot.c,v $ $Revision: 1.4 $ $Date: 2012/04/25 14:49:30 $";
#endif 

#include "ckdbm.h"

static CK_RV
nss_dbm_mdSlot_Initialize
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,                                    
  NSSCKFWInstance *fwInstance
)
{
  nss_dbm_slot_t *slot = (nss_dbm_slot_t *)mdSlot->etc;
  nss_dbm_instance_t *instance = (nss_dbm_instance_t *)mdInstance->etc;
  CK_RV rv = CKR_OK;

  slot->token_db = nss_dbm_db_open(instance->arena, fwInstance, slot->filename, 
                                   slot->flags, &rv);
  if( (nss_dbm_db_t *)NULL == slot->token_db ) {
    if( CKR_TOKEN_NOT_PRESENT == rv ) {
      
      rv = CKR_OK;
    }
  }

  return rv;
}

static void
nss_dbm_mdSlot_Destroy
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,                                    
  NSSCKFWInstance *fwInstance
)
{
  nss_dbm_slot_t *slot = (nss_dbm_slot_t *)mdSlot->etc;

  if( (nss_dbm_db_t *)NULL != slot->token_db ) {
    nss_dbm_db_close(slot->token_db);
    slot->token_db = (nss_dbm_db_t *)NULL;
  }
}

static NSSUTF8 *
nss_dbm_mdSlot_GetSlotDescription
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,                                    
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return "Database";
}

static NSSUTF8 *
nss_dbm_mdSlot_GetManufacturerID
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,                                    
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return "Berkeley";
}

static CK_BBOOL
nss_dbm_mdSlot_GetTokenPresent
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,                                    
  NSSCKFWInstance *fwInstance
)
{
  nss_dbm_slot_t *slot = (nss_dbm_slot_t *)mdSlot->etc;

  if( (nss_dbm_db_t *)NULL == slot->token_db ) {
    return CK_FALSE;
  } else {
    return CK_TRUE;
  }
}

static CK_BBOOL
nss_dbm_mdSlot_GetRemovableDevice
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,                                    
  NSSCKFWInstance *fwInstance
)
{
  







  return CK_TRUE;
}












static NSSCKMDToken *
nss_dbm_mdSlot_GetToken
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,                                    
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  nss_dbm_slot_t *slot = (nss_dbm_slot_t *)mdSlot->etc;
  return nss_dbm_mdToken_factory(slot, pError);
}

NSS_IMPLEMENT NSSCKMDSlot *
nss_dbm_mdSlot_factory
(
  nss_dbm_instance_t *instance,
  char *filename,
  int flags,
  CK_RV *pError
)
{
  nss_dbm_slot_t *slot;
  NSSCKMDSlot *rv;

  slot = nss_ZNEW(instance->arena, nss_dbm_slot_t);
  if( (nss_dbm_slot_t *)NULL == slot ) {
    *pError = CKR_HOST_MEMORY;
    return (NSSCKMDSlot *)NULL;
  }

  slot->instance = instance;
  slot->filename = filename;
  slot->flags = flags;
  slot->token_db = (nss_dbm_db_t *)NULL;

  rv = nss_ZNEW(instance->arena, NSSCKMDSlot);
  if( (NSSCKMDSlot *)NULL == rv ) {
    *pError = CKR_HOST_MEMORY;
    return (NSSCKMDSlot *)NULL;
  }

  rv->etc = (void *)slot;
  rv->Initialize = nss_dbm_mdSlot_Initialize;
  rv->Destroy = nss_dbm_mdSlot_Destroy;
  rv->GetSlotDescription = nss_dbm_mdSlot_GetSlotDescription;
  rv->GetManufacturerID = nss_dbm_mdSlot_GetManufacturerID;
  rv->GetTokenPresent = nss_dbm_mdSlot_GetTokenPresent;
  rv->GetRemovableDevice = nss_dbm_mdSlot_GetRemovableDevice;
  
  
  
  rv->GetToken = nss_dbm_mdSlot_GetToken;
  rv->null = (void *)NULL;

  return rv;
}
