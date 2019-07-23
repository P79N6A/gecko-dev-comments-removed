



































#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: slot.c,v $ $Revision: 1.6 $ $Date: 2005/01/20 02:25:45 $";
#endif 







#ifndef CK_T
#include "ck.h"
#endif 


































struct NSSCKFWSlotStr {
  NSSCKFWMutex *mutex;
  NSSCKMDSlot *mdSlot;
  NSSCKFWInstance *fwInstance;
  NSSCKMDInstance *mdInstance;
  CK_SLOT_ID slotID;

  

















  NSSUTF8 *slotDescription;
  NSSUTF8 *manufacturerID;
  CK_VERSION hardwareVersion;
  CK_VERSION firmwareVersion;
  NSSCKFWToken *fwToken;
};

#ifdef DEBUG











static CK_RV
slot_add_pointer
(
  const NSSCKFWSlot *fwSlot
)
{
  return CKR_OK;
}

static CK_RV
slot_remove_pointer
(
  const NSSCKFWSlot *fwSlot
)
{
  return CKR_OK;
}

NSS_IMPLEMENT CK_RV
nssCKFWSlot_verifyPointer
(
  const NSSCKFWSlot *fwSlot
)
{
  return CKR_OK;
}

#endif 





NSS_IMPLEMENT NSSCKFWSlot *
nssCKFWSlot_Create
(
  NSSCKFWInstance *fwInstance,
  NSSCKMDSlot *mdSlot,
  CK_SLOT_ID slotID,
  CK_RV *pError
)
{
  NSSCKFWSlot *fwSlot;
  NSSCKMDInstance *mdInstance;
  NSSArena *arena;

#ifdef NSSDEBUG
  if( (CK_RV *)NULL == pError ) {
    return (NSSCKFWSlot *)NULL;
  }

  *pError = nssCKFWInstance_verifyPointer(fwInstance);
  if( CKR_OK != *pError ) {
    return (NSSCKFWSlot *)NULL;
  }
#endif 

  mdInstance = nssCKFWInstance_GetMDInstance(fwInstance);
  if( (NSSCKMDInstance *)NULL == mdInstance ) {
    *pError = CKR_GENERAL_ERROR;
    return (NSSCKFWSlot *)NULL;
  }

  arena = nssCKFWInstance_GetArena(fwInstance, pError);
  if( (NSSArena *)NULL == arena ) {
    if( CKR_OK == *pError ) {
      *pError = CKR_GENERAL_ERROR;
    }
  }

  fwSlot = nss_ZNEW(arena, NSSCKFWSlot);
  if( (NSSCKFWSlot *)NULL == fwSlot ) {
    *pError = CKR_HOST_MEMORY;
    return (NSSCKFWSlot *)NULL;
  }

  fwSlot->mdSlot = mdSlot;
  fwSlot->fwInstance = fwInstance;
  fwSlot->mdInstance = mdInstance;
  fwSlot->slotID = slotID;

  fwSlot->mutex = nssCKFWInstance_CreateMutex(fwInstance, arena, pError);
  if( (NSSCKFWMutex *)NULL == fwSlot->mutex ) {
    if( CKR_OK == *pError ) {
      *pError = CKR_GENERAL_ERROR;
    }
    (void)nss_ZFreeIf(fwSlot);
    return (NSSCKFWSlot *)NULL;
  }

  if( (void *)NULL != (void *)mdSlot->Initialize ) {
    *pError = CKR_OK;
    *pError = mdSlot->Initialize(mdSlot, fwSlot, mdInstance, fwInstance);
    if( CKR_OK != *pError ) {
      (void)nssCKFWMutex_Destroy(fwSlot->mutex);
      (void)nss_ZFreeIf(fwSlot);
      return (NSSCKFWSlot *)NULL;
    }
  }

#ifdef DEBUG
  *pError = slot_add_pointer(fwSlot);
  if( CKR_OK != *pError ) {
    if( (void *)NULL != (void *)mdSlot->Destroy ) {
      mdSlot->Destroy(mdSlot, fwSlot, mdInstance, fwInstance);
    }

    (void)nssCKFWMutex_Destroy(fwSlot->mutex);
    (void)nss_ZFreeIf(fwSlot);
    return (NSSCKFWSlot *)NULL;
  }
#endif 

  return fwSlot;
}





NSS_IMPLEMENT CK_RV
nssCKFWSlot_Destroy
(
  NSSCKFWSlot *fwSlot
)
{
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  error = nssCKFWSlot_verifyPointer(fwSlot);
  if( CKR_OK != error ) {
    return error;
  }
#endif 
  if (fwSlot->fwToken) {
    nssCKFWToken_Destroy(fwSlot->fwToken);
  }

  (void)nssCKFWMutex_Destroy(fwSlot->mutex);

  if( (void *)NULL != (void *)fwSlot->mdSlot->Destroy ) {
    fwSlot->mdSlot->Destroy(fwSlot->mdSlot, fwSlot, 
      fwSlot->mdInstance, fwSlot->fwInstance);
  }

#ifdef DEBUG
  error = slot_remove_pointer(fwSlot);
#endif 
  (void)nss_ZFreeIf(fwSlot);
  return error;
}





NSS_IMPLEMENT NSSCKMDSlot *
nssCKFWSlot_GetMDSlot
(
  NSSCKFWSlot *fwSlot
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWSlot_verifyPointer(fwSlot) ) {
    return (NSSCKMDSlot *)NULL;
  }
#endif 

  return fwSlot->mdSlot;
}






NSS_IMPLEMENT NSSCKFWInstance *
nssCKFWSlot_GetFWInstance
(
  NSSCKFWSlot *fwSlot
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWSlot_verifyPointer(fwSlot) ) {
    return (NSSCKFWInstance *)NULL;
  }
#endif 

  return fwSlot->fwInstance;
}






NSS_IMPLEMENT NSSCKMDInstance *
nssCKFWSlot_GetMDInstance
(
  NSSCKFWSlot *fwSlot
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWSlot_verifyPointer(fwSlot) ) {
    return (NSSCKMDInstance *)NULL;
  }
#endif 

  return fwSlot->mdInstance;
}





NSS_IMPLEMENT CK_SLOT_ID
nssCKFWSlot_GetSlotID
(
  NSSCKFWSlot *fwSlot
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWSlot_verifyPointer(fwSlot) ) {
    return (CK_SLOT_ID)0;
  }
#endif 

  return fwSlot->slotID;
}





NSS_IMPLEMENT CK_RV
nssCKFWSlot_GetSlotDescription
(
  NSSCKFWSlot *fwSlot,
  CK_CHAR slotDescription[64]
)
{
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  if( (CK_CHAR_PTR)NULL == slotDescription ) {
    return CKR_ARGUMENTS_BAD;
  }

  error = nssCKFWSlot_verifyPointer(fwSlot);
  if( CKR_OK != error ) {
    return error;
  }
#endif 

  error = nssCKFWMutex_Lock(fwSlot->mutex);
  if( CKR_OK != error ) {
    return error;
  }

  if( (NSSUTF8 *)NULL == fwSlot->slotDescription ) {
    if( (void *)NULL != (void *)fwSlot->mdSlot->GetSlotDescription ) {
      fwSlot->slotDescription = fwSlot->mdSlot->GetSlotDescription(
        fwSlot->mdSlot, fwSlot, fwSlot->mdInstance, 
        fwSlot->fwInstance, &error);
      if( ((NSSUTF8 *)NULL == fwSlot->slotDescription) && (CKR_OK != error) ) {
        goto done;
      }
    } else {
      fwSlot->slotDescription = (NSSUTF8 *) "";
    }
  }

  (void)nssUTF8_CopyIntoFixedBuffer(fwSlot->slotDescription, (char *)slotDescription, 64, ' ');
  error = CKR_OK;

 done:
  (void)nssCKFWMutex_Unlock(fwSlot->mutex);
  return error;
}





NSS_IMPLEMENT CK_RV
nssCKFWSlot_GetManufacturerID
(
  NSSCKFWSlot *fwSlot,
  CK_CHAR manufacturerID[32]
)
{
  CK_RV error = CKR_OK;

#ifdef NSSDEBUG
  if( (CK_CHAR_PTR)NULL == manufacturerID ) {
    return CKR_ARGUMENTS_BAD;
  }

  error = nssCKFWSlot_verifyPointer(fwSlot);
  if( CKR_OK != error ) {
    return error;
  }
#endif 

  error = nssCKFWMutex_Lock(fwSlot->mutex);
  if( CKR_OK != error ) {
    return error;
  }

  if( (NSSUTF8 *)NULL == fwSlot->manufacturerID ) {
    if( (void *)NULL != (void *)fwSlot->mdSlot->GetManufacturerID ) {
      fwSlot->manufacturerID = fwSlot->mdSlot->GetManufacturerID(
        fwSlot->mdSlot, fwSlot, fwSlot->mdInstance, 
        fwSlot->fwInstance, &error);
      if( ((NSSUTF8 *)NULL == fwSlot->manufacturerID) && (CKR_OK != error) ) {
        goto done;
      }
    } else {
      fwSlot->manufacturerID = (NSSUTF8 *) "";
    }
  }

  (void)nssUTF8_CopyIntoFixedBuffer(fwSlot->manufacturerID, (char *)manufacturerID, 32, ' ');
  error = CKR_OK;

 done:
  (void)nssCKFWMutex_Unlock(fwSlot->mutex);
  return error;
}





NSS_IMPLEMENT CK_BBOOL
nssCKFWSlot_GetTokenPresent
(
  NSSCKFWSlot *fwSlot
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWSlot_verifyPointer(fwSlot) ) {
    return CK_FALSE;
  }
#endif 

  if( (void *)NULL == (void *)fwSlot->mdSlot->GetTokenPresent ) {
    return CK_TRUE;
  }

  return fwSlot->mdSlot->GetTokenPresent(fwSlot->mdSlot, fwSlot,
    fwSlot->mdInstance, fwSlot->fwInstance);
}





NSS_IMPLEMENT CK_BBOOL
nssCKFWSlot_GetRemovableDevice
(
  NSSCKFWSlot *fwSlot
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWSlot_verifyPointer(fwSlot) ) {
    return CK_FALSE;
  }
#endif 

  if( (void *)NULL == (void *)fwSlot->mdSlot->GetRemovableDevice ) {
    return CK_FALSE;
  }

  return fwSlot->mdSlot->GetRemovableDevice(fwSlot->mdSlot, fwSlot,
    fwSlot->mdInstance, fwSlot->fwInstance);
}





NSS_IMPLEMENT CK_BBOOL
nssCKFWSlot_GetHardwareSlot
(
  NSSCKFWSlot *fwSlot
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWSlot_verifyPointer(fwSlot) ) {
    return CK_FALSE;
  }
#endif 

  if( (void *)NULL == (void *)fwSlot->mdSlot->GetHardwareSlot ) {
    return CK_FALSE;
  }

  return fwSlot->mdSlot->GetHardwareSlot(fwSlot->mdSlot, fwSlot,
    fwSlot->mdInstance, fwSlot->fwInstance);
}





NSS_IMPLEMENT CK_VERSION
nssCKFWSlot_GetHardwareVersion
(
  NSSCKFWSlot *fwSlot
)
{
  CK_VERSION rv;

#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWSlot_verifyPointer(fwSlot) ) {
    rv.major = rv.minor = 0;
    return rv;
  }
#endif 

  if( CKR_OK != nssCKFWMutex_Lock(fwSlot->mutex) ) {
    rv.major = rv.minor = 0;
    return rv;
  }

  if( (0 != fwSlot->hardwareVersion.major) ||
      (0 != fwSlot->hardwareVersion.minor) ) {
    rv = fwSlot->hardwareVersion;
    goto done;
  }

  if( (void *)NULL != (void *)fwSlot->mdSlot->GetHardwareVersion ) {
    fwSlot->hardwareVersion = fwSlot->mdSlot->GetHardwareVersion(
      fwSlot->mdSlot, fwSlot, fwSlot->mdInstance, fwSlot->fwInstance);
  } else {
    fwSlot->hardwareVersion.major = 0;
    fwSlot->hardwareVersion.minor = 1;
  }

  rv = fwSlot->hardwareVersion;
 done:
  (void)nssCKFWMutex_Unlock(fwSlot->mutex);
  return rv;
}





NSS_IMPLEMENT CK_VERSION
nssCKFWSlot_GetFirmwareVersion
(
  NSSCKFWSlot *fwSlot
)
{
  CK_VERSION rv;

#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWSlot_verifyPointer(fwSlot) ) {
    rv.major = rv.minor = 0;
    return rv;
  }
#endif 

  if( CKR_OK != nssCKFWMutex_Lock(fwSlot->mutex) ) {
    rv.major = rv.minor = 0;
    return rv;
  }

  if( (0 != fwSlot->firmwareVersion.major) ||
      (0 != fwSlot->firmwareVersion.minor) ) {
    rv = fwSlot->firmwareVersion;
    goto done;
  }

  if( (void *)NULL != (void *)fwSlot->mdSlot->GetFirmwareVersion ) {
    fwSlot->firmwareVersion = fwSlot->mdSlot->GetFirmwareVersion(
      fwSlot->mdSlot, fwSlot, fwSlot->mdInstance, fwSlot->fwInstance);
  } else {
    fwSlot->firmwareVersion.major = 0;
    fwSlot->firmwareVersion.minor = 1;
  }

  rv = fwSlot->firmwareVersion;
 done:
  (void)nssCKFWMutex_Unlock(fwSlot->mutex);
  return rv;
}





NSS_IMPLEMENT NSSCKFWToken *
nssCKFWSlot_GetToken
(
  NSSCKFWSlot *fwSlot,
  CK_RV *pError
)
{
  NSSCKMDToken *mdToken;
  NSSCKFWToken *fwToken;

#ifdef NSSDEBUG
  if( (CK_RV *)NULL == pError ) {
    return (NSSCKFWToken *)NULL;
  }

  *pError = nssCKFWSlot_verifyPointer(fwSlot);
  if( CKR_OK != *pError ) {
    return (NSSCKFWToken *)NULL;
  }
#endif 

  *pError = nssCKFWMutex_Lock(fwSlot->mutex);
  if( CKR_OK != *pError ) {
    return (NSSCKFWToken *)NULL;
  }

  if( (NSSCKFWToken *)NULL == fwSlot->fwToken ) {
    if( (void *)NULL == (void *)fwSlot->mdSlot->GetToken ) {
      *pError = CKR_GENERAL_ERROR;
      fwToken = (NSSCKFWToken *)NULL;
      goto done;
    }

    mdToken = fwSlot->mdSlot->GetToken(fwSlot->mdSlot, fwSlot,
      fwSlot->mdInstance, fwSlot->fwInstance, pError);
    if( (NSSCKMDToken *)NULL == mdToken ) {
      if( CKR_OK == *pError ) {
        *pError = CKR_GENERAL_ERROR;
      }
      return (NSSCKFWToken *)NULL;
    }

    fwToken = nssCKFWToken_Create(fwSlot, mdToken, pError);
    fwSlot->fwToken = fwToken;
  } else {
    fwToken = fwSlot->fwToken;
  }

 done:
  (void)nssCKFWMutex_Unlock(fwSlot->mutex);
  return fwToken;
}





NSS_IMPLEMENT void
nssCKFWSlot_ClearToken
(
  NSSCKFWSlot *fwSlot
)
{
#ifdef NSSDEBUG
  if( CKR_OK != nssCKFWSlot_verifyPointer(fwSlot) ) {
    return;
  }
#endif 

  if( CKR_OK != nssCKFWMutex_Lock(fwSlot->mutex) ) {
    
    return;
  }

  fwSlot->fwToken = (NSSCKFWToken *)NULL;
  (void)nssCKFWMutex_Unlock(fwSlot->mutex);
  return;
}






NSS_IMPLEMENT NSSCKMDSlot *
NSSCKFWSlot_GetMDSlot
(
  NSSCKFWSlot *fwSlot
)
{
#ifdef DEBUG
  if( CKR_OK != nssCKFWSlot_verifyPointer(fwSlot) ) {
    return (NSSCKMDSlot *)NULL;
  }
#endif 

  return nssCKFWSlot_GetMDSlot(fwSlot);
}






NSS_IMPLEMENT NSSCKFWInstance *
NSSCKFWSlot_GetFWInstance
(
  NSSCKFWSlot *fwSlot
)
{
#ifdef DEBUG
  if( CKR_OK != nssCKFWSlot_verifyPointer(fwSlot) ) {
    return (NSSCKFWInstance *)NULL;
  }
#endif 

  return nssCKFWSlot_GetFWInstance(fwSlot);
}






NSS_IMPLEMENT NSSCKMDInstance *
NSSCKFWSlot_GetMDInstance
(
  NSSCKFWSlot *fwSlot
)
{
#ifdef DEBUG
  if( CKR_OK != nssCKFWSlot_verifyPointer(fwSlot) ) {
    return (NSSCKMDInstance *)NULL;
  }
#endif 

  return nssCKFWSlot_GetMDInstance(fwSlot);
}
