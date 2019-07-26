




#ifndef nsView_h___
#define nsView_h___

#include "nsIView.h"
#include "nsRegion.h"
#include "nsRect.h"
#include "nsCRT.h"
#include "nsIFactory.h"
#include "nsEvent.h"
#include "nsIWidgetListener.h"
#include <stdio.h>



class nsIViewManager;
class nsViewManager;

class nsView : public nsIView,
               public nsIWidgetListener
{
public:
  nsView(nsViewManager* aViewManager = nullptr,
         nsViewVisibility aVisibility = nsViewVisibility_kShow);

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_IMETHOD  QueryInterface(const nsIID& aIID, void** aInstancePtr);

  





  virtual void SetPosition(nscoord aX, nscoord aY);
  





  virtual void SetDimensions(const nsRect &aRect, bool aPaint = true,
                             bool aResizeWidget = true);

  




  NS_IMETHOD  SetVisibility(nsViewVisibility visibility);

  








  void SetZIndex(bool aAuto, int32_t aZIndex, bool aTopMost);

  







  NS_IMETHOD  SetFloating(bool aFloatingView);

  
  static nsView* GetViewFor(nsIWidget* aWidget) {
    return static_cast<nsView*>(nsIView::GetViewFor(aWidget));
  }

  
  
  void DropMouseGrabbing();

public:
  
  nsresult CreateWidget(nsWidgetInitData *aWidgetInitData,
                        bool aEnableDragDrop,
                        bool aResetVisibility);

  
  nsresult CreateWidgetForParent(nsIWidget* aParentWidget,
                                 nsWidgetInitData *aWidgetInitData,
                                 bool aEnableDragDrop,
                                 bool aResetVisibility);

  
  nsresult CreateWidgetForPopup(nsWidgetInitData *aWidgetInitData,
                                nsIWidget* aParentWidget,
                                bool aEnableDragDrop,
                                bool aResetVisibility);

  
  void DestroyWidget();

  
  
  
  nsView* GetFirstChild() const { return mFirstChild; }
  nsView* GetNextSibling() const { return mNextSibling; }
  nsView* GetParent() const { return mParent; }
  nsViewManager* GetViewManager() const { return mViewManager; }
  
  int32_t GetZIndex() const { return mZIndex; }
  bool GetZIndexIsAuto() const { return (mVFlags & NS_VIEW_FLAG_AUTO_ZINDEX) != 0; }
  
  nsRect GetBoundsInParentUnits() const;

  
  

  bool HasNonEmptyDirtyRegion() {
    return mDirtyRegion && !mDirtyRegion->IsEmpty();
  }
  nsRegion* GetDirtyRegion() {
    if (!mDirtyRegion) {
      NS_ASSERTION(!mParent || GetFloating(),
                   "Only display roots should have dirty regions");
      mDirtyRegion = new nsRegion();
      NS_ASSERTION(mDirtyRegion, "Out of memory!");
    }
    return mDirtyRegion;
  }

  void InsertChild(nsView *aChild, nsView *aSibling);
  void RemoveChild(nsView *aChild);

  void SetParent(nsView *aParent) { mParent = aParent; }
  void SetNextSibling(nsView *aSibling) { mNextSibling = aSibling; }

  uint32_t GetViewFlags() const { return mVFlags; }
  void SetViewFlags(uint32_t aFlags) { mVFlags = aFlags; }

  void SetTopMost(bool aTopMost) { aTopMost ? mVFlags |= NS_VIEW_FLAG_TOPMOST : mVFlags &= ~NS_VIEW_FLAG_TOPMOST; }
  bool IsTopMost() { return((mVFlags & NS_VIEW_FLAG_TOPMOST) != 0); }

  void ResetWidgetBounds(bool aRecurse, bool aForceSync);
  void AssertNoWindow();

  void NotifyEffectiveVisibilityChanged(bool aEffectivelyVisible);

  
  
  
  
  void InvalidateHierarchy(nsViewManager *aViewManagerParent);

  
  virtual nsIPresShell* GetPresShell();
  virtual nsIView* GetView() { return this; }
  bool WindowMoved(nsIWidget* aWidget, int32_t x, int32_t y);
  bool WindowResized(nsIWidget* aWidget, int32_t aWidth, int32_t aHeight);
  bool RequestWindowClose(nsIWidget* aWidget);
  void WillPaintWindow(nsIWidget* aWidget, bool aWillSendDidPaint);
  bool PaintWindow(nsIWidget* aWidget, nsIntRegion aRegion, bool aSentDidPaint, bool aWillSendDidPaint);
  void DidPaintWindow();
  nsEventStatus HandleEvent(nsGUIEvent* aEvent, bool aUseAttachedEvents);

  virtual ~nsView();

  nsPoint GetOffsetTo(const nsView* aOther) const;
  nsIWidget* GetNearestWidget(nsPoint* aOffset) const;
  nsPoint GetOffsetTo(const nsView* aOther, const int32_t aAPD) const;
  nsIWidget* GetNearestWidget(nsPoint* aOffset, const int32_t aAPD) const;

protected:
  
  
  void DoResetWidgetBounds(bool aMoveOnly, bool aInvalidateChangedSize);

  nsRegion*    mDirtyRegion;

private:
  void InitializeWindow(bool aEnableDragDrop, bool aResetVisibility);
};

#endif
