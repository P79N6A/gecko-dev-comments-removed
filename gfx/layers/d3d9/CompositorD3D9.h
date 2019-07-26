




#ifndef MOZILLA_GFX_COMPOSITORD3D9_H
#define MOZILLA_GFX_COMPOSITORD3D9_H

#include "mozilla/gfx/2D.h"
#include "mozilla/layers/Compositor.h"
#include "mozilla/layers/TextureD3D9.h"
#include "DeviceManagerD3D9.h"

class nsWidget;

namespace mozilla {
namespace layers {

class CompositorD3D9 : public Compositor
{
public:
  CompositorD3D9(nsIWidget *aWidget);
  ~CompositorD3D9();

  virtual bool Initialize() MOZ_OVERRIDE;
  virtual void Destroy() MOZ_OVERRIDE {}

  virtual TextureFactoryIdentifier
    GetTextureFactoryIdentifier() MOZ_OVERRIDE;

  virtual bool CanUseCanvasLayerForSize(const gfx::IntSize &aSize) MOZ_OVERRIDE;
  virtual int32_t GetMaxTextureSize() const MOZ_FINAL;

  virtual void SetTargetContext(gfx::DrawTarget *aTarget) MOZ_OVERRIDE
  {
    mTarget = aTarget;
  }

  virtual void MakeCurrent(MakeCurrentFlags aFlags = 0) MOZ_OVERRIDE {}

  virtual TemporaryRef<CompositingRenderTarget>
    CreateRenderTarget(const gfx::IntRect &aRect,
                       SurfaceInitMode aInit) MOZ_OVERRIDE;

  virtual TemporaryRef<CompositingRenderTarget>
    CreateRenderTargetFromSource(const gfx::IntRect &aRect,
                                 const CompositingRenderTarget *aSource) MOZ_OVERRIDE;

  virtual void SetRenderTarget(CompositingRenderTarget *aSurface);
  virtual CompositingRenderTarget* GetCurrentRenderTarget() MOZ_OVERRIDE
  {
    return mCurrentRT;
  }

  virtual void SetDestinationSurfaceSize(const gfx::IntSize& aSize) MOZ_OVERRIDE {}

  virtual void DrawQuad(const gfx::Rect &aRect, const gfx::Rect &aClipRect,
                        const EffectChain &aEffectChain,
                        gfx::Float aOpacity, const gfx::Matrix4x4 &aTransform,
                        const gfx::Point &aOffset) MOZ_OVERRIDE;

  virtual void BeginFrame(const gfx::Rect *aClipRectIn,
                          const gfxMatrix& aTransform,
                          const gfx::Rect& aRenderBounds,
                          gfx::Rect *aClipRectOut = nullptr,
                          gfx::Rect *aRenderBoundsOut = nullptr) MOZ_OVERRIDE;

  virtual void EndFrame() MOZ_OVERRIDE;

  virtual void EndFrameForExternalComposition(const gfxMatrix& aTransform) MOZ_OVERRIDE {}

  virtual void AbortFrame() MOZ_OVERRIDE {}

  virtual void PrepareViewport(const gfx::IntSize& aSize,
                               const gfxMatrix& aWorldTransform) MOZ_OVERRIDE;

  virtual bool SupportsPartialTextureUpdate() MOZ_OVERRIDE{ return true; }

#ifdef MOZ_DUMP_PAINTING
  virtual const char* Name() const MOZ_OVERRIDE { return "Direct3D9"; }
#endif

  virtual void NotifyLayersTransaction() MOZ_OVERRIDE {}

  virtual nsIWidget* GetWidget() const MOZ_OVERRIDE { return mWidget; }
  virtual const nsIntSize& GetWidgetSize() MOZ_OVERRIDE
  {
    NS_ASSERTION(false, "Getting the widget size on windows causes some kind of resizing of buffers. "
                        "You should not do that outside of BeginFrame, so the best we can do is return "
                        "the last size we got, that might not be up to date. So you probably shouldn't "
                        "use this method.");
    return mSize;
  }

  IDirect3DDevice9* device() const { return mDeviceManager->device(); }

  



  virtual void SetScreenRenderOffset(const ScreenPoint& aOffset) MOZ_OVERRIDE
  {
    if (aOffset.x || aOffset.y) {
      NS_RUNTIMEABORT("SetScreenRenderOffset not supported by CompositorD3D9.");
    }
    
  }

   virtual TemporaryRef<DataTextureSource>
     CreateDataTextureSource(TextureFlags aFlags = 0) MOZ_OVERRIDE { return nullptr; } 
private:
  
  void EnsureSize();
  void SetSamplerForFilter(gfx::Filter aFilter);
  void PaintToTarget();
  void SetMask(const EffectChain &aEffectChain, uint32_t aMaskTexture);

  void ReportFailure(const nsACString &aMsg, HRESULT aCode);

  
  nsRefPtr<DeviceManagerD3D9> mDeviceManager;

  
  nsRefPtr<SwapChainD3D9> mSwapChain;

  
  nsIWidget *mWidget;

  


  RefPtr<gfx::DrawTarget> mTarget;

  RefPtr<CompositingRenderTargetD3D9> mDefaultRT;
  RefPtr<CompositingRenderTargetD3D9> mCurrentRT;

  nsIntSize mSize;
};

}
}

#endif
