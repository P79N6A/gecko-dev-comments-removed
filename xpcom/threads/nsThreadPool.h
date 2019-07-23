





































#ifndef nsThreadPool_h__
#define nsThreadPool_h__

#include "nsIThreadPool.h"
#include "nsIThread.h"
#include "nsIRunnable.h"
#include "nsEventQueue.h"
#include "nsCOMArray.h"

class nsThreadPool : public nsIThreadPool, public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEVENTTARGET
  NS_DECL_NSITHREADPOOL
  NS_DECL_NSIRUNNABLE

  nsThreadPool();

private:
  ~nsThreadPool();

  void ShutdownThread(nsIThread *thread);
  nsresult PutEvent(nsIRunnable *event);

  nsCOMArray<nsIThread> mThreads;
  nsEventQueue          mEvents;
  PRUint32              mThreadLimit;
  PRUint32              mIdleThreadLimit;
  PRUint32              mIdleThreadTimeout;
  PRUint32              mIdleCount;
  PRBool                mShutdown;
};

#define NS_THREADPOOL_CLASSNAME "nsThreadPool"
#define NS_THREADPOOL_CID                          \
{ /* 547ec2a8-315e-4ec4-888e-6e4264fe90eb */       \
  0x547ec2a8,                                      \
  0x315e,                                          \
  0x4ec4,                                          \
  {0x88, 0x8e, 0x6e, 0x42, 0x64, 0xfe, 0x90, 0xeb} \
}

#endif  
