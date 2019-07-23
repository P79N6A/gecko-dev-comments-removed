





































#ifndef nsThreadManager_h__
#define nsThreadManager_h__

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
    return &sInstance;
  }

  nsresult Init();

  
  
  void Shutdown();

  
  
  void RegisterCurrentThread(nsThread *thread);

  
  
  void UnregisterCurrentThread(nsThread *thread);

  
  nsThread *GetCurrentThread();

  
  
  ~nsThreadManager() {}

private:
  nsThreadManager()
    : mCurThreadIndex(0)
    , mMainPRThread(nsnull)
    , mLock(nsnull)
    , mInitialized(PR_FALSE) {
  }
  
  static nsThreadManager sInstance;

  nsRefPtrHashtable<nsVoidPtrHashKey, nsThread> mThreadsByPRThread;
  PRUintn             mCurThreadIndex;  
  nsRefPtr<nsThread>  mMainThread;
  PRThread           *mMainPRThread;
  PRLock             *mLock;  
  PRBool              mInitialized;
};

#define NS_THREADMANAGER_CLASSNAME "nsThreadManager"
#define NS_THREADMANAGER_CID                       \
{ /* 7a4204c6-e45a-4c37-8ebb-6709a22c917c */       \
  0x7a4204c6,                                      \
  0xe45a,                                          \
  0x4c37,                                          \
  {0x8e, 0xbb, 0x67, 0x09, 0xa2, 0x2c, 0x91, 0x7c} \
}

#endif  
