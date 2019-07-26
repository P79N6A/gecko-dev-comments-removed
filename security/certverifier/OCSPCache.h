























#ifndef mozilla_psm_OCSPCache_h
#define mozilla_psm_OCSPCache_h

#include "certt.h"
#include "hasht.h"
#include "pkix/pkixtypes.h"
#include "mozilla/Mutex.h"
#include "mozilla/Vector.h"
#include "prerror.h"

namespace mozilla { namespace psm {


typedef uint8_t SHA384Buffer[SHA384_LENGTH];







class OCSPCache
{
public:
  OCSPCache();
  ~OCSPCache();

  
  
  
  
  bool Get(const CERTCertificate* aCert, const CERTCertificate* aIssuerCert,
            PRErrorCode& aErrorCode,  PRTime& aValidThrough);

  
  
  
  
  
  
  
  
  
  SECStatus Put(const CERTCertificate* aCert,
                const CERTCertificate* aIssuerCert,
                PRErrorCode aErrorCode,
                PRTime aThisUpdate,
                PRTime aValidThrough);

  
  void Clear();

private:
  class Entry
  {
  public:
    SECStatus Init(const CERTCertificate* aCert,
                   const CERTCertificate* aIssuerCert,
                   PRErrorCode aErrorCode, PRTime aThisUpdate,
                   PRTime aValidThrough);

    PRErrorCode mErrorCode;
    PRTime mThisUpdate;
    PRTime mValidThrough;
    
    
    
    SHA384Buffer mIDHash;
  };

  bool FindInternal(const CERTCertificate* aCert,
                    const CERTCertificate* aIssuerCert,
                     size_t& index,
                    const MutexAutoLock& aProofOfLock);
  void MakeMostRecentlyUsed(size_t aIndex, const MutexAutoLock& aProofOfLock);
  void LogWithCerts(const char* aMessage, const CERTCertificate* aCert,
                    const CERTCertificate* aIssuerCert);

  Mutex mMutex;
  static const size_t MaxEntries = 1024;
  
  
  
  
  
  Vector<Entry*, 256> mEntries;
};

} } 

#endif 
