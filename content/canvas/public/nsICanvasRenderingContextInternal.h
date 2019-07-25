




































#ifndef nsICanvasRenderingContextInternal_h___
#define nsICanvasRenderingContextInternal_h___

#include "nsISupports.h"
#include "nsIInputStream.h"
#include "nsIDocShell.h"
#include "gfxPattern.h"
#include "mozilla/RefPtr.h"

#define NS_ICANVASRENDERINGCONTEXTINTERNAL_IID \
{ 0xffb42d3c, 0x8281, 0x44c8, \
  { 0xac, 0xba, 0x73, 0x15, 0x31, 0xaa, 0xe5, 0x07 } }

class nsHTMLCanvasElement;
class gfxContext;
class gfxASurface;
class nsIPropertyBag;
class nsDisplayListBuilder;

namespace mozilla {
namespace layers {
class CanvasLayer;
class LayerManager;
}
namespace ipc {
class Shmem;
}
namespace gfx {
class SourceSurface;
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
  
  
  
  
  virtual mozilla::TemporaryRef<mozilla::gfx::SourceSurface> GetSurfaceSnapshot() = 0;

  
  
  
  
  NS_IMETHOD SetIsOpaque(PRBool isOpaque) = 0;

  
  
  NS_IMETHOD Reset() = 0;

  
  
  virtual already_AddRefed<CanvasLayer> GetCanvasLayer(nsDisplayListBuilder* aBuilder,
                                                       CanvasLayer *aOldLayer,
                                                       LayerManager *aManager) = 0;

  virtual void MarkContextClean() = 0;

  
  NS_IMETHOD Redraw(const gfxRect &dirty) = 0;

  
  
  NS_IMETHOD SetContextOptions(nsIPropertyBag *aNewOptions) { return NS_OK; }

  
  
  

  
  
  
  
  NS_IMETHOD SetIsIPC(PRBool isIPC) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICanvasRenderingContextInternal,
                              NS_ICANVASRENDERINGCONTEXTINTERNAL_IID)

#endif 
