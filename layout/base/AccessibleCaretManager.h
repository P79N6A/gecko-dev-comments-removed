





#ifndef AccessibleCaretManager_h
#define AccessibleCaretManager_h

#include "nsCOMPtr.h"
#include "nsCoord.h"
#include "nsIFrame.h"
#include "nsISelectionListener.h"
#include "nsRefPtr.h"
#include "nsWeakReference.h"
#include "mozilla/dom/CaretStateChangedEvent.h"
#include "mozilla/EventForwards.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/WeakPtr.h"

class nsFrameSelection;
class nsIContent;
class nsIPresShell;
struct nsPoint;

namespace mozilla {

namespace dom {
class Selection;
} 

class AccessibleCaret;











class AccessibleCaretManager
{
public:
  explicit AccessibleCaretManager(nsIPresShell* aPresShell);
  virtual ~AccessibleCaretManager();

  
  

  
  
  virtual nsresult PressCaret(const nsPoint& aPoint);

  
  
  virtual nsresult DragCaret(const nsPoint& aPoint);

  
  
  virtual nsresult ReleaseCaret();

  
  virtual nsresult TapCaret(const nsPoint& aPoint);

  
  
  virtual nsresult SelectWordOrShortcut(const nsPoint& aPoint);

  
  virtual void OnScrollStart();

  
  virtual void OnScrollEnd();

  
  virtual void OnScrolling();

  
  virtual void OnScrollPositionChanged();

  
  virtual void OnReflow();

  
  virtual void OnBlur();

  
  virtual nsresult OnSelectionChanged(nsIDOMDocument* aDoc,
                                      nsISelection* aSel,
                                      int16_t aReason);
  
  virtual void OnKeyboardEvent();

protected:
  
  enum class CaretMode : uint8_t {
    
    None,

    
    Cursor,

    
    Selection
  };
  CaretMode GetCaretMode() const;

  void UpdateCarets();
  void HideCarets();

  void UpdateCaretsForCursorMode();
  void UpdateCaretsForSelectionMode();
  void UpdateCaretsForTilt();

  bool ChangeFocus(nsIFrame* aFrame) const;
  nsresult SelectWord(nsIFrame* aFrame, const nsPoint& aPoint) const;
  void SetSelectionDragState(bool aState) const;
  void SetSelectionDirection(nsDirection aDir) const;

  
  
  
  nsIFrame* FindFirstNodeWithFrame(bool aBackward, int32_t* aOutOffset) const;

  nsresult DragCaretInternal(const nsPoint& aPoint);
  nsPoint AdjustDragBoundary(const nsPoint& aPoint) const;
  void ClearMaintainedSelection() const;

  dom::Selection* GetSelection() const;
  already_AddRefed<nsFrameSelection> GetFrameSelection() const;
  nsIContent* GetFocusedContent() const;

  
  
  void DispatchCaretStateChangedEvent(dom::CaretChangedReason aReason) const;

  
  
  
  
  bool CompareRangeWithContentOffset(nsIFrame::ContentOffsets& aOffsets);

  
  
  uint32_t CaretTimeoutMs() const;
  void LaunchCaretTimeoutTimer();
  void CancelCaretTimeoutTimer();

  
  nscoord mOffsetYToCaretLogicalPosition = NS_UNCONSTRAINEDSIZE;

  
  
  
  nsIPresShell* MOZ_NON_OWNING_REF const mPresShell = nullptr;

  
  
  UniquePtr<AccessibleCaret> mFirstCaret;

  
  
  UniquePtr<AccessibleCaret> mSecondCaret;

  
  AccessibleCaret* mActiveCaret = nullptr;

  nsCOMPtr<nsITimer> mCaretTimeoutTimer;
  CaretMode mCaretMode = CaretMode::None;

  static const int32_t kAutoScrollTimerDelay = 30;
};

} 

#endif
