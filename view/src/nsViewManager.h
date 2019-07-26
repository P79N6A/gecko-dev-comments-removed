




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

  NS_IMETHOD_(nsView*) CreateView(const nsRect& aBounds,
                                   const nsView* aParent,
                                   nsViewVisibility aVisibilityFlag = nsViewVisibility_kShow);

  NS_IMETHOD_(nsView*) GetRootView();
  NS_IMETHOD  SetRootView(nsView *aView);

  NS_IMETHOD  GetWindowDimensions(nscoord *width, nscoord *height);
  NS_IMETHOD  SetWindowDimensions(nscoord width, nscoord height);
  NS_IMETHOD  FlushDelayedResize(bool aDoReflow);

  NS_IMETHOD  InvalidateView(nsView *aView);
  NS_IMETHOD  InvalidateViewNoSuppression(nsView *aView, const nsRect &aRect);
  NS_IMETHOD  InvalidateAllViews();

  NS_IMETHOD  DispatchEvent(nsGUIEvent *aEvent,
      nsView* aTargetView, nsEventStatus* aStatus);

  NS_IMETHOD  InsertChild(nsView *parent, nsView *child, nsView *sibling,
                          bool above);

  NS_IMETHOD  InsertChild(nsView *parent, nsView *child,
                          int32_t zindex);

  NS_IMETHOD  RemoveChild(nsView *parent);

  NS_IMETHOD  MoveViewTo(nsView *aView, nscoord aX, nscoord aY);

  NS_IMETHOD  ResizeView(nsView *aView, const nsRect &aRect, bool aRepaintExposedAreaOnly = false);

  NS_IMETHOD  SetViewFloating(nsView *aView, bool aFloating);

  NS_IMETHOD  SetViewVisibility(nsView *aView, nsViewVisibility aVisible);

  NS_IMETHOD  SetViewZIndex(nsView *aView, bool aAuto, int32_t aZIndex, bool aTopMost=false);

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
  void ProcessPendingUpdatesForView(nsView *aView,
                                    bool aFlushDirtyRegion = true);
  void FlushDirtyRegionToWidget(nsView* aView);
  


  void CallWillPaintOnObservers(bool aWillSendDidPaint);
  void ReparentChildWidgets(nsView* aView, nsIWidget *aNewWidget);
  void ReparentWidgets(nsView* aView, nsView *aParent);
  void InvalidateWidgetArea(nsView *aWidgetView, const nsRegion &aDamagedRegion);

  void InvalidateViews(nsView *aView);

  
  void Refresh(nsView *aView, const nsIntRegion& aRegion, bool aWillSendDidPaint);

  void InvalidateRectDifference(nsView *aView, const nsRect& aRect, const nsRect& aCutOut);
  void InvalidateHorizontalBandDifference(nsView *aView, const nsRect& aRect, const nsRect& aCutOut,
                                          nscoord aY1, nscoord aY2, bool aInCutOut);

  

  bool IsViewInserted(nsView *aView);

  




  nsIntRect ViewToWidget(nsView *aView, const nsRect &aRect) const;

  void DoSetWindowDimensions(nscoord aWidth, nscoord aHeight);

  bool IsPainting() const {
    return RootViewManager()->mPainting;
  }

  void SetPainting(bool aPainting) {
    RootViewManager()->mPainting = aPainting;
  }

  nsresult InvalidateView(nsView *aView, const nsRect &aRect);

public: 
  nsView* GetRootViewImpl() const { return mRootView; }
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

  nsView           *mRootView;
  
  
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
