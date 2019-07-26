




#ifndef ScrollbarActivity_h___
#define ScrollbarActivity_h___

#include "nsCOMPtr.h"
#include "nsIDOMEventListener.h"
#include "mozilla/TimeStamp.h"
#include "nsRefreshDriver.h"

class nsIContent;
class nsIScrollbarOwner;
class nsITimer;
class nsIAtom;

namespace mozilla {
namespace layout {




































class ScrollbarActivity : public nsIDOMEventListener,
                          public nsARefreshObserver {
public:
  ScrollbarActivity(nsIScrollbarOwner* aScrollableFrame)
   : mScrollableFrame(aScrollableFrame)
   , mNestedActivityCounter(0)
   , mIsActive(false)
   , mIsFading(false)
   , mListeningForEvents(false)
   , mHScrollbarHovered(false)
   , mVScrollbarHovered(false)
  {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER

  virtual ~ScrollbarActivity() {}

  void Destroy();

  void ActivityOccurred();
  void ActivityStarted();
  void ActivityStopped();

  virtual void WillRefresh(TimeStamp aTime);

  static void FadeBeginTimerFired(nsITimer* aTimer, void* aSelf) {
    reinterpret_cast<ScrollbarActivity*>(aSelf)->BeginFade();
  }

  static const uint32_t kScrollbarFadeBeginDelay = 450; 
  static const uint32_t kScrollbarFadeDuration = 200; 

protected:

  bool IsActivityOngoing()
  { return mNestedActivityCounter > 0; }
  bool IsStillFading(TimeStamp aTime);

  void HandleEventForScrollbar(const nsAString& aType,
                               nsIContent* aTarget,
                               nsIContent* aScrollbar,
                               bool* aStoredHoverState);

  void SetIsActive(bool aNewActive);
  void SetIsFading(bool aNewFading);

  void BeginFade();
  void EndFade();

  void StartFadeBeginTimer();
  void CancelFadeBeginTimer();
  void StartListeningForEvents();
  void StartListeningForEventsOnScrollbar(nsIDOMEventTarget* aScrollbar);
  void StopListeningForEvents();
  void StopListeningForEventsOnScrollbar(nsIDOMEventTarget* aScrollbar);
  void RegisterWithRefreshDriver();
  void UnregisterFromRefreshDriver();

  void UpdateOpacity(TimeStamp aTime);
  void HoveredScrollbar(nsIContent* aScrollbar);

  nsRefreshDriver* GetRefreshDriver();
  nsIContent* GetScrollbarContent(bool aVertical);
  nsIContent* GetHorizontalScrollbar() { return GetScrollbarContent(false); }
  nsIContent* GetVerticalScrollbar() { return GetScrollbarContent(true); }

  static const TimeDuration FadeDuration() {
    return TimeDuration::FromMilliseconds(kScrollbarFadeDuration);
  }

  nsIScrollbarOwner* mScrollableFrame;
  TimeStamp mFadeBeginTime;
  nsCOMPtr<nsITimer> mFadeBeginTimer;
  nsCOMPtr<nsIDOMEventTarget> mHorizontalScrollbar; 
  nsCOMPtr<nsIDOMEventTarget> mVerticalScrollbar;   
  int mNestedActivityCounter;
  bool mIsActive;
  bool mIsFading;
  bool mListeningForEvents;
  bool mHScrollbarHovered;
  bool mVScrollbarHovered;
};

} 
} 

#endif 
