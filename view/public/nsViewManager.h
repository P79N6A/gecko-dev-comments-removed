




#ifndef nsViewManager_h___
#define nsViewManager_h___

#include "nscore.h"
#include "nsView.h"
#include "nsEvent.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsITimer.h"
#include "prtime.h"
#include "prinrval.h"
#include "nsVoidArray.h"
#include "nsThreadUtils.h"
#include "nsIPresShell.h"
#include "nsDeviceContext.h"

class nsIWidget;
struct nsRect;
class nsRegion;
class nsDeviceContext;
class nsIPresShell;
class nsView;

#define NS_IVIEWMANAGER_IID \
{ 0x540610a6, 0x4fdd, 0x4ae3, \
  { 0x9b, 0xdb, 0xa6, 0x4d, 0x8b, 0xca, 0x02, 0x0f } }

class nsViewManager : public nsISupports
{
public:
  friend class nsView;

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IVIEWMANAGER_IID)

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_DECL_ISUPPORTS

  nsViewManager();
  virtual ~nsViewManager();

  





  NS_IMETHOD  Init(nsDeviceContext* aContext);

  











  NS_IMETHOD_(nsView*) CreateView(const nsRect& aBounds,
                                   const nsView* aParent,
                                   nsViewVisibility aVisibilityFlag = nsViewVisibility_kShow);

  



  NS_IMETHOD_(nsView*) GetRootView() { return mRootView; }

  





  NS_IMETHOD  SetRootView(nsView *aView);

  





  NS_IMETHOD  GetWindowDimensions(nscoord *aWidth, nscoord *aHeight);

  






  NS_IMETHOD  SetWindowDimensions(nscoord aWidth, nscoord aHeight);

  


  NS_IMETHOD  FlushDelayedResize(bool aDoReflow);

  




  NS_IMETHOD  InvalidateView(nsView *aView);

  






  NS_IMETHOD  InvalidateViewNoSuppression(nsView *aView, const nsRect &aRect);

  


  NS_IMETHOD  InvalidateAllViews();

  







  NS_IMETHOD  DispatchEvent(nsGUIEvent *aEvent,
      nsView* aViewTarget, nsEventStatus* aStatus);

  














  NS_IMETHOD  InsertChild(nsView *aParent, nsView *aChild, nsView *aSibling,
                          bool aAfter);

  NS_IMETHOD InsertChild(nsView *aParent, nsView *aChild, int32_t aZIndex);

  






  NS_IMETHOD  RemoveChild(nsView *aChild);

  








  NS_IMETHOD  MoveViewTo(nsView *aView, nscoord aX, nscoord aY);

  











  NS_IMETHOD  ResizeView(nsView *aView, const nsRect &aRect,
                         bool aRepaintExposedAreaOnly = false);

  










  NS_IMETHOD  SetViewVisibility(nsView *aView, nsViewVisibility aVisible);

  















  NS_IMETHOD  SetViewZIndex(nsView *aView, bool aAutoZIndex, int32_t aZindex, bool aTopMost = false);

  






  NS_IMETHOD  SetViewFloating(nsView *aView, bool aFloatingView);

  



  virtual void SetPresShell(nsIPresShell *aPresShell) { mPresShell = aPresShell; }

  


  virtual nsIPresShell* GetPresShell() { return mPresShell; }

  



  NS_IMETHOD  GetDeviceContext(nsDeviceContext *&aContext);

  











  class NS_STACK_CLASS AutoDisableRefresh {
  public:
    AutoDisableRefresh(nsViewManager* aVM) {
      if (aVM) {
        mRootVM = aVM->IncrementDisableRefreshCount();
      }
    }
    ~AutoDisableRefresh() {
      if (mRootVM) {
        mRootVM->DecrementDisableRefreshCount();
      }
    }
  private:
    AutoDisableRefresh(const AutoDisableRefresh& aOther);
    const AutoDisableRefresh& operator=(const AutoDisableRefresh& aOther);

    nsCOMPtr<nsViewManager> mRootVM;
  };

private:
  friend class AutoDisableRefresh;

  virtual nsViewManager* IncrementDisableRefreshCount();
  virtual void DecrementDisableRefreshCount();

public:
  



  NS_IMETHOD GetRootWidget(nsIWidget **aWidget);

  





  NS_IMETHOD IsPainting(bool& aIsPainting);

  






  NS_IMETHOD GetLastUserEventTime(uint32_t& aTime);

  



  static nsView* GetDisplayRootFor(nsView* aView);

  



  virtual void ProcessPendingUpdates();

  


  virtual void UpdateWidgetGeometry();

  uint32_t AppUnitsPerDevPixel() const
  {
    return mContext->AppUnitsPerDevPixel();
  }
  nsView* GetRootViewImpl() const { return mRootView; }

private:
  static uint32_t gLastUserEventTime;

  
  void InvalidateHierarchy();
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

  nsViewManager* RootViewManager() const { return mRootViewManager; }
  bool IsRootVM() const { return this == RootViewManager(); }

  
  
  
  bool IsPaintingAllowed() { return RootViewManager()->mRefreshDisableCount == 0; }

  void WillPaintWindow(nsIWidget* aWidget, bool aWillSendDidPaint);
  bool PaintWindow(nsIWidget* aWidget, nsIntRegion aRegion,
                   uint32_t aFlags);
  void DidPaintWindow();

  
  
  void PostPendingUpdate();

  nsRefPtr<nsDeviceContext> mContext;
  nsIPresShell   *mPresShell;

  
  
  nsSize            mDelayedResize;

  nsView           *mRootView;
  
  
  nsViewManager   *mRootViewManager;

  
  
  

  int32_t           mRefreshDisableCount;
  
  bool              mPainting;
  bool              mRecursiveRefreshPending;
  bool              mHasPendingWidgetGeometryChanges;
  bool              mInScroll;

  
  static int32_t           mVMCount;        

  
  static nsVoidArray       *gViewManagers;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsViewManager, NS_IVIEWMANAGER_IID)


























#endif  
