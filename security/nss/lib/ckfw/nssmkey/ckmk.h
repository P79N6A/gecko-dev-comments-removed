



#ifndef CKMK_H
#define CKMK_H 1

#ifdef DEBUG
static const char CKMK_CVS_ID[] = "@(#) $RCSfile: ckmk.h,v $ $Revision: 1.3 $ $Date: 2012/04/25 14:49:40 $";
#endif 

#include <Security/SecKeychainSearch.h>
#include <Security/SecKeychainItem.h>
#include <Security/SecKeychain.h>
#include <Security/cssmtype.h>
#include <Security/cssmapi.h>
#include <Security/SecKey.h>
#include <Security/SecCertificate.h>

#define NTO

#include "nssckmdt.h"
#include "nssckfw.h"




#ifndef BASE_H
#include "base.h"
#endif 



#ifndef CKT_H
#include "ckt.h"
#endif 





struct ckmkRawObjectStr {
  CK_ULONG n;
  const CK_ATTRIBUTE_TYPE *types;
  const NSSItem *items;
};
typedef struct ckmkRawObjectStr ckmkRawObject;




struct ckmkItemObjectStr {
  SecKeychainItemRef itemRef;
  SecItemClass    itemClass;
  PRBool          hasID;
  NSSItem	  modify;
  NSSItem	  private;
  NSSItem	  encrypt;
  NSSItem	  decrypt;
  NSSItem	  derive;
  NSSItem	  sign;
  NSSItem	  signRecover;
  NSSItem	  verify;
  NSSItem	  verifyRecover;
  NSSItem	  wrap;
  NSSItem	  unwrap;
  NSSItem	  label;
  NSSItem	  subject;
  NSSItem	  issuer;
  NSSItem	  serial;
  NSSItem	  derCert;
  NSSItem	  id;
  NSSItem	  modulus;
  NSSItem	  exponent;
  NSSItem	  privateExponent;
  NSSItem	  prime1;
  NSSItem	  prime2;
  NSSItem	  exponent1;
  NSSItem	  exponent2;
  NSSItem	  coefficient;
};
typedef struct ckmkItemObjectStr ckmkItemObject;

typedef enum {
  ckmkRaw,
  ckmkItem,
} ckmkObjectType;





struct ckmkInternalObjectStr {
  ckmkObjectType type;
  union {
    ckmkRawObject  raw;
    ckmkItemObject item;
  } u;
  CK_OBJECT_CLASS objClass;
  NSSItem	  hashKey;
  unsigned char   hashKeyData[128];
  NSSCKMDObject mdObject;
};
typedef struct ckmkInternalObjectStr ckmkInternalObject;


NSS_EXTERN_DATA ckmkInternalObject nss_ckmk_data[];
NSS_EXTERN_DATA const PRUint32               nss_ckmk_nObjects;

NSS_EXTERN_DATA const CK_VERSION   nss_ckmk_CryptokiVersion;
NSS_EXTERN_DATA const NSSUTF8 *    nss_ckmk_ManufacturerID;
NSS_EXTERN_DATA const NSSUTF8 *    nss_ckmk_LibraryDescription;
NSS_EXTERN_DATA const CK_VERSION   nss_ckmk_LibraryVersion;
NSS_EXTERN_DATA const NSSUTF8 *    nss_ckmk_SlotDescription;
NSS_EXTERN_DATA const CK_VERSION   nss_ckmk_HardwareVersion;
NSS_EXTERN_DATA const CK_VERSION   nss_ckmk_FirmwareVersion;
NSS_EXTERN_DATA const NSSUTF8 *    nss_ckmk_TokenLabel;
NSS_EXTERN_DATA const NSSUTF8 *    nss_ckmk_TokenModel;
NSS_EXTERN_DATA const NSSUTF8 *    nss_ckmk_TokenSerialNumber;

NSS_EXTERN_DATA const NSSCKMDInstance  nss_ckmk_mdInstance;
NSS_EXTERN_DATA const NSSCKMDSlot      nss_ckmk_mdSlot;
NSS_EXTERN_DATA const NSSCKMDToken     nss_ckmk_mdToken;
NSS_EXTERN_DATA const NSSCKMDMechanism nss_ckmk_mdMechanismRSA;

NSS_EXTERN NSSCKMDSession *
nss_ckmk_CreateSession
(
  NSSCKFWSession *fwSession,
  CK_RV *pError
);

NSS_EXTERN NSSCKMDFindObjects *
nss_ckmk_FindObjectsInit
(
  NSSCKFWSession *fwSession,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulAttributeCount,
  CK_RV *pError
);




NSS_EXTERN NSSCKMDObject *
nss_ckmk_CreateMDObject
(
  NSSArena *arena,
  ckmkInternalObject *io,
  CK_RV *pError
);

NSS_EXTERN NSSCKMDObject *
nss_ckmk_CreateObject
(
  NSSCKFWSession *fwSession,
  CK_ATTRIBUTE_PTR pTemplate,
  CK_ULONG ulAttributeCount,
  CK_RV *pError
);

NSS_EXTERN const NSSItem *
nss_ckmk_FetchAttribute
(
  ckmkInternalObject *io, 
  CK_ATTRIBUTE_TYPE type,
  CK_RV *pError
);

NSS_EXTERN void
nss_ckmk_DestroyInternalObject
(
  ckmkInternalObject *io
);

unsigned char *
nss_ckmk_DERUnwrap
(
  unsigned char *src,
  int size,
  int *outSize,
  unsigned char **next
);

CK_ULONG
nss_ckmk_GetULongAttribute
(
  CK_ATTRIBUTE_TYPE type,
  CK_ATTRIBUTE *template,
  CK_ULONG templateSize,
  CK_RV *pError
);

#define NSS_CKMK_ARRAY_SIZE(x) ((sizeof (x))/(sizeof ((x)[0])))

#ifdef DEBUG
#define CKMK_MACERR(str,err) cssmPerror(str,err)
#else
#define CKMK_MACERR(str,err) 
#endif
 
#endif
