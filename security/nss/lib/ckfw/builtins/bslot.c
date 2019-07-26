



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: bslot.c,v $ $Revision: 1.5 $ $Date: 2012/04/25 14:49:29 $";
#endif 

#include "builtins.h"








static NSSUTF8 *
builtins_mdSlot_GetSlotDescription
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_builtins_SlotDescription;
}

static NSSUTF8 *
builtins_mdSlot_GetManufacturerID
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_builtins_ManufacturerID;
}

static CK_VERSION
builtins_mdSlot_GetHardwareVersion
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return nss_builtins_HardwareVersion;
}

static CK_VERSION
builtins_mdSlot_GetFirmwareVersion
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return nss_builtins_FirmwareVersion;
}

static NSSCKMDToken *
builtins_mdSlot_GetToken
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSCKMDToken *)&nss_builtins_mdToken;
}

const NSSCKMDSlot
nss_builtins_mdSlot = {
  (void *)NULL, 
  NULL, 
  NULL, 
  builtins_mdSlot_GetSlotDescription,
  builtins_mdSlot_GetManufacturerID,
  NULL, 
  NULL, 
  NULL, 
  builtins_mdSlot_GetHardwareVersion,
  builtins_mdSlot_GetFirmwareVersion,
  builtins_mdSlot_GetToken,
  (void *)NULL 
};
