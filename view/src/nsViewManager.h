




































#ifndef nsViewManager_h___
#define nsViewManager_h___
#include "nsCOMPtr.h"
#include "nsIViewManager.h"
#include "nsCRT.h"
#include "nsIWidget.h"
#include "nsITimer.h"
#include "prtime.h"
#include "prinrval.h"
#include "nsVoidArray.h"
#include "nsThreadUtils.h"
#include "nsIScrollableView.h"
#include "nsIRegion.h"
#include "nsIBlender.h"
#include "nsView.h"

class nsISupportsArray;
class BlendingBuffers;


#ifdef MOZ_PERF_METRICS

#endif

#ifdef NS_VM_PERF_METRICS
#include "nsTimer.h"
#endif






















































class nsZPlaceholderView : public nsView
{
public:
  nsZPlaceholderView(nsViewManager* aViewManager) : nsView(aViewManager) {}

  void RemoveReparentedView() { mReparentedView = nsnull; }
  void SetReparentedView(nsView* aView) { mReparentedView = aView; }
  nsView* GetReparentedView() const { return mReparentedView; }

  virtual PRBool IsZPlaceholderView() const { return PR_TRUE; }

protected:
  virtual ~nsZPlaceholderView() {
    if (nsnull != mReparentedView) {
      mReparentedView->SetZParent(nsnull);
    }
  }

protected:
  nsView   *mReparentedView;
};

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

  NS_IMETHOD  Composite(void);

  NS_IMETHOD  UpdateView(nsIView *aView, PRUint32 aUpdateFlags);
  NS_IMETHOD  UpdateView(nsIView *aView, const nsRect &aRect, PRUint32 aUpdateFlags);
  NS_IMETHOD  UpdateAllViews(PRUint32 aUpdateFlags);

  NS_IMETHOD  DispatchEvent(nsGUIEvent *aEvent, nsEventStatus* aStatus);

  NS_IMETHOD  GrabMouseEvents(nsIView *aView, PRBool &aResult);

  NS_IMETHOD  GetMouseEventGrabber(nsIView *&aView);

  NS_IMETHOD  InsertChild(nsIView *parent, nsIView *child, nsIView *sibling,
                          PRBool above);

  NS_IMETHOD  InsertChild(nsIView *parent, nsIView *child,
                          PRInt32 zindex);

  NS_IMETHOD  InsertZPlaceholder(nsIView *parent, nsIView *child, nsIView *sibling,
                                 PRBool above);

  NS_IMETHOD  RemoveChild(nsIView *parent);

  NS_IMETHOD  MoveViewBy(nsIView *aView, nscoord aX, nscoord aY);

  NS_IMETHOD  MoveViewTo(nsIView *aView, nscoord aX, nscoord aY);

  NS_IMETHOD  ResizeView(nsIView *aView, const nsRect &aRect, PRBool aRepaintExposedAreaOnly = PR_FALSE);

  NS_IMETHOD  SetViewCheckChildEvents(nsIView *aView, PRBool aEnable);

  NS_IMETHOD  SetViewFloating(nsIView *aView, PRBool aFloating);

  NS_IMETHOD  SetViewVisibility(nsIView *aView, nsViewVisibility aVisible);

  NS_IMETHOD  SetViewZIndex(nsIView *aView, PRBool aAuto, PRInt32 aZIndex, PRBool aTopMost=PR_FALSE);

  NS_IMETHOD  SetViewObserver(nsIViewObserver *aObserver);
  NS_IMETHOD  GetViewObserver(nsIViewObserver *&aObserver);

  NS_IMETHOD  GetDeviceContext(nsIDeviceContext *&aContext);

  NS_IMETHOD  DisableRefresh(void);
  NS_IMETHOD  EnableRefresh(PRUint32 aUpdateFlags);

  NS_IMETHOD  BeginUpdateViewBatch(void);
  NS_IMETHOD  EndUpdateViewBatch(PRUint32 aUpdateFlags);

  NS_IMETHOD  SetRootScrollableView(nsIScrollableView *aScrollable);
  NS_IMETHOD  GetRootScrollableView(nsIScrollableView **aScrollable);

  NS_IMETHOD RenderOffscreen(nsIView* aView, nsRect aRect, PRBool aUntrusted,
                             PRBool aIgnoreViewportScrolling,
                             nscolor aBackgroundColor,
                             nsIRenderingContext** aRenderedContext);

  NS_IMETHOD AddCompositeListener(nsICompositeListener *aListener);
  NS_IMETHOD RemoveCompositeListener(nsICompositeListener *aListener);

  NS_IMETHOD GetWidget(nsIWidget **aWidget);
  nsIWidget* GetWidget() { return mRootView ? mRootView->GetWidget() : nsnull; }
  NS_IMETHOD ForceUpdate();
 
  NS_IMETHOD IsPainting(PRBool& aIsPainting);
  NS_IMETHOD SetDefaultBackgroundColor(nscolor aColor);
  NS_IMETHOD GetDefaultBackgroundColor(nscolor* aColor);
  NS_IMETHOD GetLastUserEventTime(PRUint32& aTime);
  void ProcessInvalidateEvent();
  static PRInt32 GetViewManagerCount();
  static const nsVoidArray* GetViewManagerArray();
  static PRUint32 gLastUserEventTime;

  









  NS_IMETHOD GetRectVisibility(nsIView *aView, const nsRect &aRect, 
                               PRUint16 aMinTwips, 
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
  void UpdateWidgetArea(nsView *aWidgetView, const nsRegion &aDamagedRegion,
                        nsView* aIgnoreWidgetView);

  void UpdateViews(nsView *aView, PRUint32 aUpdateFlags);

  void Refresh(nsView *aView, nsIRenderingContext *aContext,
               nsIRegion *region, PRUint32 aUpdateFlags);
  


  void DefaultRefresh(nsView* aView, nsIRenderingContext *aContext, const nsRect* aRect);
  void RenderViews(nsView *aRootView, nsIRenderingContext& aRC,
                   const nsRegion& aRegion, nsIDrawingSurface* aRCSurface);

  void InvalidateRectDifference(nsView *aView, const nsRect& aRect, const nsRect& aCutOut, PRUint32 aUpdateFlags);
  void InvalidateHorizontalBandDifference(nsView *aView, const nsRect& aRect, const nsRect& aCutOut,
                                          PRUint32 aUpdateFlags, nscoord aY1, nscoord aY2, PRBool aInCutOut);

  virtual BlendingBuffers* CreateBlendingBuffers(nsIRenderingContext *aRC, PRBool aBorrowContext,
                                                 nsIDrawingSurface* aBorrowSurface, PRBool aNeedAlpha,
                                                 const nsRect& aArea);
  virtual nsIBlender* GetBlender() { return mBlender; }

  void AddCoveringWidgetsToOpaqueRegion(nsRegion &aRgn, nsIDeviceContext* aContext,
                                        nsView* aRootView);

  
  PRBool DoesViewHaveNativeWidget(nsView* aView);

  

  PRBool IsViewInserted(nsView *aView);

  



  void UpdateWidgetsForView(nsView* aView);

  



  static nsView* GetWidgetView(nsView *aView);

  



  void ViewToWidget(nsView *aView, nsView* aWidgetView, nsRect &aRect) const;

  









  nsresult GetAbsoluteRect(nsView *aView, const nsRect &aRect, 
                           nsRect& aAbsRect);
  





  nsresult GetVisibleRect(nsRect& aVisibleRect);

  

  






  void GetMaxWidgetBounds(nsRect& aMaxWidgetBounds) const;

  void DoSetWindowDimensions(nscoord aWidth, nscoord aHeight)
  {
    nsRect oldDim;
    nsRect newDim(0, 0, aWidth, aHeight);
    mRootView->GetDimensions(oldDim);
    if (oldDim != newDim) {
      
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
  nsView* GetMouseEventGrabber() const {
    return RootViewManager()->mMouseGrabber;
  }
  nsViewManager* RootViewManager() const { return mRootViewManager; }
  PRBool IsRootVM() const { return this == RootViewManager(); }

  nsEventStatus HandleEvent(nsView* aView, nsPoint aPoint, nsGUIEvent* aEvent,
                            PRBool aCaptured);

  






















  nsresult WillBitBlit(nsView* aView, nsPoint aScrollAmount);
  
  







  void UpdateViewAfterScroll(nsView *aView, const nsRegion& aUpdateRegion);

  




  PRBool CanScrollWithBitBlt(nsView* aView, nsPoint aDelta, nsRegion* aUpdateRegion);

  nsresult CreateRegion(nsIRegion* *result);

  
  
  
  static nsPoint ComputeViewOffset(const nsView *aView);

  PRBool IsRefreshEnabled() { return RootViewManager()->mRefreshEnabled; }

  nsIViewObserver* GetViewObserver() { return mObserver; }

  
  
  void PostPendingUpdate() { RootViewManager()->mHasPendingUpdates = PR_TRUE; }
private:
  nsIDeviceContext  *mContext;
  nsIViewObserver   *mObserver;
  nsIScrollableView *mRootScrollable;
  nscolor           mDefaultBackgroundColor;
  nsPoint           mMouseLocation; 

  
  
  nsSize            mDelayedResize;

  nsCOMPtr<nsIBlender> mBlender;
  nsISupportsArray  *mCompositeListeners;
  nsCOMPtr<nsIFactory> mRegionFactory;
  nsView            *mRootView;
  
  
  nsViewManager     *mRootViewManager;

  nsRevocableEventPtr<nsViewManagerEvent> mSynthMouseMoveEvent;
  nsRevocableEventPtr<nsViewManagerEvent> mInvalidateEvent;

  
  
  
  
  
  nsView            *mMouseGrabber;
  
  
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

#ifdef NS_VM_PERF_METRICS
  MOZ_TIMER_DECLARE(mWatch) 
#endif
};


#define NS_VMREFRESH_DOUBLE_BUFFER      0x0001

#endif 
