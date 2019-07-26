





#ifndef nsEventQueue_h__
#define nsEventQueue_h__

#include <stdlib.h>
#include "mozilla/ReentrantMonitor.h"
#include "nsIRunnable.h"


class nsEventQueue
{
  typedef mozilla::ReentrantMonitor ReentrantMonitor;

public:
  nsEventQueue();
  ~nsEventQueue();

  
  
  
  void PutEvent(nsIRunnable *event);

  
  
  
  
  
  
  bool GetEvent(bool mayWait, nsIRunnable **event);

  
  bool HasPendingEvent() {
    return GetEvent(false, nullptr);
  }

  
  bool GetPendingEvent(nsIRunnable **runnable) {
    return GetEvent(false, runnable);
  }

  
  ReentrantMonitor& GetReentrantMonitor() {
    return mReentrantMonitor;
  }

private:

  bool IsEmpty() {
    return !mHead || (mHead == mTail && mOffsetHead == mOffsetTail);
  }

  enum { EVENTS_PER_PAGE = 255 };

  

  struct Page {
    struct Page *mNext;
    nsIRunnable *mEvents[EVENTS_PER_PAGE];
  };

  static_assert((sizeof(Page) & (sizeof(Page) - 1)) == 0,
                "sizeof(Page) should be a power of two to avoid heap slop.");

  static Page *NewPage() {
    return static_cast<Page *>(moz_xcalloc(1, sizeof(Page)));
  }

  static void FreePage(Page *p) {
    free(p);
  }

  ReentrantMonitor mReentrantMonitor;

  Page *mHead;
  Page *mTail;

  uint16_t mOffsetHead;  
  uint16_t mOffsetTail;  
};

#endif  
