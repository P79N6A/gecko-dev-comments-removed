























#include "OCSPCache.h"

#include <limits>

#include "NSSCertDBTrustDomain.h"
#include "pk11pub.h"
#include "pkix/pkixtypes.h"
#include "ScopedNSSTypes.h"
#include "secerr.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gCertVerifierLog;
#endif

using namespace mozilla::pkix;

namespace mozilla { namespace psm {












static SECStatus
CertIDHash(SHA384Buffer& buf, const CertID& certID)
{
  ScopedPK11Context context(PK11_CreateDigestContext(SEC_OID_SHA384));
  if (!context) {
    return SECFailure;
  }
  SECStatus rv = PK11_DigestBegin(context.get());
  if (rv != SECSuccess) {
    return rv;
  }
  rv = PK11_DigestOp(context.get(), certID.issuer.data, certID.issuer.len);
  if (rv != SECSuccess) {
    return rv;
  }
  rv = PK11_DigestOp(context.get(), certID.issuerSubjectPublicKeyInfo.data,
                     certID.issuerSubjectPublicKeyInfo.len);
  if (rv != SECSuccess) {
    return rv;
  }
  rv = PK11_DigestOp(context.get(), certID.serialNumber.data,
                     certID.serialNumber.len);
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
OCSPCache::Entry::Init(const CertID& aCertID, PRErrorCode aErrorCode,
                       PRTime aThisUpdate, PRTime aValidThrough)
{
  mErrorCode = aErrorCode;
  mThisUpdate = aThisUpdate;
  mValidThrough = aValidThrough;
  return CertIDHash(mIDHash, aCertID);
}

OCSPCache::OCSPCache()
  : mMutex("OCSPCache-mutex")
{
}

OCSPCache::~OCSPCache()
{
  Clear();
}



bool
OCSPCache::FindInternal(const CertID& aCertID,  size_t& index,
                        const MutexAutoLock& )
{
  if (mEntries.length() == 0) {
    return false;
  }

  SHA384Buffer idHash;
  SECStatus rv = CertIDHash(idHash, aCertID);
  if (rv != SECSuccess) {
    return false;
  }

  
  
  index = mEntries.length();
  while (index > 0) {
    --index;
    if (memcmp(mEntries[index]->mIDHash, idHash, SHA384_LENGTH) == 0) {
      return true;
    }
  }
  return false;
}

static inline void
LogWithCertID(const char* aMessage, const CertID& aCertID)
{
  PR_LOG(gCertVerifierLog, PR_LOG_DEBUG, (aMessage, &aCertID));
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
OCSPCache::Get(const CertID& aCertID, PRErrorCode& aErrorCode,
               PRTime& aValidThrough)
{
  MutexAutoLock lock(mMutex);

  size_t index;
  if (!FindInternal(aCertID, index, lock)) {
    LogWithCertID("OCSPCache::Get(%p) not in cache", aCertID);
    return false;
  }
  LogWithCertID("OCSPCache::Get(%p) in cache", aCertID);
  aErrorCode = mEntries[index]->mErrorCode;
  aValidThrough = mEntries[index]->mValidThrough;
  MakeMostRecentlyUsed(index, lock);
  return true;
}

SECStatus
OCSPCache::Put(const CertID& aCertID, PRErrorCode aErrorCode,
               PRTime aThisUpdate, PRTime aValidThrough)
{
  MutexAutoLock lock(mMutex);

  size_t index;
  if (FindInternal(aCertID, index, lock)) {
    
    if (mEntries[index]->mErrorCode == SEC_ERROR_REVOKED_CERTIFICATE) {
      LogWithCertID("OCSPCache::Put(%p) already in cache as revoked - "
                    "not replacing", aCertID);
      MakeMostRecentlyUsed(index, lock);
      return SECSuccess;
    }

    
    
    if (mEntries[index]->mThisUpdate > aThisUpdate &&
        aErrorCode != SEC_ERROR_REVOKED_CERTIFICATE) {
      LogWithCertID("OCSPCache::Put(%p) already in cache with more recent "
                    "validity - not replacing", aCertID);
      MakeMostRecentlyUsed(index, lock);
      return SECSuccess;
    }

    
    
    if (aErrorCode != 0 && aErrorCode != SEC_ERROR_OCSP_UNKNOWN_CERT &&
        aErrorCode != SEC_ERROR_REVOKED_CERTIFICATE) {
      LogWithCertID("OCSPCache::Put(%p) already in cache - not replacing "
                    "with less important status", aCertID);
      MakeMostRecentlyUsed(index, lock);
      return SECSuccess;
    }

    LogWithCertID("OCSPCache::Put(%p) already in cache - replacing", aCertID);
    mEntries[index]->mErrorCode = aErrorCode;
    mEntries[index]->mThisUpdate = aThisUpdate;
    mEntries[index]->mValidThrough = aValidThrough;
    MakeMostRecentlyUsed(index, lock);
    return SECSuccess;
  }

  if (mEntries.length() == MaxEntries) {
    LogWithCertID("OCSPCache::Put(%p) too full - evicting an entry", aCertID);
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
  SECStatus rv = newEntry->Init(aCertID, aErrorCode, aThisUpdate,
                                aValidThrough);
  if (rv != SECSuccess) {
    delete newEntry;
    return rv;
  }
  mEntries.append(newEntry);
  LogWithCertID("OCSPCache::Put(%p) added to cache", aCertID);
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
