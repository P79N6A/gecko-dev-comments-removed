



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: mslot.c,v $ $Revision: 1.2 $ $Date: 2012/04/25 14:49:40 $";
#endif 

#include "ckmk.h"








static NSSUTF8 *
ckmk_mdSlot_GetSlotDescription
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_ckmk_SlotDescription;
}

static NSSUTF8 *
ckmk_mdSlot_GetManufacturerID
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_ckmk_ManufacturerID;
}

static CK_VERSION
ckmk_mdSlot_GetHardwareVersion
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return nss_ckmk_HardwareVersion;
}

static CK_VERSION
ckmk_mdSlot_GetFirmwareVersion
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return nss_ckmk_FirmwareVersion;
}

static NSSCKMDToken *
ckmk_mdSlot_GetToken
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSCKMDToken *)&nss_ckmk_mdToken;
}

NSS_IMPLEMENT_DATA const NSSCKMDSlot
nss_ckmk_mdSlot = {
  (void *)NULL, 
  NULL, 
  NULL, 
  ckmk_mdSlot_GetSlotDescription,
  ckmk_mdSlot_GetManufacturerID,
  NULL, 
  NULL, 
  NULL, 
  ckmk_mdSlot_GetHardwareVersion,
  ckmk_mdSlot_GetFirmwareVersion,
  ckmk_mdSlot_GetToken,
  (void *)NULL 
};
