



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: mtoken.c,v $ $Revision: 1.2 $ $Date: 2012/04/25 14:49:40 $";
#endif 

#include "ckmk.h"








static NSSUTF8 *
ckmk_mdToken_GetLabel
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_ckmk_TokenLabel;
}

static NSSUTF8 *
ckmk_mdToken_GetManufacturerID
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_ckmk_ManufacturerID;
}

static NSSUTF8 *
ckmk_mdToken_GetModel
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_ckmk_TokenModel;
}

static NSSUTF8 *
ckmk_mdToken_GetSerialNumber
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_ckmk_TokenSerialNumber;
}

static CK_BBOOL
ckmk_mdToken_GetIsWriteProtected
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
ckmk_mdToken_GetUserPinInitialized
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
ckmk_mdToken_GetHardwareVersion
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return nss_ckmk_HardwareVersion;
}

static CK_VERSION
ckmk_mdToken_GetFirmwareVersion
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return nss_ckmk_FirmwareVersion;
}

static NSSCKMDSession *
ckmk_mdToken_OpenSession
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
  return nss_ckmk_CreateSession(fwSession, pError);
}

static CK_ULONG
ckmk_mdToken_GetMechanismCount
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
ckmk_mdToken_GetMechanismTypes
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
ckmk_mdToken_GetMechanism
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
  return (NSSCKMDMechanism *)&nss_ckmk_mdMechanismRSA;
}

NSS_IMPLEMENT_DATA const NSSCKMDToken
nss_ckmk_mdToken = {
  (void *)NULL, 
  NULL, 
  NULL, 
  NULL, 
  ckmk_mdToken_GetLabel,
  ckmk_mdToken_GetManufacturerID,
  ckmk_mdToken_GetModel,
  ckmk_mdToken_GetSerialNumber,
  NULL, 
  ckmk_mdToken_GetIsWriteProtected,
  NULL, 
  ckmk_mdToken_GetUserPinInitialized,
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
  ckmk_mdToken_GetHardwareVersion,
  ckmk_mdToken_GetFirmwareVersion,
  NULL, 
  ckmk_mdToken_OpenSession,
  ckmk_mdToken_GetMechanismCount,
  ckmk_mdToken_GetMechanismTypes,
  ckmk_mdToken_GetMechanism,
  (void *)NULL 
};
