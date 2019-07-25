




































#ifndef nsIViewObserver_h___
#define nsIViewObserver_h___

#include "nsISupports.h"
#include "nsEvent.h"
#include "nsColor.h"
#include "nsRect.h"

class nsRenderingContext;
class nsGUIEvent;
class nsIWidget;
class nsRegion;
class nsIntRegion;

#define NS_IVIEWOBSERVER_IID  \
  { 0x0d7ea18f, 0xc154, 0x4e25, \
    { 0x81, 0x0c, 0x5d, 0x60, 0x31, 0xd0, 0xac, 0xc3 } }

class nsIViewObserver : public nsISupports
{
public:
  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IVIEWOBSERVER_IID)

  



















  NS_IMETHOD Paint(nsIView*           aViewToPaint,
                   nsIWidget*         aWidgetToPaint,
                   const nsRegion&    aDirtyRegion,
                   const nsIntRegion& aIntDirtyRegion,
                   bool               aPaintDefaultBackground,
                   bool               aWillSendDidPaint) = 0;

  









  NS_IMETHOD HandleEvent(nsIFrame*      aFrame,
                         nsGUIEvent*    aEvent,
                         bool           aDontRetargetEvents,
                         nsEventStatus* aEventStatus) = 0;

  





  NS_IMETHOD ResizeReflow(nsIView * aView, nscoord aWidth, nscoord aHeight) = 0;

  




  NS_IMETHOD_(bool) ShouldIgnoreInvalidation() = 0;

  




  NS_IMETHOD_(void) WillPaint(bool aWillSendDidPaint) = 0;

  




  NS_IMETHOD_(void) DidPaint() = 0;

  




  NS_IMETHOD_(void) DispatchSynthMouseMove(nsGUIEvent *aEvent,
                                           bool aFlushOnHoverChange) = 0;

  



  NS_IMETHOD_(void) ClearMouseCapture(nsIView* aView) = 0;

  


  NS_IMETHOD_(bool) IsVisible() = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIViewObserver, NS_IVIEWOBSERVER_IID)

#endif
