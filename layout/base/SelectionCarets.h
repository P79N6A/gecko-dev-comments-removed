





#ifndef SelectionCarets_h__
#define SelectionCarets_h__

#include "nsIReflowObserver.h"
#include "nsIScrollObserver.h"
#include "nsISelectionListener.h"
#include "nsWeakPtr.h"
#include "nsWeakReference.h"
#include "Units.h"
#include "mozilla/dom/SelectionStateChangedEvent.h"
#include "mozilla/EventForwards.h"
#include "mozilla/WeakPtr.h"

class nsCanvasFrame;
class nsDocShell;
class nsFrameSelection;
class nsIContent;
class nsIDocument;
class nsIFrame;
class nsIPresShell;
class nsITimer;
class nsIWidget;
class nsPresContext;

namespace mozilla {

namespace dom {
class Selection;
}






















class SelectionCarets MOZ_FINAL : public nsIReflowObserver,
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

  
  void NotifyBlur();

  
  virtual void ScrollPositionChanged() MOZ_OVERRIDE;

  
  virtual void AsyncPanZoomStarted(const mozilla::CSSIntPoint aScrollPos) MOZ_OVERRIDE;
  virtual void AsyncPanZoomStopped(const mozilla::CSSIntPoint aScrollPos) MOZ_OVERRIDE;

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

  SelectionCarets() MOZ_DELETE;

  


  void SetVisibility(bool aVisible);

  


  void UpdateSelectionCarets();

  



  nsresult SelectWord();

  


  nsEventStatus DragSelection(const nsPoint &movePoint);

  



  nscoord GetCaretYCenterPosition();

  



  void SetSelectionDragState(bool aState);

  void SetSelectionDirection(bool aForward);

  



  void SetStartFramePos(const nsPoint& aPosition);

  



  void SetEndFramePos(const nsPoint& aPosition);

  






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
  nsRect GetSelectionBoundingRect(dom::Selection* aSel);

  


  void LaunchLongTapDetector();
  void CancelLongTapDetector();
  static void FireLongTap(nsITimer* aTimer, void* aSelectionCarets);

  void LaunchScrollEndDetector();
  static void FireScrollEnd(nsITimer* aTimer, void* aSelectionCarets);

  nsIPresShell* mPresShell;
  WeakPtr<nsDocShell> mDocShell;

  
  
  
  nsCOMPtr<nsITimer> mLongTapDetectorTimer;

  
  
  
  
  nsCOMPtr<nsITimer> mScrollEndDetectorTimer;

  
  
  nsPoint mDownPoint;

  
  int32_t mActiveTouchId;

  nscoord mCaretCenterToDownPointOffsetY;
  DragMode mDragMode;

  
  bool mAsyncPanZoomEnabled;

  bool mEndCaretVisible;
  bool mStartCaretVisible;
  bool mSelectionVisibleInScrollFrames;
  bool mVisible;

  
  static int32_t sSelectionCaretsInflateSize;
};
} 

#endif
