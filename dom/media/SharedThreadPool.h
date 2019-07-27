





#ifndef SharedThreadPool_h_
#define SharedThreadPool_h_

#include <queue>
#include "mozilla/RefPtr.h"
#include "nsThreadUtils.h"
#include "nsIThreadPool.h"
#include "nsISupports.h"
#include "nsISupportsImpl.h"
#include "nsCOMPtr.h"

namespace mozilla {









class SharedThreadPool : public nsIThreadPool
{
public:

  
  
  
  static TemporaryRef<SharedThreadPool> Get(const nsCString& aName,
                                            uint32_t aThreadLimit = 4);

  
  
  
  
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr) override;
  NS_IMETHOD_(MozExternalRefCountType) AddRef(void) override;
  NS_IMETHOD_(MozExternalRefCountType) Release(void) override;

  
  NS_FORWARD_SAFE_NSITHREADPOOL(mPool);
  NS_FORWARD_SAFE_NSIEVENTTARGET(mEventTarget);

  
  static void InitStatics();

  
  
  static void SpinUntilEmpty();

private:

  
  static bool IsEmpty();

  
  
  
  SharedThreadPool(const nsCString& aName,
                   nsIThreadPool* aPool);
  virtual ~SharedThreadPool();

  nsresult EnsureThreadLimitIsAtLeast(uint32_t aThreadLimit);

  
  const nsCString mName;

  
  nsCOMPtr<nsIThreadPool> mPool;

  
  
  nsrefcnt mRefCnt;

  
  
  nsCOMPtr<nsIEventTarget> mEventTarget;
};

} 

#endif 
