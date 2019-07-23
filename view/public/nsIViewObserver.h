




































#ifndef nsIViewObserver_h___
#define nsIViewObserver_h___

#include "nsISupports.h"
#include "nsEvent.h"
#include "nsColor.h"
#include "nsRect.h"

class nsIRenderingContext;
class nsGUIEvent;

#define NS_IVIEWOBSERVER_IID  \
  { 0xc85d474d, 0x316e, 0x491c, \
    { 0x8b, 0xc5, 0x24, 0xba, 0xb7, 0xbb, 0x68, 0x9e } }

class nsIViewObserver : public nsISupports
{
public:
  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IVIEWOBSERVER_IID)

  










  NS_IMETHOD Paint(nsIView*             aRootView,
                   nsIRenderingContext* aRenderingContext,
                   const nsRegion&      aDirtyRegion) = 0;

  














  NS_IMETHOD PaintDefaultBackground(nsIView*             aRootView,
                                    nsIRenderingContext* aRenderingContext,
                                    const nsRect&        aDirtyRect) = 0;

  


  NS_IMETHOD ComputeRepaintRegionForCopy(nsIView*      aRootView,
                                         nsIView*      aMovingView,
                                         nsPoint       aDelta,
                                         const nsRect& aUpdateRect,
                                         nsRegion*     aBlitRegion,
                                         nsRegion*     aRepaintRegion) = 0;

  










  NS_IMETHOD HandleEvent(nsIView*       aView,
                         nsGUIEvent*    aEvent,
                         nsEventStatus* aEventStatus) = 0;

  





  NS_IMETHOD ResizeReflow(nsIView * aView, nscoord aWidth, nscoord aHeight) = 0;

  



  NS_IMETHOD_(PRBool) IsVisible() = 0;

  




  NS_IMETHOD_(void) WillPaint() = 0;

  



  NS_IMETHOD_(void) InvalidateFrameForScrolledView(nsIView *aView) = 0;

  




  NS_IMETHOD_(void) NotifyInvalidateForScrolledView(const nsRegion& aBlitRegion,
                                                    const nsRegion& aInvalidateRegion) = 0;

  




  NS_IMETHOD_(void) DispatchSynthMouseMove(nsGUIEvent *aEvent,
                                           PRBool aFlushOnHoverChange) = 0;

  



  NS_IMETHOD_(void) ClearMouseCapture(nsIView* aView) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIViewObserver, NS_IVIEWOBSERVER_IID)

#endif
