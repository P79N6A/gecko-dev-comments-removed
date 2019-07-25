




#ifndef nsICanvasRenderingContextInternal_h___
#define nsICanvasRenderingContextInternal_h___

#include "nsISupports.h"
#include "nsIInputStream.h"
#include "nsIDocShell.h"
#include "nsHTMLCanvasElement.h"
#include "gfxPattern.h"
#include "mozilla/RefPtr.h"

#define NS_ICANVASRENDERINGCONTEXTINTERNAL_IID \
{ 0x8b8da863, 0xd151, 0x4014, \
  { 0x8b, 0xdc, 0x62, 0xb5, 0x0d, 0xc0, 0x2b, 0x62 } }

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

  enum {
    RenderFlagPremultAlpha = 0x1
  };

  void SetCanvasElement(nsHTMLCanvasElement* aParentCanvas)
  {
    mCanvasElement = aParentCanvas;
  }
  nsHTMLCanvasElement* GetParentObject() const
  {
    return mCanvasElement;
  }

  
  
  NS_IMETHOD SetDimensions(PRInt32 width, PRInt32 height) = 0;

  NS_IMETHOD InitializeWithSurface(nsIDocShell *docShell, gfxASurface *surface, PRInt32 width, PRInt32 height) = 0;

  
  NS_IMETHOD Render(gfxContext *ctx,
                    gfxPattern::GraphicsFilter aFilter,
                    PRUint32 aFlags = RenderFlagPremultAlpha) = 0;

  
  
  
  
  
  
  NS_IMETHOD GetInputStream(const char *aMimeType,
                            const PRUnichar *aEncoderOptions,
                            nsIInputStream **aStream) = 0;

  
  
  NS_IMETHOD GetThebesSurface(gfxASurface **surface) = 0;
  
  
  
  
  virtual mozilla::TemporaryRef<mozilla::gfx::SourceSurface> GetSurfaceSnapshot() = 0;

  
  
  
  
  NS_IMETHOD SetIsOpaque(bool isOpaque) = 0;

  
  
  NS_IMETHOD Reset() = 0;

  
  
  virtual already_AddRefed<CanvasLayer> GetCanvasLayer(nsDisplayListBuilder* aBuilder,
                                                       CanvasLayer *aOldLayer,
                                                       LayerManager *aManager) = 0;

  
  
  
  virtual bool ShouldForceInactiveLayer(LayerManager *aManager) { return false; }

  virtual void MarkContextClean() = 0;

  
  NS_IMETHOD Redraw(const gfxRect &dirty) = 0;

  
  
  NS_IMETHOD SetContextOptions(nsIPropertyBag *aNewOptions) { return NS_OK; }

  
  
  

  
  
  
  
  NS_IMETHOD SetIsIPC(bool isIPC) = 0;

protected:
  nsRefPtr<nsHTMLCanvasElement> mCanvasElement;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICanvasRenderingContextInternal,
                              NS_ICANVASRENDERINGCONTEXTINTERNAL_IID)

#endif 
