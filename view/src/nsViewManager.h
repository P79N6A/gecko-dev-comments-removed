




#ifndef nsViewManager_h___
#define nsViewManager_h___
#include "nsCOMPtr.h"
#include "nsIViewManager.h"
#include "nsCRT.h"
#include "nsITimer.h"
#include "prtime.h"
#include "prinrval.h"
#include "nsVoidArray.h"
#include "nsThreadUtils.h"
#include "nsIPresShell.h"
#include "nsDeviceContext.h"


























class nsViewManager : public nsIViewManager {
public:
  nsViewManager();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_DECL_ISUPPORTS

  NS_IMETHOD  Init(nsDeviceContext* aContext);

  NS_IMETHOD_(nsIView*) CreateView(const nsRect& aBounds,
                                   const nsIView* aParent,
                                   nsViewVisibility aVisibilityFlag = nsViewVisibility_kShow);

  NS_IMETHOD_(nsIView*) GetRootView();
  NS_IMETHOD  SetRootView(nsIView *aView);

  NS_IMETHOD  GetWindowDimensions(nscoord *width, nscoord *height);
  NS_IMETHOD  SetWindowDimensions(nscoord width, nscoord height);
  NS_IMETHOD  FlushDelayedResize(bool aDoReflow);

  NS_IMETHOD  InvalidateView(nsIView *aView);
  NS_IMETHOD  InvalidateViewNoSuppression(nsIView *aView, const nsRect &aRect);
  NS_IMETHOD  InvalidateAllViews();

  NS_IMETHOD  DispatchEvent(nsGUIEvent *aEvent,
      nsIView* aTargetView, nsEventStatus* aStatus);

  NS_IMETHOD  InsertChild(nsIView *parent, nsIView *child, nsIView *sibling,
                          bool above);

  NS_IMETHOD  InsertChild(nsIView *parent, nsIView *child,
                          int32_t zindex);

  NS_IMETHOD  RemoveChild(nsIView *parent);

  NS_IMETHOD  MoveViewTo(nsIView *aView, nscoord aX, nscoord aY);

  NS_IMETHOD  ResizeView(nsIView *aView, const nsRect &aRect, bool aRepaintExposedAreaOnly = false);

  NS_IMETHOD  SetViewFloating(nsIView *aView, bool aFloating);

  NS_IMETHOD  SetViewVisibility(nsIView *aView, nsViewVisibility aVisible);

  NS_IMETHOD  SetViewZIndex(nsIView *aView, bool aAuto, int32_t aZIndex, bool aTopMost=false);

  virtual void SetPresShell(nsIPresShell *aPresShell) { mPresShell = aPresShell; }
  virtual nsIPresShell* GetPresShell() { return mPresShell; }

  NS_IMETHOD  GetDeviceContext(nsDeviceContext *&aContext);

  virtual nsIViewManager* IncrementDisableRefreshCount();
  virtual void DecrementDisableRefreshCount();

  NS_IMETHOD GetRootWidget(nsIWidget **aWidget);
 
  NS_IMETHOD IsPainting(bool& aIsPainting);
  NS_IMETHOD GetLastUserEventTime(uint32_t& aTime);
  static uint32_t gLastUserEventTime;

  
  void InvalidateHierarchy();

  virtual void ProcessPendingUpdates();
  virtual void UpdateWidgetGeometry();

protected:
  virtual ~nsViewManager();

private:

  void FlushPendingInvalidates();
  void ProcessPendingUpdatesForView(nsIView *aView,
                                    bool aFlushDirtyRegion = true);
  void FlushDirtyRegionToWidget(nsIView* aView);
  


  void CallWillPaintOnObservers(bool aWillSendDidPaint);
  void ReparentChildWidgets(nsIView* aView, nsIWidget *aNewWidget);
  void ReparentWidgets(nsIView* aView, nsIView *aParent);
  void InvalidateWidgetArea(nsIView *aWidgetView, const nsRegion &aDamagedRegion);

  void InvalidateViews(nsIView *aView);

  
  void Refresh(nsIView *aView, const nsIntRegion& aRegion, bool aWillSendDidPaint);

  void InvalidateRectDifference(nsIView *aView, const nsRect& aRect, const nsRect& aCutOut);
  void InvalidateHorizontalBandDifference(nsIView *aView, const nsRect& aRect, const nsRect& aCutOut,
                                          nscoord aY1, nscoord aY2, bool aInCutOut);

  

  bool IsViewInserted(nsIView *aView);

  




  nsIntRect ViewToWidget(nsIView *aView, const nsRect &aRect) const;

  void DoSetWindowDimensions(nscoord aWidth, nscoord aHeight);

  bool IsPainting() const {
    return RootViewManager()->mPainting;
  }

  void SetPainting(bool aPainting) {
    RootViewManager()->mPainting = aPainting;
  }

  nsresult InvalidateView(nsIView *aView, const nsRect &aRect);

public: 
  nsIView* GetRootViewImpl() const { return mRootView; }
  nsViewManager* RootViewManager() const { return mRootViewManager; }
  bool IsRootVM() const { return this == RootViewManager(); }

  
  
  
  bool IsPaintingAllowed() { return RootViewManager()->mRefreshDisableCount == 0; }

  void WillPaintWindow(nsIWidget* aWidget, bool aWillSendDidPaint);
  bool PaintWindow(nsIWidget* aWidget, nsIntRegion aRegion,
                   uint32_t aFlags);
  void DidPaintWindow();

  
  
  void PostPendingUpdate();

  uint32_t AppUnitsPerDevPixel() const
  {
    return mContext->AppUnitsPerDevPixel();
  }

private:
  nsRefPtr<nsDeviceContext> mContext;
  nsIPresShell   *mPresShell;

  
  
  nsSize            mDelayedResize;

  nsIView           *mRootView;
  
  
  nsViewManager     *mRootViewManager;

  
  
  
  
  int32_t           mRefreshDisableCount;
  
  bool              mPainting;
  bool              mRecursiveRefreshPending;
  bool              mHasPendingWidgetGeometryChanges;
  bool              mInScroll;

  
  static int32_t           mVMCount;        

  
  static nsVoidArray       *gViewManagers;
};

#endif 
