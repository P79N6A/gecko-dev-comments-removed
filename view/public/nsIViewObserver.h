




































#ifndef nsIViewObserver_h___
#define nsIViewObserver_h___

#include "nsISupports.h"
#include "nsEvent.h"
#include "nsColor.h"
#include "nsRect.h"

class nsIRenderingContext;
class nsGUIEvent;

#define NS_IVIEWOBSERVER_IID  \
  { 0x8e69db48, 0x9d01, 0x4c0a, \
    { 0xb9, 0xea, 0xa4, 0x4b, 0xc5, 0x89, 0xc8, 0x63 } }

class nsIViewObserver : public nsISupports
{
public:
  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IVIEWOBSERVER_IID)

  














  NS_IMETHOD Paint(nsIView*        aDisplayRoot,
                   nsIView*        aViewToPaint,
                   nsIWidget*      aWidgetToPaint,
                   const nsRegion& aDirtyRegion,
                   PRBool          aPaintDefaultBackground,
                   PRBool          aWillSendDidPaint) = 0;

  










  NS_IMETHOD HandleEvent(nsIView*       aView,
                         nsGUIEvent*    aEvent,
                         nsEventStatus* aEventStatus) = 0;

  





  NS_IMETHOD ResizeReflow(nsIView * aView, nscoord aWidth, nscoord aHeight) = 0;

  



  NS_IMETHOD_(PRBool) IsVisible() = 0;

  




  NS_IMETHOD_(void) WillPaint(PRBool aWillSendDidPaint) = 0;

  




  NS_IMETHOD_(void) DidPaint() = 0;

  




  NS_IMETHOD_(void) DispatchSynthMouseMove(nsGUIEvent *aEvent,
                                           PRBool aFlushOnHoverChange) = 0;

  



  NS_IMETHOD_(void) ClearMouseCapture(nsIView* aView) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIViewObserver, NS_IVIEWOBSERVER_IID)

#endif
