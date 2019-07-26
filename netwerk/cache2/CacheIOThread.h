



#ifndef CacheIOThread__h__
#define CacheIOThread__h__

#include "nsIThreadInternal.h"
#include "nsISupportsImpl.h"
#include "prthread.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "mozilla/Monitor.h"

class nsIRunnable;

namespace mozilla {
namespace net {

class CacheIOThread : public nsIThreadObserver
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSITHREADOBSERVER

  CacheIOThread();
  virtual ~CacheIOThread();

  enum ELevel {
    OPEN_PRIORITY,
    READ_PRIORITY,
    OPEN,
    READ,
    WRITE,
    MANAGEMENT,
    CLOSE,
    INDEX,
    EVICT,
    LAST_LEVEL,

    
    
    
    XPCOM_LEVEL
  };

  nsresult Init();
  nsresult Dispatch(nsIRunnable* aRunnable, uint32_t aLevel);
  bool IsCurrentThread();

  








  static bool YieldAndRerun()
  {
    return sSelf ? sSelf->YieldInternal() : false;
  }

  nsresult Shutdown();
  already_AddRefed<nsIEventTarget> Target();

  
  size_t SizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const;
  size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const;

private:
  static void ThreadFunc(void* aClosure);
  void ThreadFunc();
  void LoopOneLevel(uint32_t aLevel);
  bool EventsPending(uint32_t aLastLevel = LAST_LEVEL);
  bool YieldInternal();

  static CacheIOThread* sSelf;

  mozilla::Monitor mMonitor;
  PRThread* mThread;
  nsCOMPtr<nsIThread> mXPCOMThread;
  uint32_t mLowestLevelWaiting;
  uint32_t mCurrentlyExecutingLevel;
  nsTArray<nsRefPtr<nsIRunnable> > mEventQueue[LAST_LEVEL];

  bool mHasXPCOMEvents;
  bool mRerunCurrentEvent;
  bool mShutdown;
};

} 
} 

#endif
