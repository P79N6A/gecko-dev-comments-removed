




































#ifndef nsIViewManager_h___
#define nsIViewManager_h___

#include "nscore.h"
#include "nsIView.h"
#include "nsEvent.h"

class nsIWidget;
struct nsRect;
class nsRegion;
class nsDeviceContext;

#define NS_IVIEWMANAGER_IID \
{ 0x1262a33f, 0xc19f, 0x4e5b, \
  { 0x85, 0x00, 0xab, 0xf3, 0x7d, 0xcf, 0x30, 0x1d } }

class nsIViewManager : public nsISupports
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IVIEWMANAGER_IID)
  





  NS_IMETHOD  Init(nsDeviceContext* aContext) = 0;

  











  NS_IMETHOD_(nsIView*) CreateView(const nsRect& aBounds,
                                   const nsIView* aParent,
                                   nsViewVisibility aVisibilityFlag = nsViewVisibility_kShow) = 0;

  



  NS_IMETHOD_(nsIView*) GetRootView() = 0;

  





  NS_IMETHOD  SetRootView(nsIView *aView) = 0;

  





  NS_IMETHOD  GetWindowDimensions(nscoord *aWidth, nscoord *aHeight) = 0;

  






  NS_IMETHOD  SetWindowDimensions(nscoord aWidth, nscoord aHeight) = 0;

  


  NS_IMETHOD  FlushDelayedResize(bool aDoReflow) = 0;

  




  NS_IMETHOD  UpdateView(nsIView *aView) = 0;

  






  NS_IMETHOD  UpdateViewNoSuppression(nsIView *aView, const nsRect &aRect) = 0;

  



  NS_IMETHOD  UpdateAllViews() = 0;

  







  NS_IMETHOD  DispatchEvent(nsGUIEvent *aEvent,
      nsIView* aViewTarget, nsEventStatus* aStatus) = 0;

  














  NS_IMETHOD  InsertChild(nsIView *aParent, nsIView *aChild, nsIView *aSibling,
                          bool aAfter) = 0;

  






  NS_IMETHOD  RemoveChild(nsIView *aChild) = 0;

  








  NS_IMETHOD  MoveViewTo(nsIView *aView, nscoord aX, nscoord aY) = 0;

  











  NS_IMETHOD  ResizeView(nsIView *aView, const nsRect &aRect,
                         bool aRepaintExposedAreaOnly = false) = 0;

  










  NS_IMETHOD  SetViewVisibility(nsIView *aView, nsViewVisibility aVisible) = 0;

  















  NS_IMETHOD  SetViewZIndex(nsIView *aView, bool aAutoZIndex, PRInt32 aZindex, bool aTopMost = false) = 0;

  






  NS_IMETHOD  SetViewFloating(nsIView *aView, bool aFloatingView) = 0;

  



  virtual void SetPresShell(nsIPresShell *aPresShell) = 0;

  


  virtual nsIPresShell* GetPresShell() = 0;

  



  NS_IMETHOD  GetDeviceContext(nsDeviceContext *&aContext) = 0;

  











  class NS_STACK_CLASS AutoDisableRefresh {
  public:
    AutoDisableRefresh(nsIViewManager* aVM) {
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

    nsCOMPtr<nsIViewManager> mRootVM;
  };

private:
  friend class AutoDisableRefresh;

  virtual nsIViewManager* IncrementDisableRefreshCount() = 0;
  virtual void DecrementDisableRefreshCount() = 0;

public:
  



  NS_IMETHOD GetRootWidget(nsIWidget **aWidget) = 0;

  





  NS_IMETHOD IsPainting(bool& aIsPainting)=0;

  






  NS_IMETHOD GetLastUserEventTime(PRUint32& aTime)=0;

  



  static nsIView* GetDisplayRootFor(nsIView* aView);

  



  virtual void ProcessPendingUpdates()=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIViewManager, NS_IVIEWMANAGER_IID)

#endif  
