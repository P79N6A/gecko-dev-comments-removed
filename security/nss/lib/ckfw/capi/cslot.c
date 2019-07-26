



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: cslot.c,v $ $Revision: 1.2 $ $Date: 2012/04/25 14:49:30 $";
#endif 

#include "ckcapi.h"








static NSSUTF8 *
ckcapi_mdSlot_GetSlotDescription
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_ckcapi_SlotDescription;
}

static NSSUTF8 *
ckcapi_mdSlot_GetManufacturerID
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_ckcapi_ManufacturerID;
}

static CK_VERSION
ckcapi_mdSlot_GetHardwareVersion
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return nss_ckcapi_HardwareVersion;
}

static CK_VERSION
ckcapi_mdSlot_GetFirmwareVersion
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return nss_ckcapi_FirmwareVersion;
}

static NSSCKMDToken *
ckcapi_mdSlot_GetToken
(
  NSSCKMDSlot *mdSlot,
  NSSCKFWSlot *fwSlot,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSCKMDToken *)&nss_ckcapi_mdToken;
}

NSS_IMPLEMENT_DATA const NSSCKMDSlot
nss_ckcapi_mdSlot = {
  (void *)NULL, 
  NULL, 
  NULL, 
  ckcapi_mdSlot_GetSlotDescription,
  ckcapi_mdSlot_GetManufacturerID,
  NULL, 
  NULL, 
  NULL, 
  ckcapi_mdSlot_GetHardwareVersion,
  ckcapi_mdSlot_GetFirmwareVersion,
  ckcapi_mdSlot_GetToken,
  (void *)NULL 
};
