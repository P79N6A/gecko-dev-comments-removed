




































#ifndef nsIViewObserver_h___
#define nsIViewObserver_h___

#include "nsISupports.h"
#include "nsEvent.h"
#include "nsColor.h"
#include "nsRect.h"

class nsIRenderingContext;
class nsGUIEvent;

#define NS_IVIEWOBSERVER_IID  \
  { 0xac43a985, 0xcae6, 0x499d, \
    { 0xae, 0x8f, 0x9c, 0x92, 0xec, 0x6f, 0x2c, 0x47 } }

class nsIViewObserver : public nsISupports
{
public:
  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IVIEWOBSERVER_IID)

  














  NS_IMETHOD Paint(nsIView*        aDisplayRoot,
                   nsIView*        aViewToPaint,
                   nsIWidget*      aWidgetToPaint,
                   const nsRegion& aDirtyRegion,
                   PRBool          aPaintDefaultBackground) = 0;

  










  NS_IMETHOD HandleEvent(nsIView*       aView,
                         nsGUIEvent*    aEvent,
                         nsEventStatus* aEventStatus) = 0;

  





  NS_IMETHOD ResizeReflow(nsIView * aView, nscoord aWidth, nscoord aHeight) = 0;

  



  NS_IMETHOD_(PRBool) IsVisible() = 0;

  




  NS_IMETHOD_(void) WillPaint() = 0;

  




  NS_IMETHOD_(void) DispatchSynthMouseMove(nsGUIEvent *aEvent,
                                           PRBool aFlushOnHoverChange) = 0;

  



  NS_IMETHOD_(void) ClearMouseCapture(nsIView* aView) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIViewObserver, NS_IVIEWOBSERVER_IID)

#endif
