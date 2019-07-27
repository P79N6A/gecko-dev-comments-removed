





#ifndef mozilla_TouchCaret_h__
#define mozilla_TouchCaret_h__

#include "nsISelectionListener.h"
#include "nsIScrollObserver.h"
#include "nsIWeakReferenceUtils.h"
#include "nsITimer.h"
#include "mozilla/EventForwards.h"
#include "mozilla/TouchEvents.h"
#include "Units.h"

class nsIFrame;
class nsIPresShell;

namespace mozilla {







class TouchCaret MOZ_FINAL : public nsISelectionListener
{
public:
  explicit TouchCaret(nsIPresShell* aPresShell);

  NS_DECL_ISUPPORTS
  NS_DECL_NSISELECTIONLISTENER

  void Terminate()
  {
    mPresShell = nullptr;
  }

  





  nsEventStatus HandleEvent(WidgetEvent* aEvent);

  void SyncVisibilityWithCaret();

  void UpdatePositionIfNeeded();

  


  bool GetVisibility() const
  {
    return mVisible;
  }

private:
  
  TouchCaret() MOZ_DELETE;

  ~TouchCaret();

  bool IsDisplayable();

  void UpdatePosition();

  




  void SetVisibility(bool aVisible);

  


  nsIFrame* GetCanvasFrame();

  





  nsRect GetTouchFrameRect();

  







  nsRect GetContentBoundary();

  



  nscoord GetCaretYCenterPosition();

  



  void SetTouchFramePos(const nsPoint& aOrigin);

  void LaunchExpirationTimer();
  void CancelExpirationTimer();
  static void DisableTouchCaretCallback(nsITimer* aTimer, void* aPresShell);

  





  void MoveCaret(const nsPoint& movePoint);

  




  bool IsOnTouchCaret(const nsPoint& aPoint);

  



  nsEventStatus HandleMouseMoveEvent(WidgetMouseEvent* aEvent);
  nsEventStatus HandleMouseUpEvent(WidgetMouseEvent* aEvent);
  nsEventStatus HandleMouseDownEvent(WidgetMouseEvent* aEvent);
  nsEventStatus HandleTouchMoveEvent(WidgetTouchEvent* aEvent);
  nsEventStatus HandleTouchUpEvent(WidgetTouchEvent* aEvent);
  nsEventStatus HandleTouchDownEvent(WidgetTouchEvent* aEvent);

  







  nsPoint GetEventPosition(WidgetTouchEvent* aEvent, int32_t aIdentifier);

  






  void SetSelectionDragState(bool aState);

  





  nsPoint GetEventPosition(WidgetMouseEvent* aEvent);

  


  enum TouchCaretState {
    
    
    
    
    
    
    
    TOUCHCARET_NONE,
    
    
    
    TOUCHCARET_MOUSEDRAG_ACTIVE,
    
    
    
    
    
    
    TOUCHCARET_TOUCHDRAG_ACTIVE,
    
    
    
    
    
    TOUCHCARET_TOUCHDRAG_INACTIVE,
  };

  


  void SetState(TouchCaretState aState);

  


  TouchCaretState mState;

  






  nsTArray<int32_t> mTouchesId;

  


  int32_t mActiveTouchId;

  


  nscoord mCaretCenterToDownPointOffsetY;

  




  static int32_t TouchCaretInflateSize() { return sTouchCaretInflateSize; }

  static int32_t TouchCaretExpirationTime()
  {
    return sTouchCaretExpirationTime;
  }

  nsWeakPtr mPresShell;

  
  bool mVisible;
  
  nsCOMPtr<nsITimer> mTouchCaretExpirationTimer;

  
  static int32_t sTouchCaretInflateSize;
  static int32_t sTouchCaretExpirationTime;

  
  friend class SelectionCarets;
  static const int32_t sAutoScrollTimerDelay = 30;
};
} 
#endif 
