




































#ifndef nsICanvasRenderingContextInternal_h___
#define nsICanvasRenderingContextInternal_h___

#include "nsISupports.h"
#include "nsIInputStream.h"
#include "nsIDocShell.h"
#include "gfxPattern.h"


#define NS_ICANVASRENDERINGCONTEXTINTERNAL_IID \
{ 0xb96168fd, 0x6f13, 0x4ca7, \
  { 0xb8, 0x20, 0xe9, 0x6f, 0x22, 0xe7, 0x1f, 0xe5 } }

class nsHTMLCanvasElement;
class gfxContext;
class gfxASurface;

namespace mozilla {
namespace layers {
class CanvasLayer;
class LayerManager;
}
namespace ipc {
class Shmem;
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

  
  
  virtual already_AddRefed<CanvasLayer> GetCanvasLayer(CanvasLayer *aOldLayer,
                                                       LayerManager *aManager) = 0;

  virtual void MarkContextClean() = 0;

  
  NS_IMETHOD Redraw(const gfxRect &dirty) = 0;

  
  
  
  
  NS_IMETHOD SetIsIPC(PRBool isIPC) = 0;

  
  
  NS_IMETHOD Swap(mozilla::ipc::Shmem& back,
                  PRInt32 x, PRInt32 y, PRInt32 w, PRInt32 h) = 0;

  
  NS_IMETHOD Swap(PRUint32 nativeID,
                  PRInt32 x, PRInt32 y, PRInt32 w, PRInt32 h) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICanvasRenderingContextInternal,
                              NS_ICANVASRENDERINGCONTEXTINTERNAL_IID)

#endif 
