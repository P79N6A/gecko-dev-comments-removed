




#ifndef ScrollbarActivity_h___
#define ScrollbarActivity_h___

#include "nsCOMPtr.h"
#include "mozilla/TimeStamp.h"

class nsIContent;
class nsITimer;
class nsIAtom;
class nsIScrollableFrame;

namespace mozilla {






















class ScrollbarActivity {
public:
  ScrollbarActivity(nsIScrollableFrame* aScrollableFrame)
   : mIsActive(false)
   , mScrollableFrame(aScrollableFrame)
  {}

  void ActivityOccurred();
  void ActivityFinished();
  ~ScrollbarActivity();

protected:
  




  bool mIsActive;

  




  nsIScrollableFrame* mScrollableFrame;

  nsCOMPtr<nsITimer> mActivityFinishedTimer;

  void SetIsActive(bool aNewActive);

  enum { kScrollbarActivityFinishedDelay = 450 }; 
  static void ActivityFinishedTimerFired(nsITimer* aTimer, void* aSelf) {
    reinterpret_cast<ScrollbarActivity*>(aSelf)->ActivityFinished();
  }
  void StartActivityFinishedTimer();
  void CancelActivityFinishedTimer();

  nsIContent* GetScrollbarContent(bool aVertical);
  nsIContent* GetHorizontalScrollbar() {
    return GetScrollbarContent(false);
  }
  nsIContent* GetVerticalScrollbar() {
    return GetScrollbarContent(true);
  }
};

} 

#endif 
