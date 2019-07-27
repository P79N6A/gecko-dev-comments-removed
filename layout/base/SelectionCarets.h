





#ifndef SelectionCarets_h__
#define SelectionCarets_h__

#include "nsDirection.h"
#include "nsIReflowObserver.h"
#include "nsIScrollObserver.h"
#include "nsISelectionListener.h"
#include "nsWeakPtr.h"
#include "nsWeakReference.h"
#include "Units.h"
#include "mozilla/dom/SelectionStateChangedEvent.h"
#include "mozilla/EventForwards.h"
#include "mozilla/WeakPtr.h"

class nsDocShell;
class nsFrameSelection;
class nsIContent;
class nsIPresShell;
class nsITimer;

namespace mozilla {

namespace dom {
class Selection;
}






















class SelectionCarets final : public nsIReflowObserver,
                              public nsISelectionListener,
                              public nsIScrollObserver,
                              public nsSupportsWeakReference
{
public:
  


  enum DragMode {
    NONE,
    START_FRAME,
    END_FRAME
  };

  explicit SelectionCarets(nsIPresShell *aPresShell);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREFLOWOBSERVER
  NS_DECL_NSISELECTIONLISTENER

  
  void NotifyBlur(bool aIsLeavingDocument);

  
  virtual void ScrollPositionChanged() override;

  
  virtual void AsyncPanZoomStarted() override;
  virtual void AsyncPanZoomStopped() override;

  void Init();
  void Terminate();

  nsEventStatus HandleEvent(WidgetEvent* aEvent);

  bool GetVisibility() const
  {
    return mVisible;
  }

  



  static int32_t SelectionCaretsInflateSize()
  {
    return sSelectionCaretsInflateSize;
  }

private:
  virtual ~SelectionCarets();

  SelectionCarets() = delete;

  


  void SetVisibility(bool aVisible);

  


  void UpdateSelectionCarets();

  



  nsresult SelectWord();

  


  nsEventStatus DragSelection(const nsPoint &movePoint);

  



  nscoord GetCaretYCenterPosition();

  



  void SetSelectionDragState(bool aState);

  void SetSelectionDirection(nsDirection aDir);

  



  void SetStartFramePos(const nsRect& aCaretRect);

  



  void SetEndFramePos(const nsRect& aCaretRect);

  






  bool IsOnStartFrameInner(const nsPoint& aPosition);
  bool IsOnEndFrameInner(const nsPoint& aPosition);

  



  nsRect GetStartFrameRect();
  nsRect GetEndFrameRect();

  



  nsRect GetStartFrameRectInner();
  nsRect GetEndFrameRectInner();

  





  void SetStartFrameVisibility(bool aVisible);

  


  void SetEndFrameVisibility(bool aVisible);

  


  void SetTilted(bool aIsTilt);

  
  dom::Selection* GetSelection();
  already_AddRefed<nsFrameSelection> GetFrameSelection();
  nsIContent* GetFocusedContent();
  void DispatchSelectionStateChangedEvent(dom::Selection* aSelection,
                                          dom::SelectionState aState);
  void DispatchSelectionStateChangedEvent(dom::Selection* aSelection,
                                          const dom::Sequence<dom::SelectionState>& aStates);
  void DispatchCustomEvent(const nsAString& aEvent);

  


  void LaunchLongTapDetector();
  void CancelLongTapDetector();
  static void FireLongTap(nsITimer* aTimer, void* aSelectionCarets);

  void LaunchScrollEndDetector();
  void CancelScrollEndDetector();
  static void FireScrollEnd(nsITimer* aTimer, void* aSelectionCarets);

  nsIPresShell* mPresShell;
  WeakPtr<nsDocShell> mDocShell;

  
  
  
  nsCOMPtr<nsITimer> mLongTapDetectorTimer;

  
  
  
  
  nsCOMPtr<nsITimer> mScrollEndDetectorTimer;

  
  
  nsPoint mDownPoint;

  
  int32_t mActiveTouchId;

  nscoord mCaretCenterToDownPointOffsetY;

  
  
  
  
  
  nscoord mDragUpYBoundary;
  
  
  
  
  
  nscoord mDragDownYBoundary;

  DragMode mDragMode;

  
  bool mUseAsyncPanZoom;
  
  bool mInAsyncPanZoomGesture;

  bool mEndCaretVisible;
  bool mStartCaretVisible;
  bool mSelectionVisibleInScrollFrames;
  bool mVisible;

  
  static int32_t sSelectionCaretsInflateSize;
  static bool sSelectionCaretDetectsLongTap;
};
} 

#endif
