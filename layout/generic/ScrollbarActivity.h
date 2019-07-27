




#ifndef ScrollbarActivity_h___
#define ScrollbarActivity_h___

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsIDOMEventListener.h"
#include "mozilla/TimeStamp.h"
#include "nsRefreshDriver.h"

class nsIContent;
class nsIScrollbarMediator;
class nsITimer;

namespace mozilla {
namespace layout {




































class ScrollbarActivity final : public nsIDOMEventListener,
                                public nsARefreshObserver
{
public:
  explicit ScrollbarActivity(nsIScrollbarMediator* aScrollableFrame)
   : mScrollableFrame(aScrollableFrame)
   , mNestedActivityCounter(0)
   , mIsActive(false)
   , mIsFading(false)
   , mListeningForScrollbarEvents(false)
   , mListeningForScrollAreaEvents(false)
   , mHScrollbarHovered(false)
   , mVScrollbarHovered(false)
   , mDisplayOnMouseMove(false)
   , mScrollbarFadeBeginDelay(0)
   , mScrollbarFadeDuration(0)
  {
    QueryLookAndFeelVals();
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER

  void Destroy();

  void ActivityOccurred();
  void ActivityStarted();
  void ActivityStopped();

  virtual void WillRefresh(TimeStamp aTime) override;

  static void FadeBeginTimerFired(nsITimer* aTimer, void* aSelf) {
    nsRefPtr<ScrollbarActivity> scrollbarActivity(
      reinterpret_cast<ScrollbarActivity*>(aSelf));
    scrollbarActivity->BeginFade();
  }

protected:
  virtual ~ScrollbarActivity() {}

  bool IsActivityOngoing()
  { return mNestedActivityCounter > 0; }
  bool IsStillFading(TimeStamp aTime);
  void QueryLookAndFeelVals();

  void HandleEventForScrollbar(const nsAString& aType,
                               nsIContent* aTarget,
                               nsIContent* aScrollbar,
                               bool* aStoredHoverState);

  void SetIsActive(bool aNewActive);
  bool SetIsFading(bool aNewFading); 

  void BeginFade();
  void EndFade();

  void StartFadeBeginTimer();
  void CancelFadeBeginTimer();

  void StartListeningForScrollbarEvents();
  void StartListeningForScrollAreaEvents();
  void StopListeningForScrollbarEvents();
  void StopListeningForScrollAreaEvents();
  void AddScrollbarEventListeners(nsIDOMEventTarget* aScrollbar);
  void RemoveScrollbarEventListeners(nsIDOMEventTarget* aScrollbar);

  void RegisterWithRefreshDriver();
  void UnregisterFromRefreshDriver();

  bool UpdateOpacity(TimeStamp aTime); 
  void HoveredScrollbar(nsIContent* aScrollbar);

  nsRefreshDriver* GetRefreshDriver();
  nsIContent* GetScrollbarContent(bool aVertical);
  nsIContent* GetHorizontalScrollbar() { return GetScrollbarContent(false); }
  nsIContent* GetVerticalScrollbar() { return GetScrollbarContent(true); }

  const TimeDuration FadeDuration() {
    return TimeDuration::FromMilliseconds(mScrollbarFadeDuration);
  }

  nsIScrollbarMediator* mScrollableFrame;
  TimeStamp mFadeBeginTime;
  nsCOMPtr<nsITimer> mFadeBeginTimer;
  nsCOMPtr<nsIDOMEventTarget> mHorizontalScrollbar; 
  nsCOMPtr<nsIDOMEventTarget> mVerticalScrollbar;   
  int mNestedActivityCounter;
  bool mIsActive;
  bool mIsFading;
  bool mListeningForScrollbarEvents;
  bool mListeningForScrollAreaEvents;
  bool mHScrollbarHovered;
  bool mVScrollbarHovered;

  
  bool mDisplayOnMouseMove;
  int mScrollbarFadeBeginDelay;
  int mScrollbarFadeDuration;
};

} 
} 

#endif 
