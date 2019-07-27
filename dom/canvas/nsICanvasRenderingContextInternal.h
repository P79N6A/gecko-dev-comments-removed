




#ifndef nsICanvasRenderingContextInternal_h___
#define nsICanvasRenderingContextInternal_h___

#include "mozilla/gfx/2D.h"
#include "nsISupports.h"
#include "nsIInputStream.h"
#include "nsIDocShell.h"
#include "nsRefreshDriver.h"
#include "mozilla/dom/HTMLCanvasElement.h"
#include "GraphicsFilter.h"
#include "mozilla/RefPtr.h"

#define NS_ICANVASRENDERINGCONTEXTINTERNAL_IID \
{ 0x3cc9e801, 0x1806, 0x4ff6, \
  { 0x86, 0x14, 0xf9, 0xd0, 0xf4, 0xfb, 0x3b, 0x08 } }

class gfxASurface;
class nsDisplayListBuilder;

namespace mozilla {
namespace layers {
class CanvasLayer;
class LayerManager;
}
namespace gfx {
class SourceSurface;
}
}

class nsICanvasRenderingContextInternal :
  public nsISupports,
  public nsAPostRefreshObserver
{
public:
  typedef mozilla::layers::CanvasLayer CanvasLayer;
  typedef mozilla::layers::LayerManager LayerManager;

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICANVASRENDERINGCONTEXTINTERNAL_IID)

  void SetCanvasElement(mozilla::dom::HTMLCanvasElement* aParentCanvas)
  {
    RemovePostRefreshObserver();
    mCanvasElement = aParentCanvas;
    AddPostRefreshObserverIfNecessary();
  }

  virtual nsIPresShell *GetPresShell() {
    if (mCanvasElement) {
      return mCanvasElement->OwnerDoc()->GetShell();
    }
    return nullptr;
  }

  void RemovePostRefreshObserver()
  {
    if (mRefreshDriver) {
      mRefreshDriver->RemovePostRefreshObserver(this);
      mRefreshDriver = nullptr;
    }
  }

  void AddPostRefreshObserverIfNecessary()
  {
    if (!GetPresShell() ||
        !GetPresShell()->GetPresContext() ||
        !GetPresShell()->GetPresContext()->RefreshDriver()) {
      return;
    }
    mRefreshDriver = GetPresShell()->GetPresContext()->RefreshDriver();
    mRefreshDriver->AddPostRefreshObserver(this);
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

  
  virtual void GetImageBuffer(uint8_t** aImageBuffer, int32_t* aFormat) = 0;

  
  
  
  
  
  
  NS_IMETHOD GetInputStream(const char *aMimeType,
                            const char16_t *aEncoderOptions,
                            nsIInputStream **aStream) = 0;
  
  
  
  
  
  
  virtual mozilla::TemporaryRef<mozilla::gfx::SourceSurface> GetSurfaceSnapshot(bool* aPremultAlpha = nullptr) = 0;

  
  
  
  
  NS_IMETHOD SetIsOpaque(bool isOpaque) = 0;
  virtual bool GetIsOpaque() = 0;

  
  
  NS_IMETHOD Reset() = 0;

  
  
  virtual already_AddRefed<CanvasLayer> GetCanvasLayer(nsDisplayListBuilder* aBuilder,
                                                       CanvasLayer *aOldLayer,
                                                       LayerManager *aManager) = 0;

  
  
  
  virtual bool ShouldForceInactiveLayer(LayerManager *aManager) { return false; }

  virtual void MarkContextClean() = 0;

  
  NS_IMETHOD Redraw(const gfxRect &dirty) = 0;

  NS_IMETHOD SetContextOptions(JSContext* aCx, JS::Handle<JS::Value> aOptions) { return NS_OK; }

  
  virtual bool GetHitRegionRect(mozilla::dom::Element* aElement, nsRect& aRect) { return false; }

  
  virtual nsString GetHitRegion(const mozilla::gfx::Point& aPoint) { return nsString(); }

  
  
  

  
  
  
  
  NS_IMETHOD SetIsIPC(bool isIPC) = 0;

protected:
  nsRefPtr<mozilla::dom::HTMLCanvasElement> mCanvasElement;
  nsRefPtr<nsRefreshDriver> mRefreshDriver;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICanvasRenderingContextInternal,
                              NS_ICANVASRENDERINGCONTEXTINTERNAL_IID)

#endif 
