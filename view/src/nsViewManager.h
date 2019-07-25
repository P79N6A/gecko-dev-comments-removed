




































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
#include "nsIRegion.h"
#include "nsView.h"
#include "nsIViewObserver.h"
#include "nsDeviceContext.h"

































class nsInvalidateEvent;

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
  NS_IMETHOD  FlushDelayedResize(PRBool aDoReflow);

  NS_IMETHOD  Composite(void);

  NS_IMETHOD  UpdateView(nsIView *aView, PRUint32 aUpdateFlags);
  NS_IMETHOD  UpdateViewNoSuppression(nsIView *aView, const nsRect &aRect,
                                      PRUint32 aUpdateFlags);
  NS_IMETHOD  UpdateAllViews(PRUint32 aUpdateFlags);

  NS_IMETHOD  DispatchEvent(nsGUIEvent *aEvent,
      nsIView* aTargetView, nsEventStatus* aStatus);

  NS_IMETHOD  InsertChild(nsIView *parent, nsIView *child, nsIView *sibling,
                          PRBool above);

  NS_IMETHOD  InsertChild(nsIView *parent, nsIView *child,
                          PRInt32 zindex);

  NS_IMETHOD  RemoveChild(nsIView *parent);

  NS_IMETHOD  MoveViewTo(nsIView *aView, nscoord aX, nscoord aY);

  NS_IMETHOD  ResizeView(nsIView *aView, const nsRect &aRect, PRBool aRepaintExposedAreaOnly = PR_FALSE);

  NS_IMETHOD  SetViewFloating(nsIView *aView, PRBool aFloating);

  NS_IMETHOD  SetViewVisibility(nsIView *aView, nsViewVisibility aVisible);

  NS_IMETHOD  SetViewZIndex(nsIView *aView, PRBool aAuto, PRInt32 aZIndex, PRBool aTopMost=PR_FALSE);

  virtual void SetViewObserver(nsIViewObserver *aObserver) { mObserver = aObserver; }
  virtual nsIViewObserver* GetViewObserver() { return mObserver; }

  NS_IMETHOD  GetDeviceContext(nsDeviceContext *&aContext);

  virtual nsIViewManager* BeginUpdateViewBatch(void);
  NS_IMETHOD  EndUpdateViewBatch(PRUint32 aUpdateFlags);

  NS_IMETHOD GetRootWidget(nsIWidget **aWidget);
  NS_IMETHOD ForceUpdate();
 
  NS_IMETHOD IsPainting(PRBool& aIsPainting);
  NS_IMETHOD GetLastUserEventTime(PRUint32& aTime);
  void ProcessInvalidateEvent();
  static PRUint32 gLastUserEventTime;

  
  void InvalidateHierarchy();

protected:
  virtual ~nsViewManager();

private:

  void FlushPendingInvalidates();
  void ProcessPendingUpdates(nsView *aView, PRBool aDoInvalidate);
  


  void CallWillPaintOnObservers(PRBool aWillSendDidPaint);
  void CallDidPaintOnObservers();
  void ReparentChildWidgets(nsIView* aView, nsIWidget *aNewWidget);
  void ReparentWidgets(nsIView* aView, nsIView *aParent);
  void UpdateWidgetArea(nsView *aWidgetView, nsIWidget* aWidget,
                        const nsRegion &aDamagedRegion,
                        nsView* aIgnoreWidgetView);

  void UpdateViews(nsView *aView, PRUint32 aUpdateFlags);

  void TriggerRefresh(PRUint32 aUpdateFlags);

  
  void Refresh(nsView *aView, nsIWidget *aWidget,
               const nsIntRegion& aRegion, PRUint32 aUpdateFlags);
  
  
  void RenderViews(nsView *aRootView, nsIWidget *aWidget,
                   const nsRegion& aRegion, const nsIntRegion& aIntRegion,
                   PRBool aPaintDefaultBackground, PRBool aWillSendDidPaint);

  void InvalidateRectDifference(nsView *aView, const nsRect& aRect, const nsRect& aCutOut, PRUint32 aUpdateFlags);
  void InvalidateHorizontalBandDifference(nsView *aView, const nsRect& aRect, const nsRect& aCutOut,
                                          PRUint32 aUpdateFlags, nscoord aY1, nscoord aY2, PRBool aInCutOut);

  

  PRBool IsViewInserted(nsView *aView);

  



  void UpdateWidgetsForView(nsView* aView);

  




  nsIntRect ViewToWidget(nsView *aView, const nsRect &aRect) const;

  void DoSetWindowDimensions(nscoord aWidth, nscoord aHeight);

  
  void IncrementUpdateCount() {
    NS_ASSERTION(IsRootVM(),
                 "IncrementUpdateCount called on non-root viewmanager");
    ++mUpdateCnt;
  }

  void DecrementUpdateCount() {
    NS_ASSERTION(IsRootVM(),
                 "DecrementUpdateCount called on non-root viewmanager");
    --mUpdateCnt;
  }

  PRInt32 UpdateCount() const {
    NS_ASSERTION(IsRootVM(),
                 "DecrementUpdateCount called on non-root viewmanager");
    return mUpdateCnt;
  }

  void ClearUpdateCount() {
    NS_ASSERTION(IsRootVM(),
                 "DecrementUpdateCount called on non-root viewmanager");
    mUpdateCnt = 0;
  }

  PRBool IsPainting() const {
    return RootViewManager()->mPainting;
  }

  void SetPainting(PRBool aPainting) {
    RootViewManager()->mPainting = aPainting;
  }

  nsresult UpdateView(nsIView *aView, const nsRect &aRect, PRUint32 aUpdateFlags);

public: 
  nsView* GetRootViewImpl() const { return mRootView; }
  nsViewManager* RootViewManager() const { return mRootViewManager; }
  PRBool IsRootVM() const { return this == RootViewManager(); }

  nsEventStatus HandleEvent(nsView* aView, nsGUIEvent* aEvent);

  nsresult CreateRegion(nsIRegion* *result);

  PRBool IsRefreshEnabled() { return RootViewManager()->mUpdateBatchCnt == 0; }

  
  
  void PostPendingUpdate() { RootViewManager()->mHasPendingUpdates = PR_TRUE; }

  PRInt32 AppUnitsPerDevPixel() const
  {
    return mContext->AppUnitsPerDevPixel();
  }

private:
  nsRefPtr<nsDeviceContext> mContext;
  nsIViewObserver   *mObserver;

  
  
  nsSize            mDelayedResize;

  nsCOMPtr<nsIFactory> mRegionFactory;
  nsView            *mRootView;
  
  
  nsViewManager     *mRootViewManager;

  nsRevocableEventPtr<nsInvalidateEvent> mInvalidateEvent;

  
  
  
  
  
  
  PRInt32           mUpdateCnt;
  PRInt32           mUpdateBatchCnt;
  PRUint32          mUpdateBatchFlags;
  
  PRPackedBool      mPainting;
  PRPackedBool      mRecursiveRefreshPending;
  PRPackedBool      mHasPendingUpdates;
  PRPackedBool      mInScroll;

  
  static PRInt32           mVMCount;        

  
  static nsVoidArray       *gViewManagers;

  void PostInvalidateEvent();
};


#define NS_VMREFRESH_DOUBLE_BUFFER      0x0001

class nsInvalidateEvent : public nsRunnable {
public:
  nsInvalidateEvent(class nsViewManager *vm) : mViewManager(vm) {
    NS_ASSERTION(mViewManager, "null parameter");
  }
  void Revoke() { mViewManager = nsnull; }

  NS_IMETHOD Run() {
    if (mViewManager)
      mViewManager->ProcessInvalidateEvent();
    return NS_OK;
  }
protected:
  class nsViewManager *mViewManager;
};

#endif 
