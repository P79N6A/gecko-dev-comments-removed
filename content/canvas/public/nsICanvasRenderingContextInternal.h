




#ifndef nsICanvasRenderingContextInternal_h___
#define nsICanvasRenderingContextInternal_h___

#include "nsISupports.h"
#include "nsIInputStream.h"
#include "nsIDocShell.h"
#include "mozilla/dom/HTMLCanvasElement.h"
#include "GraphicsFilter.h"
#include "mozilla/RefPtr.h"

#define NS_ICANVASRENDERINGCONTEXTINTERNAL_IID \
{ 0x9a6a5bdf, 0x1261, 0x4057, \
  { 0x85, 0xcc, 0xaf, 0x97, 0x6c, 0x36, 0x99, 0xa9 } }

class gfxContext;
class gfxASurface;
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

  void SetCanvasElement(mozilla::dom::HTMLCanvasElement* aParentCanvas)
  {
    mCanvasElement = aParentCanvas;
  }
  mozilla::dom::HTMLCanvasElement* GetParentObject() const
  {
    return mCanvasElement;
  }

#ifdef DEBUG
    
    virtual int32_t GetWidth() const = 0;
    virtual int32_t GetHeight() const = 0;
#endif

  
  
  NS_IMETHOD SetDimensions(int32_t width, int32_t height) = 0;

  NS_IMETHOD InitializeWithSurface(nsIDocShell *docShell, gfxASurface *surface, int32_t width, int32_t height) = 0;

  
  NS_IMETHOD Render(gfxContext *ctx,
                    GraphicsFilter aFilter,
                    uint32_t aFlags = RenderFlagPremultAlpha) = 0;

  
  virtual void GetImageBuffer(uint8_t** aImageBuffer, int32_t* aFormat) = 0;

  
  
  
  
  
  
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

  NS_IMETHOD SetContextOptions(JSContext* aCx, JS::Handle<JS::Value> aOptions) { return NS_OK; }

  
  
  

  
  
  
  
  NS_IMETHOD SetIsIPC(bool isIPC) = 0;

protected:
  nsRefPtr<mozilla::dom::HTMLCanvasElement> mCanvasElement;
};

namespace mozilla {
namespace dom {

}
}

NS_DEFINE_STATIC_IID_ACCESSOR(nsICanvasRenderingContextInternal,
                              NS_ICANVASRENDERINGCONTEXTINTERNAL_IID)

#endif 
