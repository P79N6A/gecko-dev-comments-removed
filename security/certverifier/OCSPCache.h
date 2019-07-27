























#ifndef mozilla_psm_OCSPCache_h
#define mozilla_psm_OCSPCache_h

#include "hasht.h"
#include "mozilla/Mutex.h"
#include "mozilla/Vector.h"
#include "pkix/Result.h"
#include "pkix/Time.h"
#include "prerror.h"
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
            mozilla::pkix::Time& aValidThrough);

  
  
  
  
  
  
  
  
  
  mozilla::pkix::Result Put(const mozilla::pkix::CertID& aCertID,
                            mozilla::pkix::Result aResult,
                            mozilla::pkix::Time aThisUpdate,
                            mozilla::pkix::Time aValidThrough);

  
  void Clear();

private:
  class Entry
  {
  public:
    Entry(mozilla::pkix::Result aResult,
          mozilla::pkix::Time aThisUpdate,
          mozilla::pkix::Time aValidThrough)
      : mResult(aResult)
      , mThisUpdate(aThisUpdate)
      , mValidThrough(aValidThrough)
    {
    }
    mozilla::pkix::Result Init(const mozilla::pkix::CertID& aCertID);

    mozilla::pkix::Result mResult;
    mozilla::pkix::Time mThisUpdate;
    mozilla::pkix::Time mValidThrough;
    
    
    
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
