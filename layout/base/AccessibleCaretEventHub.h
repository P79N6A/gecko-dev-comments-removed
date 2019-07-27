





#ifndef AccessibleCaretEventHub_h
#define AccessibleCaretEventHub_h

#include "mozilla/EventForwards.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/WeakPtr.h"
#include "nsCOMPtr.h"
#include "nsIFrame.h"
#include "nsIReflowObserver.h"
#include "nsIScrollObserver.h"
#include "nsISelectionListener.h"
#include "nsPoint.h"
#include "nsRefPtr.h"
#include "nsWeakReference.h"

class nsDocShell;
class nsIPresShell;
class nsITimer;

namespace mozilla {
class AccessibleCaretManager;
class WidgetKeyboardEvent;
class WidgetMouseEvent;
class WidgetTouchEvent;
class WidgetWheelEvent;



























class AccessibleCaretEventHub : public nsIReflowObserver,
                                public nsIScrollObserver,
                                public nsISelectionListener,
                                public nsSupportsWeakReference
{
public:
  explicit AccessibleCaretEventHub();
  virtual void Init(nsIPresShell* aPresShell);
  virtual void Terminate();

  nsEventStatus HandleEvent(WidgetEvent* aEvent);

  
  void NotifyBlur(bool aIsLeavingDocument);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREFLOWOBSERVER
  NS_DECL_NSISELECTIONLISTENER

  
  virtual void ScrollPositionChanged() override;
  virtual void AsyncPanZoomStarted() override;
  virtual void AsyncPanZoomStopped() override;

  
  class State;
  State* GetState() const;

protected:
  virtual ~AccessibleCaretEventHub();

#define NS_DECL_STATE_CLASS_GETTER(aClassName)                                 \
  class aClassName;                                                            \
  static State* aClassName();

#define NS_IMPL_STATE_CLASS_GETTER(aClassName)                                 \
  AccessibleCaretEventHub::State* AccessibleCaretEventHub::aClassName()        \
  {                                                                            \
    return AccessibleCaretEventHub::aClassName::Singleton();                   \
  }

  
  NS_DECL_STATE_CLASS_GETTER(NoActionState)
  NS_DECL_STATE_CLASS_GETTER(PressCaretState)
  NS_DECL_STATE_CLASS_GETTER(DragCaretState)
  NS_DECL_STATE_CLASS_GETTER(PressNoCaretState)
  NS_DECL_STATE_CLASS_GETTER(ScrollState)
  NS_DECL_STATE_CLASS_GETTER(PostScrollState)
  NS_DECL_STATE_CLASS_GETTER(LongTapState)

  void SetState(State* aState);

  nsEventStatus HandleMouseEvent(WidgetMouseEvent* aEvent);
  nsEventStatus HandleWheelEvent(WidgetWheelEvent* aEvent);
  nsEventStatus HandleTouchEvent(WidgetTouchEvent* aEvent);
  nsEventStatus HandleKeyboardEvent(WidgetKeyboardEvent* aEvent);

  virtual nsPoint GetTouchEventPosition(WidgetTouchEvent* aEvent,
                                        int32_t aIdentifier) const;
  virtual nsPoint GetMouseEventPosition(WidgetMouseEvent* aEvent) const;

  bool MoveDistanceIsLarge(const nsPoint& aPoint) const;

  void LaunchLongTapInjector();
  void CancelLongTapInjector();
  static void FireLongTap(nsITimer* aTimer, void* aAccessibleCaretEventHub);

  void LaunchScrollEndInjector();
  void CancelScrollEndInjector();
  static void FireScrollEnd(nsITimer* aTimer, void* aAccessibleCaretEventHub);

  
  bool mInitialized = false;

  
  bool mUseAsyncPanZoom = false;

  State* mState = NoActionState();

  
  nsIPresShell* MOZ_NON_OWNING_REF mPresShell = nullptr;

  UniquePtr<AccessibleCaretManager> mManager;

  WeakPtr<nsDocShell> mDocShell;

  
  
  nsCOMPtr<nsITimer> mLongTapInjectorTimer;

  
  nsCOMPtr<nsITimer> mScrollEndInjectorTimer;

  
  nsPoint mPressPoint{ NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE };

  
  int32_t mActiveTouchId = kInvalidTouchId;

  static const int32_t kScrollEndTimerDelay = 300;
  static const int32_t kMoveStartToleranceInPixel = 5;
  static const int32_t kInvalidTouchId = -1;
  static const int32_t kDefaultTouchId = 0; 
};






class AccessibleCaretEventHub::State
{
public:
#define NS_IMPL_STATE_UTILITIES(aClassName)                                    \
  virtual const char* Name() const override { return #aClassName; }            \
  static aClassName* Singleton()                                               \
  {                                                                            \
    static aClassName singleton;                                               \
    return &singleton;                                                         \
  }

  virtual const char* Name() const { return ""; }

  virtual nsEventStatus OnPress(AccessibleCaretEventHub* aContext,
                                const nsPoint& aPoint, int32_t aTouchId)
  {
    return nsEventStatus_eIgnore;
  }

  virtual nsEventStatus OnMove(AccessibleCaretEventHub* aContext,
                               const nsPoint& aPoint)
  {
    return nsEventStatus_eIgnore;
  }

  virtual nsEventStatus OnRelease(AccessibleCaretEventHub* aContext)
  {
    return nsEventStatus_eIgnore;
  }

  virtual nsEventStatus OnLongTap(AccessibleCaretEventHub* aContext,
                                  const nsPoint& aPoint)
  {
    return nsEventStatus_eIgnore;
  }

  virtual void OnScrollStart(AccessibleCaretEventHub* aContext) {}
  virtual void OnScrollEnd(AccessibleCaretEventHub* aContext) {}
  virtual void OnScrolling(AccessibleCaretEventHub* aContext) {}
  virtual void OnScrollPositionChanged(AccessibleCaretEventHub* aContext) {}
  virtual void OnBlur(AccessibleCaretEventHub* aContext,
                      bool aIsLeavingDocument) {}
  virtual void OnSelectionChanged(AccessibleCaretEventHub* aContext,
                                  nsIDOMDocument* aDoc, nsISelection* aSel,
                                  int16_t aReason) {}
  virtual void OnReflow(AccessibleCaretEventHub* aContext) {}
  virtual void Enter(AccessibleCaretEventHub* aContext) {}
  virtual void Leave(AccessibleCaretEventHub* aContext) {}

protected:
  explicit State() = default;
  virtual ~State() = default;
  State(const State&) = delete;
  void operator=(const State&) = delete;
};

} 

#endif
