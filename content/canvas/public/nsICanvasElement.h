




































#ifndef nsICanvasElement_h___
#define nsICanvasElement_h___

#include "nsISupports.h"
#include "nsIFrame.h"


#define NS_ICANVASELEMENT_IID \
    { 0xc234660c, 0xbd06, 0x493e, { 0x85, 0x83, 0x93, 0x9a, 0x5a, 0x15, 0x8b, 0x37 } }

class nsIRenderingContext;

struct _cairo_surface;

class nsICanvasElement : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICANVASELEMENT_IID)

  


  NS_IMETHOD GetPrimaryCanvasFrame (nsIFrame **aFrame) = 0;

  


  NS_IMETHOD GetSize (PRUint32 *width, PRUint32 *height) = 0;

  



  NS_IMETHOD RenderContexts (nsIRenderingContext *rc) = 0;

  



  NS_IMETHOD RenderContextsToSurface (struct _cairo_surface *surf) = 0;
  
  


  virtual PRBool IsWriteOnly() = 0;

  


  virtual void SetWriteOnly() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICanvasElement, NS_ICANVASELEMENT_IID)

#endif 
