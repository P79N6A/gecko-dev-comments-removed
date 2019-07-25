




































#ifndef nsIViewObserver_h___
#define nsIViewObserver_h___

#include "nsISupports.h"
#include "nsEvent.h"
#include "nsColor.h"
#include "nsRect.h"

class nsIRenderingContext;
class nsGUIEvent;

#define NS_IVIEWOBSERVER_IID  \
  { 0x4d467c73, 0xb6a9, 0x462a, \
    { 0x90, 0x25, 0x80, 0xd9, 0x42, 0xbc, 0xcc, 0xb5 } }

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

  




  NS_IMETHOD_(PRBool) ShouldIgnoreInvalidation() = 0;

  




  NS_IMETHOD_(void) WillPaint(PRBool aWillSendDidPaint) = 0;

  




  NS_IMETHOD_(void) DidPaint() = 0;

  




  NS_IMETHOD_(void) DispatchSynthMouseMove(nsGUIEvent *aEvent,
                                           PRBool aFlushOnHoverChange) = 0;

  



  NS_IMETHOD_(void) ClearMouseCapture(nsIView* aView) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIViewObserver, NS_IVIEWOBSERVER_IID)

#endif
