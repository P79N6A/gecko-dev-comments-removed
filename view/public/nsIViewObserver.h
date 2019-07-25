




































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
  { 0xac6eec35, 0x65d2, 0x4fe8, \
    { 0xa1, 0x37, 0x1a, 0xc3, 0xf6, 0x51, 0x52, 0x56 } }

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

  










  NS_IMETHOD HandleEvent(nsIView*       aView,
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
