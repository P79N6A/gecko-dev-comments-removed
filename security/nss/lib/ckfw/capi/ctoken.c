



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: ctoken.c,v $ $Revision: 1.3 $ $Date: 2012/04/25 14:49:30 $";
#endif 

#include "ckcapi.h"








static NSSUTF8 *
ckcapi_mdToken_GetLabel
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_ckcapi_TokenLabel;
}

static NSSUTF8 *
ckcapi_mdToken_GetManufacturerID
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_ckcapi_ManufacturerID;
}

static NSSUTF8 *
ckcapi_mdToken_GetModel
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_ckcapi_TokenModel;
}

static NSSUTF8 *
ckcapi_mdToken_GetSerialNumber
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_ckcapi_TokenSerialNumber;
}

static CK_BBOOL
ckcapi_mdToken_GetIsWriteProtected
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return CK_FALSE;
}


static CK_BBOOL
ckcapi_mdToken_GetUserPinInitialized
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return CK_TRUE;
}

static CK_VERSION
ckcapi_mdToken_GetHardwareVersion
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return nss_ckcapi_HardwareVersion;
}

static CK_VERSION
ckcapi_mdToken_GetFirmwareVersion
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return nss_ckcapi_FirmwareVersion;
}

static NSSCKMDSession *
ckcapi_mdToken_OpenSession
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  NSSCKFWSession *fwSession,
  CK_BBOOL rw,
  CK_RV *pError
)
{
  return nss_ckcapi_CreateSession(fwSession, pError);
}

static CK_ULONG
ckcapi_mdToken_GetMechanismCount
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return (CK_ULONG)1;
}

static CK_RV
ckcapi_mdToken_GetMechanismTypes
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_MECHANISM_TYPE types[]
)
{
  types[0] = CKM_RSA_PKCS;
  return CKR_OK;
}

static NSSCKMDMechanism *
ckcapi_mdToken_GetMechanism
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_MECHANISM_TYPE which,
  CK_RV *pError
)
{
  if (which != CKM_RSA_PKCS) {
    *pError = CKR_MECHANISM_INVALID;
    return (NSSCKMDMechanism *)NULL;
  }
  return (NSSCKMDMechanism *)&nss_ckcapi_mdMechanismRSA;
}

NSS_IMPLEMENT_DATA const NSSCKMDToken
nss_ckcapi_mdToken = {
  (void *)NULL, 
  NULL, 
  NULL, 
  NULL, 
  ckcapi_mdToken_GetLabel,
  ckcapi_mdToken_GetManufacturerID,
  ckcapi_mdToken_GetModel,
  ckcapi_mdToken_GetSerialNumber,
  NULL, 
  ckcapi_mdToken_GetIsWriteProtected,
  NULL, 
  ckcapi_mdToken_GetUserPinInitialized,
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  ckcapi_mdToken_GetHardwareVersion,
  ckcapi_mdToken_GetFirmwareVersion,
  NULL, 
  ckcapi_mdToken_OpenSession,
  ckcapi_mdToken_GetMechanismCount,
  ckcapi_mdToken_GetMechanismTypes,
  ckcapi_mdToken_GetMechanism,
  (void *)NULL 
};
