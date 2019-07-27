




#ifndef MOZILLA_GFX_COMPOSITORD3D9_H
#define MOZILLA_GFX_COMPOSITORD3D9_H

#include "mozilla/gfx/2D.h"
#include "gfx2DGlue.h"
#include "mozilla/layers/Compositor.h"
#include "mozilla/layers/TextureD3D9.h"
#include "DeviceManagerD3D9.h"

class nsWidget;

namespace mozilla {
namespace layers {

class CompositorD3D9 : public Compositor
{
public:
  CompositorD3D9(PCompositorParent* aParent, nsIWidget *aWidget);
  ~CompositorD3D9();

  virtual bool Initialize() MOZ_OVERRIDE;
  virtual void Destroy() MOZ_OVERRIDE {}

  virtual TextureFactoryIdentifier
    GetTextureFactoryIdentifier() MOZ_OVERRIDE;

  virtual bool CanUseCanvasLayerForSize(const gfx::IntSize &aSize) MOZ_OVERRIDE;
  virtual int32_t GetMaxTextureSize() const MOZ_FINAL;

  virtual void MakeCurrent(MakeCurrentFlags aFlags = 0) MOZ_OVERRIDE {}

  virtual TemporaryRef<CompositingRenderTarget>
    CreateRenderTarget(const gfx::IntRect &aRect,
                       SurfaceInitMode aInit) MOZ_OVERRIDE;

  virtual TemporaryRef<CompositingRenderTarget>
    CreateRenderTargetFromSource(const gfx::IntRect &aRect,
                                 const CompositingRenderTarget *aSource,
                                 const gfx::IntPoint &aSourcePoint) MOZ_OVERRIDE;

  virtual void SetRenderTarget(CompositingRenderTarget *aSurface);
  virtual CompositingRenderTarget* GetCurrentRenderTarget() const MOZ_OVERRIDE
  {
    return mCurrentRT;
  }

  virtual void SetDestinationSurfaceSize(const gfx::IntSize& aSize) MOZ_OVERRIDE {}

  virtual void ClearRect(const gfx::Rect& aRect) MOZ_OVERRIDE;

  virtual void DrawQuad(const gfx::Rect &aRect,
                        const gfx::Rect &aClipRect,
                        const EffectChain &aEffectChain,
                        gfx::Float aOpacity,
                        const gfx::Matrix4x4 &aTransform) MOZ_OVERRIDE;

  virtual void BeginFrame(const nsIntRegion& aInvalidRegion,
                          const gfx::Rect *aClipRectIn,
                          const gfx::Rect& aRenderBounds,
                          gfx::Rect *aClipRectOut = nullptr,
                          gfx::Rect *aRenderBoundsOut = nullptr) MOZ_OVERRIDE;

  virtual void EndFrame() MOZ_OVERRIDE;

  virtual void EndFrameForExternalComposition(const gfx::Matrix& aTransform) MOZ_OVERRIDE {}

  virtual void AbortFrame() MOZ_OVERRIDE {}

  virtual void PrepareViewport(const gfx::IntSize& aSize) MOZ_OVERRIDE;

  virtual bool SupportsPartialTextureUpdate() MOZ_OVERRIDE{ return true; }

#ifdef MOZ_DUMP_PAINTING
  virtual const char* Name() const MOZ_OVERRIDE { return "Direct3D9"; }
#endif

  virtual LayersBackend GetBackendType() const MOZ_OVERRIDE {
    return LayersBackend::LAYERS_D3D9;
  }

  virtual nsIWidget* GetWidget() const MOZ_OVERRIDE { return mWidget; }

  IDirect3DDevice9* device() const
  {
    
    
    
    return mDeviceManager && mDeviceResetCount == mDeviceManager->GetDeviceResetCount()
           ? mDeviceManager->device()
           : nullptr;
  }

  








  virtual bool Ready() MOZ_OVERRIDE;

  



  virtual void SetScreenRenderOffset(const ScreenPoint& aOffset) MOZ_OVERRIDE
  {
    if (aOffset.x || aOffset.y) {
      NS_RUNTIMEABORT("SetScreenRenderOffset not supported by CompositorD3D9.");
    }
    
  }

  virtual TemporaryRef<DataTextureSource>
    CreateDataTextureSource(TextureFlags aFlags = TextureFlags::NO_FLAGS) MOZ_OVERRIDE;
private:
  
  void EnsureSize();
  void SetSamplerForFilter(gfx::Filter aFilter);
  void PaintToTarget();
  void SetMask(const EffectChain &aEffectChain, uint32_t aMaskTexture);
  








  bool EnsureSwapChain();

  








  void CheckResetCount();

  void ReportFailure(const nsACString &aMsg, HRESULT aCode);

  virtual gfx::IntSize GetWidgetSize() const MOZ_OVERRIDE
  {
    return gfx::ToIntSize(mSize);
  }

  
  nsRefPtr<DeviceManagerD3D9> mDeviceManager;

  
  nsRefPtr<SwapChainD3D9> mSwapChain;

  
  nsIWidget *mWidget;

  RefPtr<CompositingRenderTargetD3D9> mDefaultRT;
  RefPtr<CompositingRenderTargetD3D9> mCurrentRT;

  nsIntSize mSize;

  uint32_t mDeviceResetCount;
};

}
}

#endif
