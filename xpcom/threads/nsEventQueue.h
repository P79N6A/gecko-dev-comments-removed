





































#ifndef nsEventQueue_h__
#define nsEventQueue_h__

#include <stdlib.h>
#include "prmon.h"
#include "nsIRunnable.h"


class NS_COM nsEventQueue
{
public:
  nsEventQueue();
  ~nsEventQueue();

  
  PRBool IsInitialized() { return mMonitor != nsnull; }

  
  
  
  
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

  
  PRMonitor *Monitor() {
    return mMonitor;
  }

private:

  PRBool IsEmpty() {
    return !mHead || (mHead == mTail && mOffsetHead == mOffsetTail);
  }

  enum { EVENTS_PER_PAGE = 250 };

  

  struct Page; friend struct Page; 
  struct Page {
    struct Page *mNext;
    nsIRunnable *mEvents[EVENTS_PER_PAGE];
  };

  static Page *NewPage() {
    return NS_STATIC_CAST(Page *, calloc(1, sizeof(Page)));
  }

  static void FreePage(Page *p) {
    free(p);
  }

  PRMonitor *mMonitor;

  Page *mHead;
  Page *mTail;

  PRUint16 mOffsetHead;  
  PRUint16 mOffsetTail;  
};

#endif  
