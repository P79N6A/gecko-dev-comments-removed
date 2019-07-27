





#ifndef nsThreadManager_h__
#define nsThreadManager_h__

#include "mozilla/Mutex.h"
#include "nsIThreadManager.h"
#include "nsRefPtrHashtable.h"
#include "nsThread.h"

class nsIRunnable;

namespace mozilla {
class ReentrantMonitor;
}

class nsThreadManager : public nsIThreadManager
{
public:
#ifdef MOZ_NUWA_PROCESS
  struct ThreadStatusInfo;
  class AllThreadsWereIdleListener {
  public:
    NS_INLINE_DECL_REFCOUNTING(AllThreadsWereIdleListener);
    virtual void OnAllThreadsWereIdle() = 0;
  protected:
    virtual ~AllThreadsWereIdleListener()
    {
    }
  };
#endif 

  NS_DECL_ISUPPORTS
  NS_DECL_NSITHREADMANAGER

  static nsThreadManager* get()
  {
    static nsThreadManager sInstance;
    return &sInstance;
  }

  nsresult Init();

  
  
  void Shutdown();

  
  
  void RegisterCurrentThread(nsThread* aThread);

  
  
  void UnregisterCurrentThread(nsThread* aThread);

  
  
  nsThread* GetCurrentThread();

  
  
  uint32_t GetHighestNumberOfThreads();

  
  
  ~nsThreadManager()
  {
  }

#ifdef MOZ_NUWA_PROCESS
  
  
  
  void SetThreadIdle(nsIRunnable** aReturnRunnable);
  void SetThreadWorking();

  
  
  
  
  
  
  void SetThreadIsWorking(ThreadStatusInfo* aInfo,
                          bool aIsWorking,
                          nsIRunnable** aReturnRunnable);
  void ResetIsDispatchingToMainThread();

  void AddAllThreadsWereIdleListener(AllThreadsWereIdleListener *listener);
  void RemoveAllThreadsWereIdleListener(AllThreadsWereIdleListener *listener);
  ThreadStatusInfo* GetCurrentThreadStatusInfo();
#endif 

private:
  nsThreadManager()
    : mCurThreadIndex(0)
    , mMainPRThread(nullptr)
    , mLock("nsThreadManager.mLock")
    , mInitialized(false)
    , mCurrentNumberOfThreads(1)
    , mHighestNumberOfThreads(1)
#ifdef MOZ_NUWA_PROCESS
    , mMonitor(nullptr)
    , mMainThreadStatusInfo(nullptr)
    , mDispatchingToMainThread(nullptr)
#endif
  {
  }

  nsRefPtrHashtable<nsPtrHashKey<PRThread>, nsThread> mThreadsByPRThread;
  unsigned            mCurThreadIndex;  
  nsRefPtr<nsThread>  mMainThread;
  PRThread*           mMainPRThread;
  mozilla::OffTheBooksMutex mLock;  
  mozilla::Atomic<bool> mInitialized;

  
  uint32_t            mCurrentNumberOfThreads;
  
  uint32_t            mHighestNumberOfThreads;

#ifdef MOZ_NUWA_PROCESS
  static void DeleteThreadStatusInfo(void *aData);
  unsigned mThreadStatusInfoIndex;
  nsTArray<nsRefPtr<AllThreadsWereIdleListener>> mThreadsIdledListeners;
  nsTArray<ThreadStatusInfo*> mThreadStatusInfos;
  mozilla::UniquePtr<mozilla::ReentrantMonitor> mMonitor;
  ThreadStatusInfo* mMainThreadStatusInfo;
  
  
  
  bool mDispatchingToMainThread;
#endif 
};

#define NS_THREADMANAGER_CID                       \
{ /* 7a4204c6-e45a-4c37-8ebb-6709a22c917c */       \
  0x7a4204c6,                                      \
  0xe45a,                                          \
  0x4c37,                                          \
  {0x8e, 0xbb, 0x67, 0x09, 0xa2, 0x2c, 0x91, 0x7c} \
}

#endif  
