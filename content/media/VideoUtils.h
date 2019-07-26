





#ifndef VideoUtils_h
#define VideoUtils_h

#include "mozilla/Attributes.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/CheckedInt.h"
#include "nsIThread.h"

#if !(defined(XP_WIN) || defined(XP_MACOSX) || defined(LINUX)) || \
    defined(MOZ_ASAN)

#include "nsIThreadManager.h"
#endif
#include "nsThreadUtils.h"
#include "prtime.h"
#include "AudioSampleFormat.h"
#include "mozilla/RefPtr.h"

using mozilla::CheckedInt64;
using mozilla::CheckedUint64;
using mozilla::CheckedInt32;
using mozilla::CheckedUint32;

struct nsIntSize;
struct nsIntRect;








namespace mozilla {








class MOZ_STACK_CLASS ReentrantMonitorConditionallyEnter
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
  NS_IMETHOD Run() MOZ_OVERRIDE {
    mThread->Shutdown();
    mThread = nullptr;
    return NS_OK;
  }
private:
  nsCOMPtr<nsIThread> mThread;
};

template<class T>
class DeleteObjectTask: public nsRunnable {
public:
  DeleteObjectTask(nsAutoPtr<T>& aObject)
    : mObject(aObject)
  {
  }
  NS_IMETHOD Run() {
    NS_ASSERTION(NS_IsMainThread(), "Must be on main thread.");
    mObject = nullptr;
    return NS_OK;
  }
private:
  nsAutoPtr<T> mObject;
};

template<class T>
void DeleteOnMainThread(nsAutoPtr<T>& aObject) {
  NS_DispatchToMainThread(new DeleteObjectTask<T>(aObject));
}

class MediaResource;

namespace dom {
class TimeRanges;
}







void GetEstimatedBufferedTimeRanges(mozilla::MediaResource* aStream,
                                    int64_t aDurationUsecs,
                                    mozilla::dom::TimeRanges* aOutBuffered);





CheckedInt64 FramesToUsecs(int64_t aFrames, uint32_t aRate);





CheckedInt64 UsecsToFrames(int64_t aUsecs, uint32_t aRate);


static const int64_t USECS_PER_S = 1000000;


static const int64_t USECS_PER_MS = 1000;


#define MS_TO_SECONDS(s) ((double)(s) / (PR_MSEC_PER_SEC))



nsresult SecondsToUsecs(double aSeconds, int64_t& aOutUsecs);





static const int32_t MAX_VIDEO_WIDTH = 4000;
static const int32_t MAX_VIDEO_HEIGHT = 3000;




void ScaleDisplayByAspectRatio(nsIntSize& aDisplay, float aAspectRatio);


#if (defined(XP_WIN) || defined(XP_MACOSX) || defined(LINUX)) && \
    !defined(MOZ_ASAN)
#define MEDIA_THREAD_STACK_SIZE (128 * 1024)
#else

#define MEDIA_THREAD_STACK_SIZE nsIThreadManager::DEFAULT_STACK_SIZE
#endif




int DownmixAudioToStereo(mozilla::AudioDataValue* buffer,
                         int channels,
                         uint32_t frames);

bool IsVideoContentType(const nsCString& aContentType);





bool IsValidVideoRegion(const nsIntSize& aFrame, const nsIntRect& aPicture,
                        const nsIntSize& aDisplay);



template<typename T>
class AutoSetOnScopeExit {
public:
  AutoSetOnScopeExit(T& aVar, T aValue)
    : mVar(aVar)
    , mValue(aValue)
  {}
  ~AutoSetOnScopeExit() {
    mVar = mValue;
  }
private:
  T& mVar;
  const T mValue;
};

class SharedThreadPool;



TemporaryRef<SharedThreadPool> GetMediaDecodeThreadPool();

} 

#endif
