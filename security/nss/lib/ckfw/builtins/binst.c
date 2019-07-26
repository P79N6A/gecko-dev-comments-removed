



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: binst.c,v $ $Revision: 1.6 $ $Date: 2012/04/25 14:49:29 $";
#endif 

#include "builtins.h"












static CK_ULONG
builtins_mdInstance_GetNSlots
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (CK_ULONG)1;
}

static CK_VERSION
builtins_mdInstance_GetCryptokiVersion
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return nss_builtins_CryptokiVersion;
}

static NSSUTF8 *
builtins_mdInstance_GetManufacturerID
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_builtins_ManufacturerID;
}

static NSSUTF8 *
builtins_mdInstance_GetLibraryDescription
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_builtins_LibraryDescription;
}

static CK_VERSION
builtins_mdInstance_GetLibraryVersion
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  extern const char __nss_builtins_rcsid[];
  extern const char __nss_builtins_sccsid[];
  volatile char c; 

  c = __nss_builtins_rcsid[0] + __nss_builtins_sccsid[0];
  return nss_builtins_LibraryVersion;
}

static CK_RV
builtins_mdInstance_GetSlots
(
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  NSSCKMDSlot *slots[]
)
{
  slots[0] = (NSSCKMDSlot *)&nss_builtins_mdSlot;
  return CKR_OK;
}

const NSSCKMDInstance
nss_builtins_mdInstance = {
  (void *)NULL, 
  NULL, 
  NULL, 
  builtins_mdInstance_GetNSlots,
  builtins_mdInstance_GetCryptokiVersion,
  builtins_mdInstance_GetManufacturerID,
  builtins_mdInstance_GetLibraryDescription,
  builtins_mdInstance_GetLibraryVersion,
  NULL, 
  builtins_mdInstance_GetSlots,
  NULL, 
  (void *)NULL 
};
