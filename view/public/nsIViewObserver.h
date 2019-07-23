




































#ifndef nsIViewObserver_h___
#define nsIViewObserver_h___

#include "nsISupports.h"
#include "nsEvent.h"
#include "nsColor.h"
#include "nsRect.h"

class nsIRenderingContext;
class nsGUIEvent;


#define NS_IVIEWOBSERVER_IID  \
{ 0x8d7ae493, 0x1eb1, 0x4d38, \
  { 0x89, 0xda, 0x9e, 0xee, 0xaa, 0x29, 0xfd, 0x79 } }

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

  



  NS_IMETHOD_(void) InvalidateFrameForView(nsIView *aView) = 0;

  




  NS_IMETHOD_(void) DispatchSynthMouseMove(nsGUIEvent *aEvent,
                                           PRBool aFlushOnHoverChange) = 0;

  



  NS_IMETHOD_(void) ClearMouseCapture(nsIView* aView) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIViewObserver, NS_IVIEWOBSERVER_IID)

#endif
