
















#ifndef insanity_pkix__pkixtypes_h
#define insanity_pkix__pkixtypes_h

#include "insanity/ScopedPtr.h"
#include "plarena.h"
#include "cert.h"
#include "keyhi.h"

namespace insanity { namespace pkix {

typedef ScopedPtr<PLArenaPool, PL_FreeArenaPool> ScopedPLArenaPool;

typedef ScopedPtr<CERTCertificate, CERT_DestroyCertificate>
        ScopedCERTCertificate;
typedef ScopedPtr<CERTCertList, CERT_DestroyCertList> ScopedCERTCertList;
typedef ScopedPtr<SECKEYPublicKey, SECKEY_DestroyPublicKey>
        ScopedSECKEYPublicKey;

} } 

#endif 
