



#ifndef PKIT_H
#define PKIT_H







#ifndef NSSBASET_H
#include "nssbaset.h"
#endif 

#ifndef BASET_H
#include "baset.h"
#endif 

#include "certt.h"
#include "pkcs11t.h"

#ifndef NSSPKIT_H
#include "nsspkit.h"
#endif 

#ifndef NSSDEVT_H
#include "nssdevt.h"
#endif 

#ifndef DEVT_H
#include "devt.h"
#endif 

#ifndef nssrwlkt_h__
#include "nssrwlkt.h"
#endif 

PR_BEGIN_EXTERN_C
















typedef enum {
    nssPKILock = 1,
    nssPKIMonitor = 2
} nssPKILockType;






struct nssPKIObjectStr 
{
    
    NSSArena *arena;
    
    PRInt32 refCount;
    
    union {
        PZLock* lock;
        PZMonitor *mlock;
    } sync;
    nssPKILockType lockType;
    



    nssCryptokiObject **instances;
    PRUint32 numInstances;
    
    NSSTrustDomain *trustDomain;
    
    NSSCryptoContext *cryptoContext;
    
    NSSUTF8 *tempName;
};

typedef struct nssDecodedCertStr nssDecodedCert;

typedef struct nssCertificateStoreStr nssCertificateStore;


typedef struct nssSMIMEProfileStr nssSMIMEProfile;

typedef struct nssPKIObjectStr nssPKIObject;

struct NSSTrustStr 
{
    nssPKIObject object;
    NSSCertificate *certificate;
    nssTrustLevel serverAuth;
    nssTrustLevel clientAuth;
    nssTrustLevel emailProtection;
    nssTrustLevel codeSigning;
    PRBool stepUpApproved;
};

struct nssSMIMEProfileStr
{
    nssPKIObject object;
    NSSCertificate *certificate;
    NSSASCII7 *email;
    NSSDER *subject;
    NSSItem *profileTime;
    NSSItem *profileData;
};

struct NSSCertificateStr
{
    nssPKIObject object;
    NSSCertificateType type;
    NSSItem id;
    NSSBER encoding;
    NSSDER issuer;
    NSSDER subject;
    NSSDER serial;
    NSSASCII7 *email;
    nssDecodedCert *decoding;
};

struct NSSPrivateKeyStr;

struct NSSPublicKeyStr;

struct NSSSymmetricKeyStr;

typedef struct nssTDCertificateCacheStr nssTDCertificateCache;

struct NSSTrustDomainStr {
    PRInt32 refCount;
    NSSArena *arena;
    NSSCallback *defaultCallback;
    nssList *tokenList;
    nssListIterator *tokens;
    nssTDCertificateCache *cache;
    NSSRWLock *tokensLock;
    void *spkDigestInfo;
    CERTStatusConfig *statusConfig;
};

struct NSSCryptoContextStr
{
    PRInt32 refCount;
    NSSArena *arena;
    NSSTrustDomain *td;
    NSSToken *token;
    nssSession *session;
    nssCertificateStore *certStore;
};

struct NSSTimeStr {
    PRTime prTime;
};

struct NSSCRLStr {
  nssPKIObject object;
  NSSDER encoding;
  NSSUTF8 *url;
  PRBool isKRL;
};

typedef struct NSSCRLStr NSSCRL;

struct NSSPoliciesStr;

struct NSSAlgorithmAndParametersStr;

struct NSSPKIXCertificateStr;

PR_END_EXTERN_C

#endif 
