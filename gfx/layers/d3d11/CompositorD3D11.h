




#ifndef MOZILLA_GFX_COMPOSITORD3D11_H
#define MOZILLA_GFX_COMPOSITORD3D11_H

#include "mozilla/gfx/2D.h"
#include "mozilla/layers/Compositor.h"
#include "TextureD3D11.h"
#include <d3d11.h>

class nsWidget;

namespace mozilla {
namespace layers {

#define LOGD3D11(param)

struct VertexShaderConstants
{
  float layerTransform[4][4];
  float projection[4][4];
  float renderTargetOffset[4];
  gfx::Rect textureCoords;
  gfx::Rect layerQuad;
  gfx::Rect maskQuad;
};

struct PixelShaderConstants
{
  float layerColor[4];
  float layerOpacity[4];
};

struct DeviceAttachmentsD3D11;

class CompositorD3D11 : public Compositor
{
public:
  CompositorD3D11(nsIWidget* aWidget);
  ~CompositorD3D11();

  virtual bool Initialize() MOZ_OVERRIDE;
  virtual void Destroy() MOZ_OVERRIDE {}

  virtual TextureFactoryIdentifier
    GetTextureFactoryIdentifier() MOZ_OVERRIDE;

  virtual TemporaryRef<DataTextureSource>
    CreateDataTextureSource(TextureFlags aFlags = 0) MOZ_OVERRIDE { return nullptr; }

  virtual bool CanUseCanvasLayerForSize(const gfx::IntSize& aSize) MOZ_OVERRIDE;
  virtual int32_t GetMaxTextureSize() const MOZ_FINAL;

  virtual void SetTargetContext(gfx::DrawTarget* aTarget)  MOZ_OVERRIDE
  {
    mTarget = aTarget;
  }

  virtual void MakeCurrent(MakeCurrentFlags aFlags = 0)  MOZ_OVERRIDE {}

  virtual TemporaryRef<CompositingRenderTarget>
    CreateRenderTarget(const gfx::IntRect &aRect,
                       SurfaceInitMode aInit) MOZ_OVERRIDE;

  virtual TemporaryRef<CompositingRenderTarget>
    CreateRenderTargetFromSource(const gfx::IntRect& aRect,
                                 const CompositingRenderTarget* aSource) MOZ_OVERRIDE;

  virtual void SetRenderTarget(CompositingRenderTarget* aSurface) MOZ_OVERRIDE;
  virtual CompositingRenderTarget* GetCurrentRenderTarget() MOZ_OVERRIDE
  {
    return mCurrentRT;
  }

  virtual void SetDestinationSurfaceSize(const gfx::IntSize& aSize) MOZ_OVERRIDE {}

  



  virtual void SetScreenRenderOffset(const ScreenPoint& aOffset) MOZ_OVERRIDE
  {
    if (aOffset.x || aOffset.y) {
      NS_RUNTIMEABORT("SetScreenRenderOffset not supported by CompositorD3D11.");
    }
    
  }

  virtual void DrawQuad(const gfx::Rect &aRect,
                        const gfx::Rect &aClipRect,
                        const EffectChain &aEffectChain,
                        gfx::Float aOpacity,
                        const gfx::Matrix4x4 &aTransform,
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

  virtual bool SupportsPartialTextureUpdate() MOZ_OVERRIDE { return true; }

#ifdef MOZ_DUMP_PAINTING
  virtual const char* Name() const MOZ_OVERRIDE { return "Direct3D 11"; }
#endif

  virtual void NotifyLayersTransaction() MOZ_OVERRIDE { }

  virtual nsIWidget* GetWidget() const MOZ_OVERRIDE { return mWidget; }
  virtual const nsIntSize& GetWidgetSize() MOZ_OVERRIDE
  {
    NS_ASSERTION(false, "Getting the widget size on windows causes some kind of resizing of buffers. "
                        "You should not do that outside of BeginFrame, so the best we can do is return "
                        "the last size we got, that might not be up to date. So you probably shouldn't "
                        "use this method.");
    return mSize;
  }

  ID3D11Device* GetDevice() { return mDevice; }

private:
  
  void EnsureSize();
  void VerifyBufferSize();
  void UpdateRenderTarget();
  bool CreateShaders();
  void UpdateConstantBuffers();
  void SetSamplerForFilter(gfx::Filter aFilter);
  void SetPSForEffect(Effect *aEffect, MaskType aMaskType);
  void PaintToTarget();

  RefPtr<ID3D11DeviceContext> mContext;
  RefPtr<ID3D11Device> mDevice;
  RefPtr<IDXGISwapChain> mSwapChain;
  RefPtr<CompositingRenderTargetD3D11> mDefaultRT;
  RefPtr<CompositingRenderTargetD3D11> mCurrentRT;

  DeviceAttachmentsD3D11* mAttachments;

  RefPtr<gfx::DrawTarget> mTarget;

  nsIWidget* mWidget;

  nsIntSize mSize;

  HWND mHwnd;

  D3D_FEATURE_LEVEL mFeatureLevel;

  VertexShaderConstants mVSConstants;
  PixelShaderConstants mPSConstants;
  bool mDisableSequenceForNextFrame;
};

}
}

#endif
