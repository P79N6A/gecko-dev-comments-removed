




#ifndef ScrollbarActivity_h___
#define ScrollbarActivity_h___

#include "mozilla/Attributes.h"
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

  virtual ~ScrollbarActivity() {}

  void Destroy();

  void ActivityOccurred();
  void ActivityStarted();
  void ActivityStopped();

  virtual void WillRefresh(TimeStamp aTime) MOZ_OVERRIDE;

  static void FadeBeginTimerFired(nsITimer* aTimer, void* aSelf) {
    reinterpret_cast<ScrollbarActivity*>(aSelf)->BeginFade();
  }

protected:

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

  nsIScrollbarOwner* mScrollableFrame;
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
