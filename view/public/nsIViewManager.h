




































#ifndef nsIViewManager_h___
#define nsIViewManager_h___

#include "nscore.h"
#include "nsIView.h"
#include "nsColor.h"
#include "nsEvent.h"
#include "nsIRenderingContext.h"

class nsIScrollableView;
class nsIWidget;
struct nsRect;
class nsRegion;
class nsIDeviceContext;
class nsIViewObserver;

enum nsRectVisibility { 
  nsRectVisibility_kVisible, 
  nsRectVisibility_kAboveViewport, 
  nsRectVisibility_kBelowViewport, 
  nsRectVisibility_kLeftOfViewport, 
  nsRectVisibility_kRightOfViewport, 
  nsRectVisibility_kZeroAreaRect
}; 


#define NS_IVIEWMANAGER_IID   \
{ 0x8d6c85d1, 0x5e2a, 0x4510, \
  { 0xaf, 0xf0, 0x50, 0x56, 0x76, 0xab, 0x9b, 0x48 } }

class nsIViewManager : public nsISupports
{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IVIEWMANAGER_IID)
  





  NS_IMETHOD  Init(nsIDeviceContext* aContext) = 0;

  











  NS_IMETHOD_(nsIView*) CreateView(const nsRect& aBounds,
                                   const nsIView* aParent,
                                   nsViewVisibility aVisibilityFlag = nsViewVisibility_kShow) = 0;

  









  NS_IMETHOD_(nsIScrollableView*) CreateScrollableView(const nsRect& aBounds,
                                                       const nsIView* aParent) = 0;

  



  NS_IMETHOD  GetRootView(nsIView *&aView) = 0;

  





  NS_IMETHOD  SetRootView(nsIView *aView) = 0;

  





  NS_IMETHOD  GetWindowDimensions(nscoord *aWidth, nscoord *aHeight) = 0;

  






  NS_IMETHOD  SetWindowDimensions(nscoord aWidth, nscoord aHeight) = 0;

  


  
  
  
  
  
  NS_IMETHOD  Composite(void) = 0;

  





  NS_IMETHOD  UpdateView(nsIView *aView, PRUint32 aUpdateFlags) = 0;

  







  NS_IMETHOD  UpdateView(nsIView *aView, const nsRect &aRect, PRUint32 aUpdateFlags) = 0;

  




  NS_IMETHOD  UpdateAllViews(PRUint32 aUpdateFlags) = 0;

  






  NS_IMETHOD  DispatchEvent(nsGUIEvent *aEvent, nsEventStatus* aStatus) = 0;

  






  NS_IMETHOD  GrabMouseEvents(nsIView *aView, PRBool& aResult) = 0;

  



  NS_IMETHOD  GetMouseEventGrabber(nsIView *&aView) = 0;

  














  NS_IMETHOD  InsertChild(nsIView *aParent, nsIView *aChild, nsIView *aSibling,
                          PRBool aAfter) = 0;

  






  NS_IMETHOD  RemoveChild(nsIView *aChild) = 0;

  








  NS_IMETHOD  MoveViewTo(nsIView *aView, nscoord aX, nscoord aY) = 0;

  











  NS_IMETHOD  ResizeView(nsIView *aView, const nsRect &aRect,
                         PRBool aRepaintExposedAreaOnly = PR_FALSE) = 0;

  





  NS_IMETHOD  SetViewVisibility(nsIView *aView, nsViewVisibility aVisible) = 0;

  















  NS_IMETHOD  SetViewZIndex(nsIView *aView, PRBool aAutoZIndex, PRInt32 aZindex, PRBool aTopMost = PR_FALSE) = 0;

  






  NS_IMETHOD  SetViewFloating(nsIView *aView, PRBool aFloatingView) = 0;

  




  NS_IMETHOD SetViewObserver(nsIViewObserver *aObserver) = 0;

  




  NS_IMETHOD GetViewObserver(nsIViewObserver *&aObserver) = 0;

  



  NS_IMETHOD  GetDeviceContext(nsIDeviceContext *&aContext) = 0;

  



  
  
  
  NS_IMETHOD DisableRefresh(void) = 0;

  





  NS_IMETHOD EnableRefresh(PRUint32 aUpdateFlags) = 0;

  





  NS_IMETHOD BeginUpdateViewBatch(void) = 0;

  





















  NS_IMETHOD EndUpdateViewBatch(PRUint32 aUpdateFlags) = 0;

  





  NS_IMETHOD SetRootScrollableView(nsIScrollableView *aScrollable) = 0;

  





  NS_IMETHOD GetRootScrollableView(nsIScrollableView **aScrollable) = 0;

  




  NS_IMETHOD GetWidget(nsIWidget **aWidget) = 0;

  




  
  
  
  NS_IMETHOD ForceUpdate() = 0;

  





  NS_IMETHOD IsPainting(PRBool& aIsPainting)=0;

  






  NS_IMETHOD SetDefaultBackgroundColor(nscolor aColor)=0;

  




  NS_IMETHOD GetDefaultBackgroundColor(nscolor* aColor)=0;

  






  NS_IMETHOD GetLastUserEventTime(PRUint32& aTime)=0;

  









  NS_IMETHOD GetRectVisibility(nsIView *aView, const nsRect &aRect, 
                               PRUint16 aMinTwips, 
                               nsRectVisibility *aRectVisibility)=0;

  




  NS_IMETHOD SynthesizeMouseMove(PRBool aFromScroll)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIViewManager, NS_IVIEWMANAGER_IID)






#define NS_VMREFRESH_NO_SYNC            0





#define NS_VMREFRESH_DEFERRED           0x0001



#define NS_VMREFRESH_IMMEDIATE          0x0002


#define NS_VMREFRESH_SMOOTHSCROLL       0x0008

#endif  
