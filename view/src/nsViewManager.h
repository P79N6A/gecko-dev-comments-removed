




































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
#include "nsView.h"
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

  NS_IMETHOD  UpdateView(nsIView *aView);
  NS_IMETHOD  UpdateViewNoSuppression(nsIView *aView, const nsRect &aRect);
  NS_IMETHOD  UpdateAllViews();

  NS_IMETHOD  DispatchEvent(nsGUIEvent *aEvent,
      nsIView* aTargetView, nsEventStatus* aStatus);

  NS_IMETHOD  InsertChild(nsIView *parent, nsIView *child, nsIView *sibling,
                          bool above);

  NS_IMETHOD  InsertChild(nsIView *parent, nsIView *child,
                          PRInt32 zindex);

  NS_IMETHOD  RemoveChild(nsIView *parent);

  NS_IMETHOD  MoveViewTo(nsIView *aView, nscoord aX, nscoord aY);

  NS_IMETHOD  ResizeView(nsIView *aView, const nsRect &aRect, bool aRepaintExposedAreaOnly = false);

  NS_IMETHOD  SetViewFloating(nsIView *aView, bool aFloating);

  NS_IMETHOD  SetViewVisibility(nsIView *aView, nsViewVisibility aVisible);

  NS_IMETHOD  SetViewZIndex(nsIView *aView, bool aAuto, PRInt32 aZIndex, bool aTopMost=false);

  virtual void SetPresShell(nsIPresShell *aPresShell) { mPresShell = aPresShell; }
  virtual nsIPresShell* GetPresShell() { return mPresShell; }

  NS_IMETHOD  GetDeviceContext(nsDeviceContext *&aContext);

  virtual nsIViewManager* BeginUpdateViewBatch(void);
  NS_IMETHOD  EndUpdateViewBatch();

  NS_IMETHOD GetRootWidget(nsIWidget **aWidget);
 
  NS_IMETHOD IsPainting(bool& aIsPainting);
  NS_IMETHOD GetLastUserEventTime(PRUint32& aTime);
  static PRUint32 gLastUserEventTime;

  
  void InvalidateHierarchy();

protected:
  virtual ~nsViewManager();

private:

  void FlushPendingInvalidates();
  void ProcessPendingUpdates(nsView *aView, bool aDoInvalidate);
  void FlushDirtyRegionToWidget(nsView* aView);
  


  void CallWillPaintOnObservers(bool aWillSendDidPaint);
  void CallDidPaintOnObservers();
  void ReparentChildWidgets(nsIView* aView, nsIWidget *aNewWidget);
  void ReparentWidgets(nsIView* aView, nsIView *aParent);
  void UpdateWidgetArea(nsView *aWidgetView, nsIWidget* aWidget,
                        const nsRegion &aDamagedRegion);

  void UpdateViews(nsView *aView);

  void TriggerRefresh();

  
  void Refresh(nsView *aView, nsIWidget *aWidget, const nsIntRegion& aRegion);
  
  
  void RenderViews(nsView *aRootView, nsIWidget *aWidget,
                   const nsRegion& aRegion, const nsIntRegion& aIntRegion,
                   bool aPaintDefaultBackground, bool aWillSendDidPaint);

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

  nsresult UpdateView(nsIView *aView, const nsRect &aRect);

public: 
  nsView* GetRootViewImpl() const { return mRootView; }
  nsViewManager* RootViewManager() const { return mRootViewManager; }
  bool IsRootVM() const { return this == RootViewManager(); }

  bool IsRefreshEnabled() { return RootViewManager()->mUpdateBatchCnt == 0; }

  
  
  void PostPendingUpdate() { RootViewManager()->mHasPendingUpdates = true; }

  PRUint32 AppUnitsPerDevPixel() const
  {
    return mContext->AppUnitsPerDevPixel();
  }

private:
  nsRefPtr<nsDeviceContext> mContext;
  nsIPresShell   *mPresShell;

  
  
  nsSize            mDelayedResize;

  nsView            *mRootView;
  
  
  nsViewManager     *mRootViewManager;

  
  
  
  
  PRInt32           mUpdateBatchCnt;
  
  bool              mPainting;
  bool              mRecursiveRefreshPending;
  bool              mHasPendingUpdates;
  bool              mInScroll;

  
  static PRInt32           mVMCount;        

  
  static nsVoidArray       *gViewManagers;
};

#endif 
