





#ifndef SelectionCarets_h__
#define SelectionCarets_h__

#include "nsIScrollObserver.h"
#include "nsISelectionListener.h"
#include "nsWeakPtr.h"
#include "nsWeakReference.h"
#include "Units.h"
#include "mozilla/EventForwards.h"

class nsCanvasFrame;
class nsIDocument;
class nsIFrame;
class nsIPresShell;
class nsITimer;
class nsIWidget;
class nsPresContext;

namespace mozilla {



















class SelectionCarets MOZ_FINAL : public nsISelectionListener,
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
  NS_DECL_NSISELECTIONLISTENER

  
  virtual void ScrollPositionChanged() MOZ_OVERRIDE;

  void Terminate()
  {
    mPresShell = nullptr;
  }

  nsEventStatus HandleEvent(WidgetEvent* aEvent);

  


  void SetVisibility(bool aVisible);

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

  


  void UpdateSelectionCarets();

  



  nsresult SelectWord();

  


  nsEventStatus DragSelection(const nsPoint &movePoint);

  



  nscoord GetCaretYCenterPosition();

  



  void SetSelectionDragState(bool aState);

  void SetSelectionDirection(bool aForward);

  



  void SetStartFramePos(const nsPoint& aPosition);

  



  void SetEndFramePos(const nsPoint& aPosition);

  






  bool IsOnStartFrame(const nsPoint& aPosition);
  bool IsOnEndFrame(const nsPoint& aPosition);

  



  nsRect GetStartFrameRect();

  



  nsRect GetEndFrameRect();

  





  void SetStartFrameVisibility(bool aVisible);

  


  void SetEndFrameVisibility(bool aVisible);

  


  void SetTilted(bool aIsTilt);

  
  nsIFrame* GetCaretFocusFrame();
  bool GetCaretVisible();
  nsISelection* GetSelection();

  


  void LaunchLongTapDetector();
  void CancelLongTapDetector();
  static void FireLongTap(nsITimer* aTimer, void* aSelectionCarets);

  void LaunchScrollEndDetector();
  static void FireScrollEnd(nsITimer* aTimer, void* aSelectionCarets);

  nsIPresShell* mPresShell;

  
  
  
  nsCOMPtr<nsITimer> mLongTapDetectorTimer;

  
  
  
  
  nsCOMPtr<nsITimer> mScrollEndDetectorTimer;

  
  
  nsPoint mDownPoint;

  
  int32_t mActiveTouchId;

  nscoord mCaretCenterToDownPointOffsetY;
  DragMode mDragMode;
  bool mVisible;
  bool mStartCaretVisible;
  bool mEndCaretVisible;

  
  static int32_t sSelectionCaretsInflateSize;
};
} 

#endif
