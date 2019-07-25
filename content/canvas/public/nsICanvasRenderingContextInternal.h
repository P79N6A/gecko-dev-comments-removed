




































#ifndef nsICanvasRenderingContextInternal_h___
#define nsICanvasRenderingContextInternal_h___

#include "nsISupports.h"
#include "nsIInputStream.h"
#include "nsIDocShell.h"
#include "gfxPattern.h"


#define NS_ICANVASRENDERINGCONTEXTINTERNAL_IID \
  { 0xed741c16, 0x4039, 0x469b, { 0x91, 0xda, 0xdc, 0xa7, 0x42, 0xc5, 0x1a, 0x9f } }

class nsHTMLCanvasElement;
class gfxContext;
class gfxASurface;

namespace mozilla {
namespace layers {
class CanvasLayer;
class LayerManager;
}
}

class nsICanvasRenderingContextInternal : public nsISupports {
public:
  typedef mozilla::layers::CanvasLayer CanvasLayer;
  typedef mozilla::layers::LayerManager LayerManager;

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICANVASRENDERINGCONTEXTINTERNAL_IID)

  
  
  NS_IMETHOD SetCanvasElement(nsHTMLCanvasElement* aParentCanvas) = 0;

  
  
  NS_IMETHOD SetDimensions(PRInt32 width, PRInt32 height) = 0;

  NS_IMETHOD InitializeWithSurface(nsIDocShell *docShell, gfxASurface *surface, PRInt32 width, PRInt32 height) = 0;

  
  NS_IMETHOD Render(gfxContext *ctx, gfxPattern::GraphicsFilter aFilter) = 0;

  
  
  
  
  
  
  NS_IMETHOD GetInputStream(const char *aMimeType,
                            const PRUnichar *aEncoderOptions,
                            nsIInputStream **aStream) = 0;

  
  
  NS_IMETHOD GetThebesSurface(gfxASurface **surface) = 0;

  
  
  
  
  NS_IMETHOD SetIsOpaque(PRBool isOpaque) = 0;

  
  
  virtual already_AddRefed<CanvasLayer> GetCanvasLayer(LayerManager *mgr) = 0;

  virtual void MarkContextClean() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICanvasRenderingContextInternal,
                              NS_ICANVASRENDERINGCONTEXTINTERNAL_IID)

#endif 
