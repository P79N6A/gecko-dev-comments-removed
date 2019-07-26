


#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: cinst.c,v $ $Revision: 1.3 $ $Date: 2012/04/25 14:49:30 $";
#endif 

#include "ckcapi.h"












static CK_ULONG
ckcapi_mdInstance_GetNSlots
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (CK_ULONG)1;
}

static CK_VERSION
ckcapi_mdInstance_GetCryptokiVersion
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return nss_ckcapi_CryptokiVersion;
}

static NSSUTF8 *
ckcapi_mdInstance_GetManufacturerID
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_ckcapi_ManufacturerID;
}

static NSSUTF8 *
ckcapi_mdInstance_GetLibraryDescription
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_ckcapi_LibraryDescription;
}

static CK_VERSION
ckcapi_mdInstance_GetLibraryVersion
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return nss_ckcapi_LibraryVersion;
}

static CK_RV
ckcapi_mdInstance_GetSlots
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  NSSCKMDSlot *slots[]
)
{
  slots[0] = (NSSCKMDSlot *)&nss_ckcapi_mdSlot;
  return CKR_OK;
}

static CK_BBOOL
ckcapi_mdInstance_ModuleHandlesSessionObjects
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  


  return CK_TRUE;
}

NSS_IMPLEMENT_DATA const NSSCKMDInstance
nss_ckcapi_mdInstance = {
  (void *)NULL, 
  NULL, 
  NULL, 
  ckcapi_mdInstance_GetNSlots,
  ckcapi_mdInstance_GetCryptokiVersion,
  ckcapi_mdInstance_GetManufacturerID,
  ckcapi_mdInstance_GetLibraryDescription,
  ckcapi_mdInstance_GetLibraryVersion,
  ckcapi_mdInstance_ModuleHandlesSessionObjects, 
  
  ckcapi_mdInstance_GetSlots,
  NULL, 
  (void *)NULL 
};
