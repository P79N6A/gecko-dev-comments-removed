


#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: minst.c,v $ $Revision: 1.2 $ $Date: 2012/04/25 14:49:40 $";
#endif 

#include "ckmk.h"












static CK_ULONG
ckmk_mdInstance_GetNSlots
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (CK_ULONG)1;
}

static CK_VERSION
ckmk_mdInstance_GetCryptokiVersion
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return nss_ckmk_CryptokiVersion;
}

static NSSUTF8 *
ckmk_mdInstance_GetManufacturerID
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_ckmk_ManufacturerID;
}

static NSSUTF8 *
ckmk_mdInstance_GetLibraryDescription
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_ckmk_LibraryDescription;
}

static CK_VERSION
ckmk_mdInstance_GetLibraryVersion
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return nss_ckmk_LibraryVersion;
}

static CK_RV
ckmk_mdInstance_GetSlots
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  NSSCKMDSlot *slots[]
)
{
  slots[0] = (NSSCKMDSlot *)&nss_ckmk_mdSlot;
  return CKR_OK;
}

static CK_BBOOL
ckmk_mdInstance_ModuleHandlesSessionObjects
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  


  return CK_TRUE;
}

NSS_IMPLEMENT_DATA const NSSCKMDInstance
nss_ckmk_mdInstance = {
  (void *)NULL, 
  NULL, 
  NULL, 
  ckmk_mdInstance_GetNSlots,
  ckmk_mdInstance_GetCryptokiVersion,
  ckmk_mdInstance_GetManufacturerID,
  ckmk_mdInstance_GetLibraryDescription,
  ckmk_mdInstance_GetLibraryVersion,
  ckmk_mdInstance_ModuleHandlesSessionObjects, 
  
  ckmk_mdInstance_GetSlots,
  NULL, 
  (void *)NULL 
};
