























#include "OCSPCache.h"

#include "NSSCertDBTrustDomain.h"
#include "pk11pub.h"
#include "secerr.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gCertVerifierLog;
#endif

namespace mozilla { namespace psm {

void
MozillaPKIX_PK11_DestroyContext_true(PK11Context* context)
{
  PK11_DestroyContext(context, true);
}

typedef mozilla::pkix::ScopedPtr<PK11Context,
                                 MozillaPKIX_PK11_DestroyContext_true>
                                 ScopedPK11Context;












static SECStatus
CertIDHash(SHA384Buffer& buf, const CERTCertificate* aCert,
       const CERTCertificate* aIssuerCert)
{
  ScopedPK11Context context(PK11_CreateDigestContext(SEC_OID_SHA384));
  if (!context) {
    return SECFailure;
  }
  SECStatus rv = PK11_DigestBegin(context.get());
  if (rv != SECSuccess) {
    return rv;
  }
  rv = PK11_DigestOp(context.get(), aCert->derIssuer.data,
                     aCert->derIssuer.len);
  if (rv != SECSuccess) {
    return rv;
  }
  rv = PK11_DigestOp(context.get(), aIssuerCert->derPublicKey.data,
                     aIssuerCert->derPublicKey.len);
  if (rv != SECSuccess) {
    return rv;
  }
  rv = PK11_DigestOp(context.get(), aCert->serialNumber.data,
                     aCert->serialNumber.len);
  if (rv != SECSuccess) {
    return rv;
  }
  uint32_t outLen = 0;
  rv = PK11_DigestFinal(context.get(), buf, &outLen, SHA384_LENGTH);
  if (outLen != SHA384_LENGTH) {
    return SECFailure;
  }
  return rv;
}

SECStatus
OCSPCache::Entry::Init(const CERTCertificate* aCert,
                       const CERTCertificate* aIssuerCert,
                       PRErrorCode aErrorCode,
                       PRTime aThisUpdate,
                       PRTime aValidThrough)
{
  mErrorCode = aErrorCode;
  mThisUpdate = aThisUpdate;
  mValidThrough = aValidThrough;
  return CertIDHash(mIDHash, aCert, aIssuerCert);
}

OCSPCache::OCSPCache()
  : mMutex("OCSPCache-mutex")
{
}

OCSPCache::~OCSPCache()
{
  Clear();
}


int32_t
OCSPCache::FindInternal(const CERTCertificate* aCert,
                        const CERTCertificate* aIssuerCert,
                        const MutexAutoLock& )
{
  if (mEntries.length() == 0) {
    return -1;
  }

  SHA384Buffer idHash;
  SECStatus rv = CertIDHash(idHash, aCert, aIssuerCert);
  if (rv != SECSuccess) {
    return -1;
  }

  
  
  for (int32_t i = mEntries.length() - 1; i >= 0; i--) {
    if (memcmp(mEntries[i]->mIDHash, idHash, SHA384_LENGTH) == 0) {
      return i;
    }
  }
  return -1;
}

void
OCSPCache::LogWithCerts(const char* aMessage, const CERTCertificate* aCert,
                        const CERTCertificate* aIssuerCert)
{
#ifdef PR_LOGGING
  if (PR_LOG_TEST(gCertVerifierLog, PR_LOG_DEBUG)) {
    mozilla::pkix::ScopedPtr<char, mozilla::psm::PORT_Free_string>
      cn(CERT_GetCommonName(&aCert->subject));
    mozilla::pkix::ScopedPtr<char, mozilla::psm::PORT_Free_string>
      cnIssuer(CERT_GetCommonName(&aIssuerCert->subject));
    PR_LOG(gCertVerifierLog, PR_LOG_DEBUG, (aMessage, cn.get(), cnIssuer.get()));
  }
#endif
}

void
OCSPCache::MakeMostRecentlyUsed(size_t aIndex,
                                const MutexAutoLock& )
{
  Entry* entry = mEntries[aIndex];
  
  
  mEntries.erase(mEntries.begin() + aIndex);
  mEntries.append(entry);
}

bool
OCSPCache::Get(const CERTCertificate* aCert,
               const CERTCertificate* aIssuerCert,
               PRErrorCode& aErrorCode,
               PRTime& aValidThrough)
{
  PR_ASSERT(aCert);
  PR_ASSERT(aIssuerCert);

  MutexAutoLock lock(mMutex);

  int32_t index = FindInternal(aCert, aIssuerCert, lock);
  if (index < 0) {
    LogWithCerts("OCSPCache::Get(%s, %s) not in cache", aCert, aIssuerCert);
    return false;
  }
  LogWithCerts("OCSPCache::Get(%s, %s) in cache", aCert, aIssuerCert);
  aErrorCode = mEntries[index]->mErrorCode;
  aValidThrough = mEntries[index]->mValidThrough;
  MakeMostRecentlyUsed(index, lock);
  return true;
}

SECStatus
OCSPCache::Put(const CERTCertificate* aCert,
               const CERTCertificate* aIssuerCert,
               PRErrorCode aErrorCode,
               PRTime aThisUpdate,
               PRTime aValidThrough)
{
  PR_ASSERT(aCert);
  PR_ASSERT(aIssuerCert);

  MutexAutoLock lock(mMutex);

  int32_t index = FindInternal(aCert, aIssuerCert, lock);

  if (index >= 0) {
    
    if (mEntries[index]->mErrorCode == SEC_ERROR_REVOKED_CERTIFICATE) {
      LogWithCerts("OCSPCache::Put(%s, %s) already in cache as revoked - "
                   "not replacing", aCert, aIssuerCert);
      MakeMostRecentlyUsed(index, lock);
      return SECSuccess;
    }

    
    
    if (mEntries[index]->mThisUpdate > aThisUpdate &&
        aErrorCode != SEC_ERROR_REVOKED_CERTIFICATE) {
      LogWithCerts("OCSPCache::Put(%s, %s) already in cache with more recent "
                   "validity - not replacing", aCert, aIssuerCert);
      MakeMostRecentlyUsed(index, lock);
      return SECSuccess;
    }

    
    
    if (aErrorCode != 0 && aErrorCode != SEC_ERROR_OCSP_UNKNOWN_CERT &&
        aErrorCode != SEC_ERROR_REVOKED_CERTIFICATE) {
      LogWithCerts("OCSPCache::Put(%s, %s) already in cache - not replacing "
                   "with less important status", aCert, aIssuerCert);
      MakeMostRecentlyUsed(index, lock);
      return SECSuccess;
    }

    LogWithCerts("OCSPCache::Put(%s, %s) already in cache - replacing",
                 aCert, aIssuerCert);
    mEntries[index]->mErrorCode = aErrorCode;
    mEntries[index]->mThisUpdate = aThisUpdate;
    mEntries[index]->mValidThrough = aValidThrough;
    MakeMostRecentlyUsed(index, lock);
    return SECSuccess;
  }

  if (mEntries.length() == MaxEntries) {
    LogWithCerts("OCSPCache::Put(%s, %s) too full - evicting an entry", aCert,
                 aIssuerCert);
    for (Entry** toEvict = mEntries.begin(); toEvict != mEntries.end();
         toEvict++) {
      
      
      if ((*toEvict)->mErrorCode != SEC_ERROR_REVOKED_CERTIFICATE &&
          (*toEvict)->mErrorCode != SEC_ERROR_OCSP_UNKNOWN_CERT) {
        delete *toEvict;
        mEntries.erase(toEvict);
        break;
      }
    }
    
    
    
    
    
    
    
    if (mEntries.length() == MaxEntries) {
      if (aErrorCode != 0) {
        PR_SetError(aErrorCode, 0);
        return SECFailure;
      }
      return SECSuccess;
    }
  }

  Entry* newEntry = new Entry();
  
  
  
  if (!newEntry) {
    PR_SetError(SEC_ERROR_NO_MEMORY, 0);
    return SECFailure;
  }
  SECStatus rv = newEntry->Init(aCert, aIssuerCert, aErrorCode, aThisUpdate,
                                aValidThrough);
  if (rv != SECSuccess) {
    return rv;
  }
  mEntries.append(newEntry);
  LogWithCerts("OCSPCache::Put(%s, %s) added to cache", aCert, aIssuerCert);
  return SECSuccess;
}

void
OCSPCache::Clear()
{
  MutexAutoLock lock(mMutex);
  PR_LOG(gCertVerifierLog, PR_LOG_DEBUG, ("OCSPCache::Clear: clearing cache"));
  
  
  for (Entry** entry = mEntries.begin(); entry < mEntries.end();
       entry++) {
    delete *entry;
  }
  
  mEntries.clearAndFree();
}

} } 
