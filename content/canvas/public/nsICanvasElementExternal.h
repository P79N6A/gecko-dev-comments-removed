




































#ifndef nsICanvasElementExternal_h___
#define nsICanvasElementExternal_h___

#include "nsISupports.h"
#include "gfxPattern.h"

class gfxContext;
class nsIFrame;
struct gfxRect;

#define NS_ICANVASELEMENTEXTERNAL_IID \
  { 0x51870f54, 0x6c4c, 0x469a, {0xad, 0x46, 0xf0, 0xa9, 0x8e, 0x32, 0xa7, 0xe2 } }

class nsRenderingContext;
class nsICanvasRenderingContextInternal;

struct _cairo_surface;











class nsICanvasElementExternal : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICANVASELEMENTEXTERNAL_IID)

  


  NS_IMETHOD_(nsIntSize) GetSizeExternal() = 0;

  



  NS_IMETHOD RenderContextsExternal(gfxContext *ctx,
                                    gfxPattern::GraphicsFilter aFilter) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICanvasElementExternal, NS_ICANVASELEMENTEXTERNAL_IID)

#endif 
