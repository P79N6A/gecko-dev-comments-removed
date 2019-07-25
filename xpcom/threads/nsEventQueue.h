





































#ifndef nsEventQueue_h__
#define nsEventQueue_h__

#include <stdlib.h>
#include "mozilla/ReentrantMonitor.h"
#include "nsIRunnable.h"


class NS_COM nsEventQueue
{
  typedef mozilla::ReentrantMonitor ReentrantMonitor;

public:
  nsEventQueue();
  ~nsEventQueue();

  
  
  
  
  PRBool PutEvent(nsIRunnable *event);

  
  
  
  
  
  
  PRBool GetEvent(PRBool mayWait, nsIRunnable **event);

  
  PRBool HasPendingEvent() {
    return GetEvent(PR_FALSE, nsnull);
  }

  
  PRBool GetPendingEvent(nsIRunnable **runnable) {
    return GetEvent(PR_FALSE, runnable);
  }

  
  PRBool WaitPendingEvent(nsIRunnable **runnable) {
    return GetEvent(PR_TRUE, runnable);
  }

  
  ReentrantMonitor& GetReentrantMonitor() {
    return mReentrantMonitor;
  }

private:

  PRBool IsEmpty() {
    return !mHead || (mHead == mTail && mOffsetHead == mOffsetTail);
  }

  enum { EVENTS_PER_PAGE = 250 };

  

  struct Page {
    struct Page *mNext;
    nsIRunnable *mEvents[EVENTS_PER_PAGE];
  };

  static Page *NewPage() {
    return static_cast<Page *>(calloc(1, sizeof(Page)));
  }

  static void FreePage(Page *p) {
    free(p);
  }

  ReentrantMonitor mReentrantMonitor;

  Page *mHead;
  Page *mTail;

  PRUint16 mOffsetHead;  
  PRUint16 mOffsetTail;  
};

#endif  
