




































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

  


  
  
  
  
  
  NS_IMETHOD  Composite(void) = 0;

  





  NS_IMETHOD  UpdateView(nsIView *aView, PRUint32 aUpdateFlags) = 0;

  







  NS_IMETHOD  UpdateViewNoSuppression(nsIView *aView, const nsRect &aRect,
                                      PRUint32 aUpdateFlags) = 0;

  




  NS_IMETHOD  UpdateAllViews(PRUint32 aUpdateFlags) = 0;

  







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

  class UpdateViewBatch {
  public:
    UpdateViewBatch() {}
  






    UpdateViewBatch(nsIViewManager* aVM) {
      if (aVM) {
        mRootVM = aVM->BeginUpdateViewBatch();
      }
    }
    ~UpdateViewBatch() {
      NS_ASSERTION(!mRootVM, "Someone forgot to call EndUpdateViewBatch!");
    }
    
    


    void BeginUpdateViewBatch(nsIViewManager* aVM) {
      NS_ASSERTION(!mRootVM, "already started a batch!");
      if (aVM) {
        mRootVM = aVM->BeginUpdateViewBatch();
      }
    }

  





















    void EndUpdateViewBatch(PRUint32 aUpdateFlags) {
      if (!mRootVM)
        return;
      mRootVM->EndUpdateViewBatch(aUpdateFlags);
      mRootVM = nsnull;
    }

  private:
    UpdateViewBatch(const UpdateViewBatch& aOther);
    const UpdateViewBatch& operator=(const UpdateViewBatch& aOther);

    nsCOMPtr<nsIViewManager> mRootVM;
  };
  
private:
  friend class UpdateViewBatch;

  virtual nsIViewManager* BeginUpdateViewBatch(void) = 0;
  NS_IMETHOD EndUpdateViewBatch(PRUint32 aUpdateFlags) = 0;

public:
  



  NS_IMETHOD GetRootWidget(nsIWidget **aWidget) = 0;

  




  
  
  
  NS_IMETHOD ForceUpdate() = 0;

  





  NS_IMETHOD IsPainting(bool& aIsPainting)=0;

  






  NS_IMETHOD GetLastUserEventTime(PRUint32& aTime)=0;

  



  static nsIView* GetDisplayRootFor(nsIView* aView);
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIViewManager, NS_IVIEWMANAGER_IID)






#define NS_VMREFRESH_NO_SYNC            0





#define NS_VMREFRESH_DEFERRED           0x0001



#define NS_VMREFRESH_IMMEDIATE          0x0002

#endif  
