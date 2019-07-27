




#ifndef MOZILLA_GFX_COMPOSITORD3D11_H
#define MOZILLA_GFX_COMPOSITORD3D11_H

#include "mozilla/gfx/2D.h"
#include "gfx2DGlue.h"
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
    CreateDataTextureSource(TextureFlags aFlags = TextureFlags::NO_FLAGS) MOZ_OVERRIDE;

  virtual bool CanUseCanvasLayerForSize(const gfx::IntSize& aSize) MOZ_OVERRIDE;
  virtual int32_t GetMaxTextureSize() const MOZ_FINAL;

  virtual void MakeCurrent(MakeCurrentFlags aFlags = 0)  MOZ_OVERRIDE {}

  virtual TemporaryRef<CompositingRenderTarget>
    CreateRenderTarget(const gfx::IntRect &aRect,
                       SurfaceInitMode aInit) MOZ_OVERRIDE;

  virtual TemporaryRef<CompositingRenderTarget>
    CreateRenderTargetFromSource(const gfx::IntRect& aRect,
                                 const CompositingRenderTarget* aSource,
                                 const gfx::IntPoint& aSourcePoint) MOZ_OVERRIDE;

  virtual void SetRenderTarget(CompositingRenderTarget* aSurface) MOZ_OVERRIDE;
  virtual CompositingRenderTarget* GetCurrentRenderTarget() const MOZ_OVERRIDE
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

  virtual bool SupportsPartialTextureUpdate() MOZ_OVERRIDE { return true; }

#ifdef MOZ_DUMP_PAINTING
  virtual const char* Name() const MOZ_OVERRIDE { return "Direct3D 11"; }
#endif

  virtual LayersBackend GetBackendType() const MOZ_OVERRIDE {
    return LayersBackend::LAYERS_D3D11;
  }

  virtual nsIWidget* GetWidget() const MOZ_OVERRIDE { return mWidget; }

  ID3D11Device* GetDevice() { return mDevice; }

  ID3D11DeviceContext* GetDC() { return mContext; }

private:
  
  void EnsureSize();
  void VerifyBufferSize();
  void UpdateRenderTarget();
  bool CreateShaders();
  void UpdateConstantBuffers();
  void SetSamplerForFilter(gfx::Filter aFilter);
  void SetPSForEffect(Effect *aEffect, MaskType aMaskType, gfx::SurfaceFormat aFormat);
  void PaintToTarget();

  virtual gfx::IntSize GetWidgetSize() const MOZ_OVERRIDE { return gfx::ToIntSize(mSize); }

  RefPtr<ID3D11DeviceContext> mContext;
  RefPtr<ID3D11Device> mDevice;
  RefPtr<IDXGISwapChain> mSwapChain;
  RefPtr<CompositingRenderTargetD3D11> mDefaultRT;
  RefPtr<CompositingRenderTargetD3D11> mCurrentRT;

  DeviceAttachmentsD3D11* mAttachments;

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
