



#ifndef NSPRINTERPOSER_H_
#define NSPRINTERPOSER_H_

#ifdef MOZ_ENABLE_PROFILER_SPS

#include "IOInterposer.h"
#include "mozilla/Atomics.h"
#include "mozilla/TimeStamp.h"
#include "prio.h"

namespace mozilla {









class NSPRInterposer MOZ_FINAL : public IOInterposerModule
{
public:
  static IOInterposerModule* GetInstance(IOInterposeObserver* aObserver, 
      IOInterposeObserver::Operation aOpsToInterpose);

  





  static void ClearInstance();

  ~NSPRInterposer();
  void Enable(bool aEnable);

private:
  NSPRInterposer();
  bool Init(IOInterposeObserver* aObserver,
            IOInterposeObserver::Operation aOpsToInterpose);

  static PRInt32 PR_CALLBACK Read(PRFileDesc* aFd, void* aBuf, PRInt32 aAmt);
  static PRInt32 PR_CALLBACK Write(PRFileDesc* aFd, const void* aBuf,
                                   PRInt32 aAmt);
  static PRStatus PR_CALLBACK FSync(PRFileDesc* aFd);
  static StaticAutoPtr<NSPRInterposer> sSingleton;

  friend class NSPRAutoTimer;

  IOInterposeObserver*  mObserver;
  PRIOMethods*          mFileIOMethods;
  Atomic<uint32_t,ReleaseAcquire> mEnabled;
  PRReadFN              mOrigReadFn;
  PRWriteFN             mOrigWriteFn;
  PRFsyncFN             mOrigFSyncFn;
};





class NSPRAutoTimer
{
public:
  NSPRAutoTimer(IOInterposeObserver::Operation aOp)
    :mOp(aOp),
     mShouldObserve(NSPRInterposer::sSingleton->mEnabled && NS_IsMainThread())
  {
    if (mShouldObserve) {
      mStart = mozilla::TimeStamp::Now();
    }
  }

  ~NSPRAutoTimer()
  {
    if (mShouldObserve) {
      double duration = (mozilla::TimeStamp::Now() - mStart).ToMilliseconds();
      NSPRInterposer::sSingleton->mObserver->Observe(mOp, duration,
                                                     sModuleInfo);
    }
  }

private:
  IOInterposeObserver::Operation  mOp;
  bool                            mShouldObserve;
  mozilla::TimeStamp              mStart;
  static const char*              sModuleInfo;
};

} 

#endif 

#endif 

