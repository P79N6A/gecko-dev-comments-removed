





#ifndef VideoUtils_h
#define VideoUtils_h

#include "mozilla/ReentrantMonitor.h"
#include "mozilla/CheckedInt.h"

#include "nsRect.h"
#include "nsIThreadManager.h"
#include "nsThreadUtils.h"

using mozilla::CheckedInt64;
using mozilla::CheckedUint64;
using mozilla::CheckedInt32;
using mozilla::CheckedUint32;








namespace mozilla {







 
class NS_STACK_CLASS ReentrantMonitorAutoExit
{
public:
    








    ReentrantMonitorAutoExit(ReentrantMonitor& aReentrantMonitor) :
        mReentrantMonitor(&aReentrantMonitor)
    {
        NS_ASSERTION(mReentrantMonitor, "null monitor");
        mReentrantMonitor->AssertCurrentThreadIn();
        mReentrantMonitor->Exit();
    }
    
    ~ReentrantMonitorAutoExit(void)
    {
        mReentrantMonitor->Enter();
    }
 
private:
    ReentrantMonitorAutoExit();
    ReentrantMonitorAutoExit(const ReentrantMonitorAutoExit&);
    ReentrantMonitorAutoExit& operator =(const ReentrantMonitorAutoExit&);
    static void* operator new(size_t) CPP_THROW_NEW;
    static void operator delete(void*);

    ReentrantMonitor* mReentrantMonitor;
};








class NS_STACK_CLASS ReentrantMonitorConditionallyEnter
{
public:
  ReentrantMonitorConditionallyEnter(bool aEnter,
                                     ReentrantMonitor &aReentrantMonitor) :
    mReentrantMonitor(nullptr)
  {
    MOZ_COUNT_CTOR(ReentrantMonitorConditionallyEnter);
    if (aEnter) {
      mReentrantMonitor = &aReentrantMonitor;
      NS_ASSERTION(mReentrantMonitor, "null monitor");
      mReentrantMonitor->Enter();
    }
  }
  ~ReentrantMonitorConditionallyEnter(void)
  {
    if (mReentrantMonitor) {
      mReentrantMonitor->Exit();
    }
    MOZ_COUNT_DTOR(ReentrantMonitorConditionallyEnter);
  }
private:
  
  ReentrantMonitorConditionallyEnter();
  ReentrantMonitorConditionallyEnter(const ReentrantMonitorConditionallyEnter&);
  ReentrantMonitorConditionallyEnter& operator =(const ReentrantMonitorConditionallyEnter&);
  static void* operator new(size_t) CPP_THROW_NEW;
  static void operator delete(void*);

  ReentrantMonitor* mReentrantMonitor;
};


class ShutdownThreadEvent : public nsRunnable 
{
public:
  ShutdownThreadEvent(nsIThread* aThread) : mThread(aThread) {}
  ~ShutdownThreadEvent() {}
  NS_IMETHOD Run() {
    mThread->Shutdown();
    mThread = nullptr;
    return NS_OK;
  }
private:
  nsCOMPtr<nsIThread> mThread;
};

class MediaResource;
} 

namespace mozilla {
namespace dom {
class TimeRanges;
}
}







void GetEstimatedBufferedTimeRanges(mozilla::MediaResource* aStream,
                                    int64_t aDurationUsecs,
                                    mozilla::dom::TimeRanges* aOutBuffered);





CheckedInt64 FramesToUsecs(int64_t aFrames, uint32_t aRate);





CheckedInt64 UsecsToFrames(int64_t aUsecs, uint32_t aRate);


static const int64_t USECS_PER_S = 1000000;


static const int64_t USECS_PER_MS = 1000;





static const int32_t MAX_VIDEO_WIDTH = 4000;
static const int32_t MAX_VIDEO_HEIGHT = 3000;




void ScaleDisplayByAspectRatio(nsIntSize& aDisplay, float aAspectRatio);


#if (defined(XP_WIN) || defined(XP_MACOSX) || defined(LINUX)) && \
    !defined(MOZ_ASAN)
#define MEDIA_THREAD_STACK_SIZE (128 * 1024)
#else

#define MEDIA_THREAD_STACK_SIZE nsIThreadManager::DEFAULT_STACK_SIZE
#endif

#endif
