





#ifndef mozilla_dom_cache_Action_h
#define mozilla_dom_cache_Action_h

#include "mozilla/Atomics.h"
#include "mozilla/dom/cache/Types.h"
#include "nsISupportsImpl.h"

namespace mozilla {
namespace dom {
namespace cache {

class Action
{
public:
  class Resolver : public nsISupports
  {
  protected:
    
    virtual ~Resolver() { }

  public:
    
    
    
    virtual void Resolve(nsresult aRv) = 0;

    
    
    NS_DECL_THREADSAFE_ISUPPORTS
  };

  
  
  
  
  
  virtual void RunOnTarget(Resolver* aResolver, const QuotaInfo& aQuotaInfo) = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual void CancelOnInitiatingThread();

  
  
  virtual void CompleteOnInitiatingThread(nsresult aRv) { }

  
  
  virtual bool MatchesCacheId(CacheId aCacheId) const { return false; }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(cache::Action)
  NS_DECL_OWNINGTHREAD

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
