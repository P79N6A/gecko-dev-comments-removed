





#include "nsEventQueue.h"
#include "nsAutoPtr.h"
#include "mozilla/Logging.h"
#include "nsThreadUtils.h"
#include "prthread.h"
#include "mozilla/ChaosMode.h"

using namespace mozilla;

static PRLogModuleInfo*
GetLog()
{
  static PRLogModuleInfo* sLog;
  if (!sLog) {
    sLog = PR_NewLogModule("nsEventQueue");
  }
  return sLog;
}
#ifdef LOG
#undef LOG
#endif
#define LOG(args) MOZ_LOG(GetLog(), mozilla::LogLevel::Debug, args)

nsEventQueue::nsEventQueue()
  : mReentrantMonitor("nsEventQueue.mReentrantMonitor")
  , mHead(nullptr)
  , mTail(nullptr)
  , mOffsetHead(0)
  , mOffsetTail(0)
{
}

nsEventQueue::~nsEventQueue()
{
  
  
  NS_ASSERTION(IsEmpty(),
               "Non-empty event queue being destroyed; events being leaked.");

  if (mHead) {
    FreePage(mHead);
  }
}

bool
nsEventQueue::GetEvent(bool aMayWait, nsIRunnable** aResult)
{
  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    while (IsEmpty()) {
      if (!aMayWait) {
        if (aResult) {
          *aResult = nullptr;
        }
        return false;
      }
      LOG(("EVENTQ(%p): wait begin\n", this));
      mon.Wait();
      LOG(("EVENTQ(%p): wait end\n", this));
    }

    if (aResult) {
      *aResult = mHead->mEvents[mOffsetHead++];

      
      if (mOffsetHead == EVENTS_PER_PAGE) {
        Page* dead = mHead;
        mHead = mHead->mNext;
        FreePage(dead);
        mOffsetHead = 0;
      }
    }
  }

  return true;
}

void
nsEventQueue::PutEvent(nsIRunnable* aRunnable)
{
  nsCOMPtr<nsIRunnable> event(aRunnable);
  PutEvent(event.forget());
}

void
nsEventQueue::PutEvent(already_AddRefed<nsIRunnable>&& aRunnable)
{
  
  nsCOMPtr<nsIRunnable> event(aRunnable);

  if (ChaosMode::isActive(ChaosMode::ThreadScheduling)) {
    
    
    if (ChaosMode::randomUint32LessThan(2)) {
      PR_Sleep(PR_INTERVAL_NO_WAIT);
    }
  }

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  if (!mHead) {
    mHead = NewPage();
    MOZ_ASSERT(mHead);

    mTail = mHead;
    mOffsetHead = 0;
    mOffsetTail = 0;
  } else if (mOffsetTail == EVENTS_PER_PAGE) {
    Page* page = NewPage();
    MOZ_ASSERT(page);

    mTail->mNext = page;
    mTail = page;
    mOffsetTail = 0;
  }

  event.swap(mTail->mEvents[mOffsetTail]);
  ++mOffsetTail;
  LOG(("EVENTQ(%p): notify\n", this));
  mon.NotifyAll();
}

size_t
nsEventQueue::Count()
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  
  if (!mHead) {
    return 0;
  }

  












  int count = -mOffsetHead;

  
  for (Page* page = mHead; page != mTail; page = page->mNext) {
    count += EVENTS_PER_PAGE;
  }

  count += mOffsetTail;
  MOZ_ASSERT(count >= 0);

  return count;
}
