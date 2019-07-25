




































#ifndef nsIViewObserver_h___
#define nsIViewObserver_h___

#include "nsISupports.h"
#include "nsEvent.h"
#include "nsColor.h"
#include "nsRect.h"

class nsIRenderingContext;
class nsGUIEvent;

#define NS_IVIEWOBSERVER_IID  \
  { 0x6af699da, 0x8bfe, 0x43c9, \
    { 0xae, 0xc1, 0x76, 0x1b, 0x03, 0x62, 0x8d, 0x64 } }

class nsIViewObserver : public nsISupports
{
public:
  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IVIEWOBSERVER_IID)

  



















  NS_IMETHOD Paint(nsIView*           aDisplayRoot,
                   nsIView*           aViewToPaint,
                   nsIWidget*         aWidgetToPaint,
                   const nsRegion&    aDirtyRegion,
                   const nsIntRegion& aIntDirtyRegion,
                   PRBool             aPaintDefaultBackground,
                   PRBool             aWillSendDidPaint) = 0;

  










  NS_IMETHOD HandleEvent(nsIView*       aView,
                         nsGUIEvent*    aEvent,
                         nsEventStatus* aEventStatus) = 0;

  





  NS_IMETHOD ResizeReflow(nsIView * aView, nscoord aWidth, nscoord aHeight) = 0;

  



  NS_IMETHOD_(PRBool) IsVisible() = 0;

  




  NS_IMETHOD_(PRBool) ShouldIgnoreInvalidation() = 0;

  




  NS_IMETHOD_(void) WillPaint(PRBool aWillSendDidPaint) = 0;

  




  NS_IMETHOD_(void) DidPaint() = 0;

  




  NS_IMETHOD_(void) DispatchSynthMouseMove(nsGUIEvent *aEvent,
                                           PRBool aFlushOnHoverChange) = 0;

  



  NS_IMETHOD_(void) ClearMouseCapture(nsIView* aView) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIViewObserver, NS_IVIEWOBSERVER_IID)

#endif
