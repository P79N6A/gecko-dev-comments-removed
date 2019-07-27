





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

  
  
  
  static already_AddRefed<SharedThreadPool> Get(const nsCString& aName,
                                            uint32_t aThreadLimit = 4);

  
  
  
  
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr) override;
  NS_IMETHOD_(MozExternalRefCountType) AddRef(void) override;
  NS_IMETHOD_(MozExternalRefCountType) Release(void) override;

  
  NS_FORWARD_SAFE_NSITHREADPOOL(mPool);

  
  
  nsresult Dispatch(nsIRunnable *event, uint32_t flags) { return !mEventTarget ? NS_ERROR_NULL_POINTER : mEventTarget->Dispatch(event, flags); }
 
  NS_IMETHOD DispatchFromScript(nsIRunnable *event, uint32_t flags) override {
      return Dispatch(event, flags);
  }

  NS_IMETHOD Dispatch(already_AddRefed<nsIRunnable>&& event, uint32_t flags) override
    { return !mEventTarget ? NS_ERROR_NULL_POINTER : mEventTarget->Dispatch(Move(event), flags); }

  NS_IMETHOD IsOnCurrentThread(bool *_retval) override { return !mEventTarget ? NS_ERROR_NULL_POINTER : mEventTarget->IsOnCurrentThread(_retval); }

  
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
