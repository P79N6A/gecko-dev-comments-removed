




































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
#include "nsIScrollableView.h"
#include "nsIRegion.h"
#include "nsView.h"
#include "nsIViewObserver.h"




































class nsViewManagerEvent : public nsRunnable {
public:
  nsViewManagerEvent(class nsViewManager *vm) : mViewManager(vm) {
    NS_ASSERTION(mViewManager, "null parameter");
  }
  void Revoke() { mViewManager = nsnull; }
protected:
  class nsViewManager *mViewManager;
};

class nsViewManager : public nsIViewManager {
public:
  nsViewManager();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_DECL_ISUPPORTS

  NS_IMETHOD  Init(nsIDeviceContext* aContext);

  NS_IMETHOD_(nsIView*) CreateView(const nsRect& aBounds,
                                   const nsIView* aParent,
                                   nsViewVisibility aVisibilityFlag = nsViewVisibility_kShow);

  NS_IMETHOD_(nsIScrollableView*) CreateScrollableView(const nsRect& aBounds,
                                                       const nsIView* aParent);

  NS_IMETHOD  GetRootView(nsIView *&aView);
  NS_IMETHOD  SetRootView(nsIView *aView);

  NS_IMETHOD  GetWindowDimensions(nscoord *width, nscoord *height);
  NS_IMETHOD  SetWindowDimensions(nscoord width, nscoord height);
  NS_IMETHOD  FlushDelayedResize();

  NS_IMETHOD  Composite(void);

  NS_IMETHOD  UpdateView(nsIView *aView, PRUint32 aUpdateFlags);
  NS_IMETHOD  UpdateView(nsIView *aView, const nsRect &aRect, PRUint32 aUpdateFlags);
  NS_IMETHOD  UpdateAllViews(PRUint32 aUpdateFlags);

  NS_IMETHOD  DispatchEvent(nsGUIEvent *aEvent,
      nsIView* aTargetView, nsEventStatus* aStatus);

  NS_IMETHOD  InsertChild(nsIView *parent, nsIView *child, nsIView *sibling,
                          PRBool above);

  NS_IMETHOD  InsertChild(nsIView *parent, nsIView *child,
                          PRInt32 zindex);

  NS_IMETHOD  RemoveChild(nsIView *parent);

  NS_IMETHOD  MoveViewBy(nsIView *aView, nscoord aX, nscoord aY);

  NS_IMETHOD  MoveViewTo(nsIView *aView, nscoord aX, nscoord aY);

  NS_IMETHOD  ResizeView(nsIView *aView, const nsRect &aRect, PRBool aRepaintExposedAreaOnly = PR_FALSE);

  NS_IMETHOD  SetViewFloating(nsIView *aView, PRBool aFloating);

  NS_IMETHOD  SetViewVisibility(nsIView *aView, nsViewVisibility aVisible);

  NS_IMETHOD  SetViewZIndex(nsIView *aView, PRBool aAuto, PRInt32 aZIndex, PRBool aTopMost=PR_FALSE);

  NS_IMETHOD  SetViewObserver(nsIViewObserver *aObserver);
  NS_IMETHOD  GetViewObserver(nsIViewObserver *&aObserver);

  NS_IMETHOD  GetDeviceContext(nsIDeviceContext *&aContext);

  NS_IMETHOD  DisableRefresh(void);
  NS_IMETHOD  EnableRefresh(PRUint32 aUpdateFlags);

  virtual nsIViewManager* BeginUpdateViewBatch(void);
  NS_IMETHOD  EndUpdateViewBatch(PRUint32 aUpdateFlags);

  NS_IMETHOD  SetRootScrollableView(nsIScrollableView *aScrollable);
  NS_IMETHOD  GetRootScrollableView(nsIScrollableView **aScrollable);

  NS_IMETHOD GetRootWidget(nsIWidget **aWidget);
  NS_IMETHOD ForceUpdate();
 
  NS_IMETHOD IsPainting(PRBool& aIsPainting);
  NS_IMETHOD GetLastUserEventTime(PRUint32& aTime);
  void ProcessInvalidateEvent();
  static PRUint32 gLastUserEventTime;

  









  NS_IMETHOD GetRectVisibility(nsIView *aView, const nsRect &aRect, 
                               nscoord aMinTwips,
                               nsRectVisibility *aRectVisibility);

  NS_IMETHOD SynthesizeMouseMove(PRBool aFromScroll);
  void ProcessSynthMouseMoveEvent(PRBool aFromScroll);

  
  void InvalidateHierarchy();

protected:
  virtual ~nsViewManager();

private:

  void FlushPendingInvalidates();
  void ProcessPendingUpdates(nsView *aView, PRBool aDoInvalidate);
  void ReparentChildWidgets(nsIView* aView, nsIWidget *aNewWidget);
  void ReparentWidgets(nsIView* aView, nsIView *aParent);
  already_AddRefed<nsIRenderingContext> CreateRenderingContext(nsView &aView);
  void UpdateWidgetArea(nsView *aWidgetView, nsIWidget* aWidget,
                        const nsRegion &aDamagedRegion,
                        nsView* aIgnoreWidgetView);

  void UpdateViews(nsView *aView, PRUint32 aUpdateFlags);

  void Refresh(nsView *aView, nsIRenderingContext *aContext,
               nsIRegion *region, PRUint32 aUpdateFlags);
  void RenderViews(nsView *aRootView, nsIRenderingContext& aRC,
                   const nsRegion& aRegion);

  void InvalidateRectDifference(nsView *aView, const nsRect& aRect, const nsRect& aCutOut, PRUint32 aUpdateFlags);
  void InvalidateHorizontalBandDifference(nsView *aView, const nsRect& aRect, const nsRect& aCutOut,
                                          PRUint32 aUpdateFlags, nscoord aY1, nscoord aY2, PRBool aInCutOut);

  

  PRBool IsViewInserted(nsView *aView);

  



  void UpdateWidgetsForView(nsView* aView);

  




  nsIntRect ViewToWidget(nsView *aView, nsView* aWidgetView, const nsRect &aRect) const;

  









  nsresult GetAbsoluteRect(nsView *aView, const nsRect &aRect, 
                           nsRect& aAbsRect);
  





  nsresult GetVisibleRect(nsRect& aVisibleRect);

  void DoSetWindowDimensions(nscoord aWidth, nscoord aHeight)
  {
    nsRect oldDim;
    nsRect newDim(0, 0, aWidth, aHeight);
    mRootView->GetDimensions(oldDim);
    
    if (!oldDim.IsExactEqual(newDim)) {
      
      mRootView->SetDimensions(newDim, PR_TRUE, PR_FALSE);
      if (mObserver)
        mObserver->ResizeReflow(mRootView, aWidth, aHeight);
    }
  }

  
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

public: 
  nsView* GetRootView() const { return mRootView; }
  nsViewManager* RootViewManager() const { return mRootViewManager; }
  PRBool IsRootVM() const { return this == RootViewManager(); }

  nsEventStatus HandleEvent(nsView* aView, nsPoint aPoint, nsGUIEvent* aEvent);

  






















  nsresult WillBitBlit(nsView* aView, nsPoint aScrollAmount);
  
  










  void UpdateViewAfterScroll(nsView *aView, const nsRegion& aBlitRegion,
                             const nsRegion& aUpdateRegion);

  




  void GetRegionsForBlit(nsView* aView, nsPoint aDelta,
                         nsRegion* aBlitRegion, nsRegion* aRepaintRegion);

  nsresult CreateRegion(nsIRegion* *result);

  PRBool IsRefreshEnabled() { return RootViewManager()->mRefreshEnabled; }

  nsIViewObserver* GetViewObserver() { return mObserver; }

  
  
  void PostPendingUpdate() { RootViewManager()->mHasPendingUpdates = PR_TRUE; }
private:
  nsIDeviceContext  *mContext;
  nsIViewObserver   *mObserver;
  nsIScrollableView *mRootScrollable;
  nsIntPoint        mMouseLocation; 

  
  
  nsSize            mDelayedResize;

  nsCOMPtr<nsIFactory> mRegionFactory;
  nsView            *mRootView;
  
  
  nsViewManager     *mRootViewManager;

  nsRevocableEventPtr<nsViewManagerEvent> mSynthMouseMoveEvent;
  nsRevocableEventPtr<nsViewManagerEvent> mInvalidateEvent;

  
  
  
  
  
  
  PRInt32           mUpdateCnt;
  PRInt32           mUpdateBatchCnt;
  PRUint32          mUpdateBatchFlags;
  PRInt32           mScrollCnt;
  
  PRPackedBool      mRefreshEnabled;
  
  PRPackedBool      mPainting;
  PRPackedBool      mRecursiveRefreshPending;
  PRPackedBool      mHasPendingUpdates;
  PRPackedBool      mInScroll;

  
  static PRInt32           mVMCount;        

  
  static nsIRenderingContext* gCleanupContext;

  
  static nsVoidArray       *gViewManagers;

  void PostInvalidateEvent();
};


#define NS_VMREFRESH_DOUBLE_BUFFER      0x0001

#endif 
