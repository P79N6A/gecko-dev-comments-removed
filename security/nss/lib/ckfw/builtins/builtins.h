



#ifdef DEBUG
static const char BUILTINS_CVS_ID[] = "@(#) $RCSfile: builtins.h,v $ $Revision: 1.7 $ $Date: 2012/04/25 14:49:29 $";
#endif 

#include "nssckmdt.h"
#include "nssckfw.h"





#ifndef BASE_H
#include "base.h"
#endif 




#ifndef CKT_H
#include "ckt.h"
#endif 

struct builtinsInternalObjectStr {
  CK_ULONG n;
  const CK_ATTRIBUTE_TYPE *types;
  const NSSItem *items;
  NSSCKMDObject mdObject;
};
typedef struct builtinsInternalObjectStr builtinsInternalObject;

extern       builtinsInternalObject nss_builtins_data[];
extern const PRUint32               nss_builtins_nObjects;

extern const CK_VERSION   nss_builtins_CryptokiVersion;
extern const CK_VERSION   nss_builtins_LibraryVersion;
extern const CK_VERSION   nss_builtins_HardwareVersion;
extern const CK_VERSION   nss_builtins_FirmwareVersion;

extern const NSSUTF8      nss_builtins_ManufacturerID[];
extern const NSSUTF8      nss_builtins_LibraryDescription[];
extern const NSSUTF8      nss_builtins_SlotDescription[];
extern const NSSUTF8      nss_builtins_TokenLabel[];
extern const NSSUTF8      nss_builtins_TokenModel[];
extern const NSSUTF8      nss_builtins_TokenSerialNumber[];

extern const NSSCKMDInstance nss_builtins_mdInstance;
extern const NSSCKMDSlot     nss_builtins_mdSlot;
extern const NSSCKMDToken    nss_builtins_mdToken;

NSS_EXTERN NSSCKMDSession *
nss_builtins_CreateSession
(
  NSSCKFWSession *fwSession,
  CK_RV *pError
);

NSS_EXTERN NSSCKMDFindObjects *
nss_builtins_FindObjectsInit
(
  NSSCKFWSession *fwSession,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulAttributeCount,
  CK_RV *pError
);

NSS_EXTERN NSSCKMDObject *
nss_builtins_CreateMDObject
(
  NSSArena *arena,
  builtinsInternalObject *io,
  CK_RV *pError
);
