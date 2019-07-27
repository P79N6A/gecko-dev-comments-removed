




#ifndef nsView_h__
#define nsView_h__

#include "nsCoord.h"
#include "nsRect.h"
#include "nsPoint.h"
#include "nsRegion.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "nsWidgetInitData.h" 
#include "nsIWidgetListener.h"
#include "mozilla/EventForwards.h"

class nsViewManager;
class nsIWidget;
class nsIFrame;





enum nsViewVisibility {
  nsViewVisibility_kHide = 0,
  nsViewVisibility_kShow = 1
};




#define NS_VIEW_FLAG_AUTO_ZINDEX          0x0004


#define NS_VIEW_FLAG_FLOATING             0x0008
















class nsView final : public nsIWidgetListener
{
public:
  friend class nsViewManager;

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  





  nsViewManager* GetViewManager() const { return mViewManager; }

  




  static nsView* GetViewFor(nsIWidget* aWidget);

  









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

  

















  nsPoint GetOffsetTo(const nsView* aOther) const;

  






  nsPoint GetOffsetToWidget(nsIWidget* aWidget) const;

  




  nsPoint ConvertFromParentCoords(nsPoint aPt) const;

  



  nsViewVisibility GetVisibility() const { return mVis; }

  







  bool GetFloating() const { return (mVFlags & NS_VIEW_FLAG_FLOATING) != 0; }

  



  nsView* GetParent() const { return mParent; }

  



  nsView* GetFirstChild() const { return mFirstChild; }

  



  nsView* GetNextSibling() const { return mNextSibling; }

  


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
    mForcedRepaint = aForceRepaint; 
  }

  




  void AttachWidgetEventHandler(nsIWidget* aWidget);
  


  void DetachWidgetEventHandler(nsIWidget* aWidget);

#ifdef DEBUG
  





  virtual void List(FILE* out, int32_t aIndent = 0) const;
#endif 

  


  bool IsRoot() const;

  nsIntRect CalcWidgetBounds(nsWindowType aType);

  
  
  
  
  nsPoint ViewToWidgetOffset() const { return mViewToWidgetOffset; }

  





  void SetPosition(nscoord aX, nscoord aY);

  








  void SetZIndex(bool aAuto, int32_t aZIndex);
  bool GetZIndexIsAuto() const { return (mVFlags & NS_VIEW_FLAG_AUTO_ZINDEX) != 0; }
  int32_t GetZIndex() const { return mZIndex; }

  void SetParent(nsView *aParent) { mParent = aParent; }
  void SetNextSibling(nsView *aSibling)
  {
    NS_ASSERTION(aSibling != this, "Can't be our own sibling!");
    mNextSibling = aSibling;
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

  
  virtual nsIPresShell* GetPresShell() override;
  virtual nsView* GetView() override { return this; }
  virtual bool WindowMoved(nsIWidget* aWidget, int32_t x, int32_t y) override;
  virtual bool WindowResized(nsIWidget* aWidget, int32_t aWidth, int32_t aHeight) override;
  virtual bool RequestWindowClose(nsIWidget* aWidget) override;
  virtual void WillPaintWindow(nsIWidget* aWidget) override;
  virtual bool PaintWindow(nsIWidget* aWidget, nsIntRegion aRegion) override;
  virtual void DidPaintWindow() override;
  virtual void DidCompositeWindow() override;
  virtual void RequestRepaint() override;
  virtual nsEventStatus HandleEvent(mozilla::WidgetGUIEvent* aEvent,
                                    bool aUseAttachedEvents) override;

  virtual ~nsView();

  nsPoint GetOffsetTo(const nsView* aOther, const int32_t aAPD) const;
  nsIWidget* GetNearestWidget(nsPoint* aOffset, const int32_t aAPD) const;

private:
  explicit nsView(nsViewManager* aViewManager = nullptr,
                  nsViewVisibility aVisibility = nsViewVisibility_kShow);

  bool ForcedRepaint() { return mForcedRepaint; }

  
  
  void DoResetWidgetBounds(bool aMoveOnly, bool aInvalidateChangedSize);
  void InitializeWindow(bool aEnableDragDrop, bool aResetVisibility);

  bool IsEffectivelyVisible();

  





  void SetDimensions(const nsRect &aRect, bool aPaint = true,
                     bool aResizeWidget = true);

  




  void SetVisibility(nsViewVisibility visibility);

  







  void SetFloating(bool aFloatingView);

  
  
  void DropMouseGrabbing();

  
  nsRect GetBoundsInParentUnits() const;

  bool HasNonEmptyDirtyRegion() {
    return mDirtyRegion && !mDirtyRegion->IsEmpty();
  }

  void InsertChild(nsView *aChild, nsView *aSibling);
  void RemoveChild(nsView *aChild);

  void ResetWidgetBounds(bool aRecurse, bool aForceSync);
  void AssertNoWindow();

  void NotifyEffectiveVisibilityChanged(bool aEffectivelyVisible);

  
  
  
  
  void InvalidateHierarchy(nsViewManager *aViewManagerParent);

  nsViewManager    *mViewManager;
  nsView           *mParent;
  nsCOMPtr<nsIWidget> mWindow;
  nsView           *mNextSibling;
  nsView           *mFirstChild;
  nsIFrame         *mFrame;
  nsRegion         *mDirtyRegion;
  int32_t           mZIndex;
  nsViewVisibility  mVis;
  
  nscoord           mPosX, mPosY;
  
  nsRect            mDimBounds;
  
  nsPoint           mViewToWidgetOffset;
  uint32_t          mVFlags;
  bool              mWidgetIsTopLevel;
  bool              mForcedRepaint;
};

#endif
