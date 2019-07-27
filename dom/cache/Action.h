





#ifndef mozilla_dom_cache_Action_h
#define mozilla_dom_cache_Action_h

#include "mozilla/Atomics.h"
#include "mozilla/dom/cache/Types.h"
#include "nsISupportsImpl.h"

class mozIStorageConnection;

namespace mozilla {
namespace dom {
namespace cache {

class Action
{
public:
  class Resolver
  {
  public:
    
    
    
    virtual void Resolve(nsresult aRv) = 0;

    NS_IMETHOD_(MozExternalRefCountType)
    AddRef(void) = 0;

    NS_IMETHOD_(MozExternalRefCountType)
    Release(void) = 0;
  };

  
  
  
  
  class Data
  {
  public:
    virtual mozIStorageConnection*
    GetConnection() const = 0;

    virtual void
    SetConnection(mozIStorageConnection* aConn) = 0;
  };

  
  
  
  
  
  virtual void RunOnTarget(Resolver* aResolver, const QuotaInfo& aQuotaInfo,
                           Data* aOptionalData) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual void CancelOnInitiatingThread();

  
  
  virtual void CompleteOnInitiatingThread(nsresult aRv) { }

  
  
  virtual bool MatchesCacheId(CacheId aCacheId) const { return false; }

  NS_INLINE_DECL_REFCOUNTING(cache::Action)

protected:
  Action();

  
  virtual ~Action();

  
  
  bool IsCanceled() const;

private:
  
  Atomic<bool> mCanceled;
};

} 
} 
} 

#endif 
