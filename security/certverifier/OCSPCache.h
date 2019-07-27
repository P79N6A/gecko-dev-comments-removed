























#ifndef mozilla_psm_OCSPCache_h
#define mozilla_psm_OCSPCache_h

#include "hasht.h"
#include "mozilla/Mutex.h"
#include "mozilla/Vector.h"
#include "pkix/Result.h"
#include "prerror.h"
#include "prtime.h"
#include "seccomon.h"

namespace mozilla { namespace pkix {
struct CertID;
} } 

namespace mozilla { namespace psm {


typedef uint8_t SHA384Buffer[SHA384_LENGTH];







class OCSPCache
{
public:
  OCSPCache();
  ~OCSPCache();

  
  
  
  
  bool Get(const mozilla::pkix::CertID& aCertID,
            mozilla::pkix::Result& aResult,
            PRTime& aValidThrough);

  
  
  
  
  
  
  
  
  
  mozilla::pkix::Result Put(const mozilla::pkix::CertID& aCertID,
                            mozilla::pkix::Result aResult, PRTime aThisUpdate,
                            PRTime aValidThrough);

  
  void Clear();

private:
  class Entry
  {
  public:
    mozilla::pkix::Result Init(const mozilla::pkix::CertID& aCertID,
                               mozilla::pkix::Result aResult,
                               PRTime aThisUpdate, PRTime aValidThrough);

    mozilla::pkix::Result mResult;
    PRTime mThisUpdate;
    PRTime mValidThrough;
    
    
    
    SHA384Buffer mIDHash;
  };

  bool FindInternal(const mozilla::pkix::CertID& aCertID,  size_t& index,
                    const MutexAutoLock& aProofOfLock);
  void MakeMostRecentlyUsed(size_t aIndex, const MutexAutoLock& aProofOfLock);

  Mutex mMutex;
  static const size_t MaxEntries = 1024;
  
  
  
  
  
  Vector<Entry*, 256> mEntries;
};

} } 

#endif 
