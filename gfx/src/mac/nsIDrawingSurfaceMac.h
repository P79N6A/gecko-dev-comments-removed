




































#ifndef nsIDrawingSurfaceMac_h___
#define nsIDrawingSurfaceMac_h___

#include "nsIDrawingSurface.h"
#include "nsIWidget.h"
#include "nsIRenderingContext.h"
#include <QDOffscreen.h>

class GraphicsState;



#define NS_IDRAWING_SURFACE_MAC_IID   \
{ 0xd49598bb, 0x04ff, 0x4aba, \
 { 0xab, 0x90, 0x5b, 0xd0, 0xdd, 0x82, 0xba, 0xef } }

class nsIDrawingSurfaceMac : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDRAWING_SURFACE_MAC_IID)

  





  NS_IMETHOD Init(nsIDrawingSurface* aDS) = 0;

  





  NS_IMETHOD Init(CGrafPtr aPort) = 0;

  





	NS_IMETHOD Init(nsIWidget *aTheWidget) = 0;

  







  NS_IMETHOD Init(PRUint32 aDepth, PRUint32 aWidth, PRUint32 aHeight,PRUint32 aFlags) = 0;

  




  NS_IMETHOD GetGrafPtr(CGrafPtr *aPort) = 0;

  





  NS_IMETHOD_(CGContextRef) StartQuartzDrawing() = 0;

  





  NS_IMETHOD_(void) EndQuartzDrawing(CGContextRef aContext) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDrawingSurfaceMac,
                              NS_IDRAWING_SURFACE_MAC_IID)

#endif  
