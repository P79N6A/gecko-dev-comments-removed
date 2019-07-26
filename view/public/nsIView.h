




#ifndef nsIView_h___
#define nsIView_h___

#include "nsISupports.h"
#include "nsCoord.h"
#include "nsRect.h"
#include "nsPoint.h"
#include "nsNativeWidget.h"
#include "nsIWidget.h"
#include "nsWidgetInitData.h"
#include "nsRegion.h"
#include "nsCRT.h"
#include "nsIFactory.h"
#include "nsEvent.h"
#include "nsIWidgetListener.h"
#include <stdio.h>

class nsIViewManager;
class nsViewManager;
class nsIWidget;
class nsIFrame;





enum nsViewVisibility {
  nsViewVisibility_kHide = 0,
  nsViewVisibility_kShow = 1
};

#define NS_IVIEW_IID    \
{ 0x7f979fcc, 0xa15a, 0x4f8a, \
  { 0x8b, 0x68, 0xa4, 0x16, 0xa1, 0x87, 0xad, 0xdc } }




#define NS_VIEW_FLAG_AUTO_ZINDEX          0x0004


#define NS_VIEW_FLAG_FLOATING             0x0008




#define NS_VIEW_FLAG_TOPMOST              0x0010
















class nsIView MOZ_FINAL : public nsIWidgetListener
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IVIEW_IID)

  nsIView(nsViewManager* aViewManager = nullptr,
          nsViewVisibility aVisibility = nsViewVisibility_kShow);

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  




  static nsIView* GetViewFor(nsIWidget* aWidget);

  





  nsIViewManager* GetViewManager() const
  { return reinterpret_cast<nsIViewManager*>(mViewManager); }

  









  void Destroy();

  







  nsPoint GetPosition() const {
    NS_ASSERTION(!IsRoot() || (mPosX == 0 && mPosY == 0),
                 "root views should always have explicit position of (0,0)");
    return nsPoint(mPosX, mPosY);
  }

  







  nsRect GetBounds() const { return mDimBounds; }

  



  nsRect GetDimensions() const {
    nsRect r = mDimBounds; r.MoveBy(-mPosX, -mPosY); return r;
  }

  

















  nsPoint GetOffsetTo(const nsIView* aOther) const;

  






  nsPoint GetOffsetToWidget(nsIWidget* aWidget) const;

  




  nsPoint ConvertFromParentCoords(nsPoint aPt) const;

  



  nsViewVisibility GetVisibility() const { return mVis; }

  







  bool GetFloating() const { return (mVFlags & NS_VIEW_FLAG_FLOATING) != 0; }

  



  nsIView* GetParent() const { return mParent; }

  



  nsIView* GetFirstChild() const { return mFirstChild; }

  



  nsIView* GetNextSibling() const { return mNextSibling; }

  


  void SetFrame(nsIFrame* aRootFrame) { mFrame = aRootFrame; }

  


  nsIFrame* GetFrame() const { return mFrame; }

  









  nsIWidget* GetNearestWidget(nsPoint* aOffset) const;

  








  nsresult CreateWidget(nsWidgetInitData *aWidgetInitData = nullptr,
                        bool aEnableDragDrop = true,
                        bool aResetVisibility = true);

  




  nsresult CreateWidgetForParent(nsIWidget* aParentWidget,
                                 nsWidgetInitData *aWidgetInitData = nullptr,
                                 bool aEnableDragDrop = true,
                                 bool aResetVisibility = true);

  






  nsresult CreateWidgetForPopup(nsWidgetInitData *aWidgetInitData,
                                nsIWidget* aParentWidget = nullptr,
                                bool aEnableDragDrop = true,
                                bool aResetVisibility = true);

  




  void DestroyWidget();

  










  nsresult AttachToTopLevelWidget(nsIWidget* aWidget);
  nsresult DetachFromTopLevelWidget();

  



  bool IsAttachedToTopLevel() const { return mWidgetIsTopLevel; }

  






  nsIWidget* GetWidget() const { return mWindow; }

  


  bool HasWidget() const { return mWindow != nullptr; }
  
  void SetForcedRepaint(bool aForceRepaint) { 
    if (!mInAlternatePaint) { 
      mForcedRepaint = aForceRepaint; 
    }
  }
  bool ForcedRepaint() { return mForcedRepaint; }

  




  void AttachWidgetEventHandler(nsIWidget* aWidget);
  


  void DetachWidgetEventHandler(nsIWidget* aWidget);

#ifdef DEBUG
  





  virtual void List(FILE* out, int32_t aIndent = 0) const;
#endif 

  


  bool IsRoot() const;

  nsIntRect CalcWidgetBounds(nsWindowType aType);

  bool IsEffectivelyVisible();

  
  
  
  
  nsPoint ViewToWidgetOffset() const { return mViewToWidgetOffset; }

  NS_IMETHOD  QueryInterface(const nsIID& aIID, void** aInstancePtr);

  





  void SetPosition(nscoord aX, nscoord aY);
  





  void SetDimensions(const nsRect &aRect, bool aPaint = true,
                     bool aResizeWidget = true);

  




  NS_IMETHOD  SetVisibility(nsViewVisibility visibility);

  








  void SetZIndex(bool aAuto, int32_t aZIndex, bool aTopMost);

  







  NS_IMETHOD  SetFloating(bool aFloatingView);

  
  
  void DropMouseGrabbing();

  nsViewManager* GetViewManagerInternal() const { return mViewManager; }
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

  void InsertChild(nsIView *aChild, nsIView *aSibling);
  void RemoveChild(nsIView *aChild);

  void SetParent(nsIView *aParent) { mParent = aParent; }
  void SetNextSibling(nsIView *aSibling)
  {
    NS_ASSERTION(aSibling != this, "Can't be our own sibling!");
    mNextSibling = aSibling;
  }

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
  bool PaintWindow(nsIWidget* aWidget, nsIntRegion aRegion, uint32_t aFlags);
  void DidPaintWindow();
  void RequestRepaint() MOZ_OVERRIDE;
  nsEventStatus HandleEvent(nsGUIEvent* aEvent, bool aUseAttachedEvents);

  virtual ~nsIView();

  nsPoint GetOffsetTo(const nsIView* aOther, const int32_t aAPD) const;
  nsIWidget* GetNearestWidget(nsPoint* aOffset, const int32_t aAPD) const;

protected:
  
  
  void DoResetWidgetBounds(bool aMoveOnly, bool aInvalidateChangedSize);
  void InitializeWindow(bool aEnableDragDrop, bool aResetVisibility);

  nsViewManager     *mViewManager;
  nsIView           *mParent;
  nsIWidget         *mWindow;
  nsIView           *mNextSibling;
  nsIView           *mFirstChild;
  nsIFrame          *mFrame;
  nsRegion          *mDirtyRegion;
  int32_t           mZIndex;
  nsViewVisibility  mVis;
  
  nscoord           mPosX, mPosY;
  
  nsRect            mDimBounds;
  
  nsPoint           mViewToWidgetOffset;
  float             mOpacity;
  uint32_t          mVFlags;
  bool              mWidgetIsTopLevel;
  bool              mForcedRepaint;
  bool              mInAlternatePaint;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIView, NS_IVIEW_IID)

#endif
