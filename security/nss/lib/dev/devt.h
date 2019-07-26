



#ifndef DEVT_H
#define DEVT_H

#ifdef DEBUG
static const char DEVT_CVS_ID[] = "@(#) $RCSfile: devt.h,v $ $Revision: 1.26 $ $Date: 2012/04/25 14:49:42 $";
#endif 







#ifndef NSSBASET_H
#include "nssbaset.h"
#endif 

#ifndef NSSPKIT_H
#include "nsspkit.h"
#endif 

#ifndef NSSDEVT_H
#include "nssdevt.h"
#endif 

#ifndef BASET_H
#include "baset.h"
#endif 

#include "secmodt.h"

PR_BEGIN_EXTERN_C

typedef struct nssSessionStr nssSession;


struct nssDeviceBaseStr
{
  NSSArena *arena;
  PZLock *lock;
  PRInt32 refCount;
  NSSUTF8 *name;
  PRUint32 flags;
};

typedef struct nssTokenObjectCacheStr nssTokenObjectCache;


struct NSSTokenStr
{
    struct nssDeviceBaseStr base;
    NSSSlot *slot;  
    CK_FLAGS ckFlags; 
    PRUint32 flags;
    void *epv;
    nssSession *defaultSession;
    NSSTrustDomain *trustDomain;
    PRIntervalTime lastTime;
    nssTokenObjectCache *cache;
    PK11SlotInfo *pk11slot;
};

typedef enum {
  nssSlotAskPasswordTimes_FirstTime = 0,
  nssSlotAskPasswordTimes_EveryTime = 1,
  nssSlotAskPasswordTimes_Timeout = 2
} 
nssSlotAskPasswordTimes;

struct nssSlotAuthInfoStr
{
  PRTime lastLogin;
  nssSlotAskPasswordTimes askTimes;
  PRIntervalTime askPasswordTimeout;
};

struct NSSSlotStr
{
  struct nssDeviceBaseStr base;
  NSSModule *module; 
  NSSToken *token;  
  CK_SLOT_ID slotID;
  CK_FLAGS ckFlags; 
  struct nssSlotAuthInfoStr authInfo;
  PRIntervalTime lastTokenPing;
  PZLock *lock;
  void *epv;
  PK11SlotInfo *pk11slot;
};

struct nssSessionStr
{
  PZLock *lock;
  CK_SESSION_HANDLE handle;
  NSSSlot *slot;
  PRBool isRW;
  PRBool ownLock;
};

typedef enum {
    NSSCertificateType_Unknown = 0,
    NSSCertificateType_PKIX = 1
} NSSCertificateType;

typedef enum {
    nssTrustLevel_Unknown = 0,
    nssTrustLevel_NotTrusted = 1,
    nssTrustLevel_Trusted = 2,
    nssTrustLevel_TrustedDelegator = 3,
    nssTrustLevel_MustVerify = 4,
    nssTrustLevel_ValidDelegator = 5
} nssTrustLevel;

typedef struct nssCryptokiInstanceStr nssCryptokiInstance;

struct nssCryptokiInstanceStr
{
    CK_OBJECT_HANDLE handle;
    NSSToken *token;
    PRBool isTokenObject;
    NSSUTF8 *label;
};

typedef struct nssCryptokiInstanceStr nssCryptokiObject;

typedef struct nssTokenCertSearchStr nssTokenCertSearch;

typedef enum {
    nssTokenSearchType_AllObjects = 0,
    nssTokenSearchType_SessionOnly = 1,
    nssTokenSearchType_TokenOnly = 2,
    nssTokenSearchType_TokenForced = 3
} nssTokenSearchType;

struct nssTokenCertSearchStr
{
    nssTokenSearchType searchType;
    PRStatus (* callback)(NSSCertificate *c, void *arg);
    void *cbarg;
    nssList *cached;
    


};

struct nssSlotListStr;
typedef struct nssSlotListStr nssSlotList;

struct NSSAlgorithmAndParametersStr
{
    CK_MECHANISM mechanism;
};

PR_END_EXTERN_C

#endif 
