





#ifndef nsThreadManager_h__
#define nsThreadManager_h__

#include "mozilla/Mutex.h"
#include "nsIThreadManager.h"
#include "nsRefPtrHashtable.h"
#include "nsThread.h"

class nsIRunnable;

class nsThreadManager : public nsIThreadManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITHREADMANAGER

  static nsThreadManager *get() {
    static nsThreadManager sInstance;
    return &sInstance;
  }

  nsresult Init();

  
  
  void Shutdown();

  
  
  void RegisterCurrentThread(nsThread *thread);

  
  
  void UnregisterCurrentThread(nsThread *thread);

  
  
  nsThread *GetCurrentThread();

  
  
  uint32_t GetHighestNumberOfThreads();

  
  
  ~nsThreadManager() {}

private:
  nsThreadManager()
    : mCurThreadIndex(0)
    , mMainPRThread(nullptr)
    , mLock(nullptr)
    , mInitialized(false)
    , mCurrentNumberOfThreads(1)
    , mHighestNumberOfThreads(1) {
  }

  nsRefPtrHashtable<nsPtrHashKey<PRThread>, nsThread> mThreadsByPRThread;
  unsigned             mCurThreadIndex;  
  nsRefPtr<nsThread>  mMainThread;
  PRThread           *mMainPRThread;
  
  
  nsAutoPtr<mozilla::Mutex> mLock;  
  bool                mInitialized;

   
   uint32_t           mCurrentNumberOfThreads;
   
   uint32_t           mHighestNumberOfThreads;
};

#define NS_THREADMANAGER_CID                       \
{ /* 7a4204c6-e45a-4c37-8ebb-6709a22c917c */       \
  0x7a4204c6,                                      \
  0xe45a,                                          \
  0x4c37,                                          \
  {0x8e, 0xbb, 0x67, 0x09, 0xa2, 0x2c, 0x91, 0x7c} \
}

#endif  
