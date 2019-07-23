





































#include "nsEventQueue.h"
#include "nsAutoLock.h"
#include "nsAutoPtr.h"
#include "prlog.h"
#include "nsThreadUtils.h"

#ifdef PR_LOGGING
static PRLogModuleInfo *sLog = PR_NewLogModule("nsEventQueue");
#endif
#define LOG(args) PR_LOG(sLog, PR_LOG_DEBUG, args)

nsEventQueue::nsEventQueue()
  : mMonitor(nsAutoMonitor::NewMonitor("xpcom.eventqueue"))
  , mHead(nsnull)
  , mTail(nsnull)
  , mOffsetHead(0)
  , mOffsetTail(0)
{
}

nsEventQueue::~nsEventQueue()
{
  
  
  NS_ASSERTION(IsEmpty(), "Non-empty event queue being destroyed; events being leaked.");

  if (mHead)
    FreePage(mHead);

  if (mMonitor)
    nsAutoMonitor::DestroyMonitor(mMonitor);
}

PRBool
nsEventQueue::GetEvent(PRBool mayWait, nsIRunnable **result)
{
  {
    nsAutoMonitor mon(mMonitor);
    
    while (IsEmpty()) {
      if (!mayWait) {
        if (result)
          *result = nsnull;
        return PR_FALSE;
      }
      LOG(("EVENTQ(%p): wait begin\n", this)); 
      mon.Wait();
      LOG(("EVENTQ(%p): wait end\n", this)); 
    }
    
    if (result) {
      *result = mHead->mEvents[mOffsetHead++];
      
      
      if (mOffsetHead == EVENTS_PER_PAGE) {
        Page *dead = mHead;
        mHead = mHead->mNext;
        FreePage(dead);
        mOffsetHead = 0;
      }
    }
  }

  return PR_TRUE;
}

PRBool
nsEventQueue::PutEvent(nsIRunnable *runnable)
{
  
  nsRefPtr<nsIRunnable> event(runnable);
  PRBool rv = PR_TRUE;
  {
    nsAutoMonitor mon(mMonitor);

    if (!mHead) {
      mHead = NewPage();
      if (!mHead) {
        rv = PR_FALSE;
      } else {
        mTail = mHead;
        mOffsetHead = 0;
        mOffsetTail = 0;
      }
    } else if (mOffsetTail == EVENTS_PER_PAGE) {
      Page *page = NewPage();
      if (!page) {
        rv = PR_FALSE;
      } else {
        mTail->mNext = page;
        mTail = page;
        mOffsetTail = 0;
      }
    }
    if (rv) {
      event.swap(mTail->mEvents[mOffsetTail]);
      ++mOffsetTail;
      LOG(("EVENTQ(%p): notify\n", this)); 
      mon.NotifyAll();
    }
  }
  return rv;
}
