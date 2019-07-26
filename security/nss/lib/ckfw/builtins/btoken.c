



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: btoken.c,v $ $Revision: 1.5 $ $Date: 2012/04/25 14:49:29 $";
#endif 

#include "builtins.h"








static NSSUTF8 *
builtins_mdToken_GetLabel
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_builtins_TokenLabel;
}

static NSSUTF8 *
builtins_mdToken_GetManufacturerID
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_builtins_ManufacturerID;
}

static NSSUTF8 *
builtins_mdToken_GetModel
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_builtins_TokenModel;
}

static NSSUTF8 *
builtins_mdToken_GetSerialNumber
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance,
  CK_RV *pError
)
{
  return (NSSUTF8 *)nss_builtins_TokenSerialNumber;
}

static CK_BBOOL
builtins_mdToken_GetIsWriteProtected
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
builtins_mdToken_GetHardwareVersion
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return nss_builtins_HardwareVersion;
}

static CK_VERSION
builtins_mdToken_GetFirmwareVersion
(
  NSSCKMDToken *mdToken,
  NSSCKFWToken *fwToken,
  NSSCKMDInstance *mdInstance,
  NSSCKFWInstance *fwInstance
)
{
  return nss_builtins_FirmwareVersion;
}

static NSSCKMDSession *
builtins_mdToken_OpenSession
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
  return nss_builtins_CreateSession(fwSession, pError);
}

const NSSCKMDToken
nss_builtins_mdToken = {
  (void *)NULL, 
  NULL, 
  NULL, 
  NULL, 
  builtins_mdToken_GetLabel,
  builtins_mdToken_GetManufacturerID,
  builtins_mdToken_GetModel,
  builtins_mdToken_GetSerialNumber,
  NULL, 
  builtins_mdToken_GetIsWriteProtected,
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
  NULL, 
  NULL, 
  builtins_mdToken_GetHardwareVersion,
  builtins_mdToken_GetFirmwareVersion,
  NULL, 
  builtins_mdToken_OpenSession,
  NULL, 
  NULL, 
  NULL, 
  (void *)NULL 
};
