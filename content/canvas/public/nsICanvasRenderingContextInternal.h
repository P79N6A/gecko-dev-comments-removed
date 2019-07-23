




































#ifndef nsICanvasRenderingContextInternal_h___
#define nsICanvasRenderingContextInternal_h___

#include "nsISupports.h"
#include "nsICanvasElement.h"
#include "nsIInputStream.h"


#define NS_ICANVASRENDERINGCONTEXTINTERNAL_IID \
  { 0x5150761, 0x22a3, 0x4e8d, { 0xa0, 0x3e, 0xec, 0x53, 0xcb, 0x73, 0x1c, 0x70 } }

class nsIRenderingContext;

struct _cairo_surface;

class nsICanvasRenderingContextInternal : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICANVASRENDERINGCONTEXTINTERNAL_IID)

  
  
  NS_IMETHOD SetCanvasElement(nsICanvasElement* aParentCanvas) = 0;

  
  
  NS_IMETHOD SetDimensions(PRInt32 width, PRInt32 height) = 0;

  
  NS_IMETHOD Render(nsIRenderingContext *rc) = 0;

  
  NS_IMETHOD RenderToSurface(struct _cairo_surface *surf) = 0;

  
  
  
  
  
  
  NS_IMETHOD GetInputStream(const nsACString& aMimeType,
                            const nsAString& aEncoderOptions,
                            nsIInputStream **aStream) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICanvasRenderingContextInternal,
                              NS_ICANVASRENDERINGCONTEXTINTERNAL_IID)

#endif 
