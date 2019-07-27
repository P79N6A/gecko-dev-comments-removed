




#ifndef nsViewManager_h___
#define nsViewManager_h___

#include "nscore.h"
#include "nsView.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsTArray.h"
#include "nsDeviceContext.h"
#include "nsTArray.h"
#include "mozilla/EventForwards.h"

class nsIWidget;
struct nsRect;
class nsRegion;
class nsDeviceContext;
class nsIPresShell;

class nsViewManager final
{
  ~nsViewManager();
public:
  friend class nsView;

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_INLINE_DECL_REFCOUNTING(nsViewManager)

  nsViewManager();

  





  nsresult Init(nsDeviceContext* aContext);

  











  nsView* CreateView(const nsRect& aBounds,
                     nsView* aParent,
                     nsViewVisibility aVisibilityFlag = nsViewVisibility_kShow);

  



  nsView* GetRootView() { return mRootView; }

  





  void SetRootView(nsView *aView);

  





  void GetWindowDimensions(nscoord *aWidth, nscoord *aHeight);

  






  void SetWindowDimensions(nscoord aWidth, nscoord aHeight);

  


  void FlushDelayedResize(bool aDoReflow);

  




  void InvalidateView(nsView *aView);

  






  void InvalidateViewNoSuppression(nsView *aView, const nsRect &aRect);

  


  void InvalidateAllViews();

  







  void DispatchEvent(mozilla::WidgetGUIEvent *aEvent,
                     nsView* aViewTarget,
                     nsEventStatus* aStatus);

  














  void InsertChild(nsView *aParent, nsView *aChild, nsView *aSibling,
                   bool aAfter);

  void InsertChild(nsView *aParent, nsView *aChild, int32_t aZIndex);

  






  void RemoveChild(nsView *aChild);

  








  void MoveViewTo(nsView *aView, nscoord aX, nscoord aY);

  











  void ResizeView(nsView *aView, const nsRect &aRect,
                  bool aRepaintExposedAreaOnly = false);

  










  void SetViewVisibility(nsView *aView, nsViewVisibility aVisible);

  











  void SetViewZIndex(nsView *aView, bool aAutoZIndex, int32_t aZindex);

  






  void SetViewFloating(nsView *aView, bool aFloatingView);

  



  void SetPresShell(nsIPresShell *aPresShell) { mPresShell = aPresShell; }

  


  nsIPresShell* GetPresShell() { return mPresShell; }

  


  nsDeviceContext* GetDeviceContext() const
  {
    return mContext;
  }

  











  class MOZ_STACK_CLASS AutoDisableRefresh {
  public:
    explicit AutoDisableRefresh(nsViewManager* aVM) {
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

    nsRefPtr<nsViewManager> mRootVM;
  };

private:
  friend class AutoDisableRefresh;

  nsViewManager* IncrementDisableRefreshCount();
  void DecrementDisableRefreshCount();

public:
  



  void GetRootWidget(nsIWidget **aWidget);

  





  void IsPainting(bool& aIsPainting);

  






  void GetLastUserEventTime(uint32_t& aTime);

  



  static nsView* GetDisplayRootFor(nsView* aView);

  



  void ProcessPendingUpdates();

  


  void UpdateWidgetGeometry();

  int32_t AppUnitsPerDevPixel() const
  {
    return mContext->AppUnitsPerDevPixel();
  }

private:
  static uint32_t gLastUserEventTime;

  
  void InvalidateHierarchy();
  void FlushPendingInvalidates();

  void ProcessPendingUpdatesForView(nsView *aView,
                                    bool aFlushDirtyRegion = true);
  void ProcessPendingUpdatesRecurse(nsView* aView,
                                    nsTArray<nsCOMPtr<nsIWidget> >& aWidgets);
  void ProcessPendingUpdatesPaint(nsIWidget* aWidget);

  void FlushDirtyRegionToWidget(nsView* aView);
  


  void CallWillPaintOnObservers();
  void ReparentChildWidgets(nsView* aView, nsIWidget *aNewWidget);
  void ReparentWidgets(nsView* aView, nsView *aParent);
  void InvalidateWidgetArea(nsView *aWidgetView, const nsRegion &aDamagedRegion);

  void InvalidateViews(nsView *aView);

  
  void Refresh(nsView *aView, const nsIntRegion& aRegion);

  

  bool IsViewInserted(nsView *aView);

  




  nsIntRect ViewToWidget(nsView *aView, const nsRect &aRect) const;

  void DoSetWindowDimensions(nscoord aWidth, nscoord aHeight);

  bool IsPainting() const {
    return RootViewManager()->mPainting;
  }

  void SetPainting(bool aPainting) {
    RootViewManager()->mPainting = aPainting;
  }

  void InvalidateView(nsView *aView, const nsRect &aRect);

  nsViewManager* RootViewManager() const { return mRootViewManager; }
  bool IsRootVM() const { return this == RootViewManager(); }

  
  
  
  bool IsPaintingAllowed() { return RootViewManager()->mRefreshDisableCount == 0; }

  void WillPaintWindow(nsIWidget* aWidget);
  bool PaintWindow(nsIWidget* aWidget, nsIntRegion aRegion);
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

  

  
  static nsTArray<nsViewManager*> *gViewManagers;
};

























#endif  
