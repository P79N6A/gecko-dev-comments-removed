




































#ifndef nsIViewObserver_h___
#define nsIViewObserver_h___

#include "nsISupports.h"
#include "nsEvent.h"
#include "nsColor.h"
#include "nsRect.h"

class nsIRenderingContext;
class nsGUIEvent;


#define NS_IVIEWOBSERVER_IID  \
{ 0x52b3b616, 0x23a9, 0x4516, \
  { 0xa8, 0xd3, 0x45, 0x2b, 0x41, 0x26, 0xeb, 0x2b } }

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
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIViewObserver, NS_IVIEWOBSERVER_IID)

#endif
