




#ifndef GFX_LAYERMANAGERD3D10_H
#define GFX_LAYERMANAGERD3D10_H

#include "Layers.h"

#include <windows.h>
#include <d3d10_1.h>

#include "gfxContext.h"
#include "nsIWidget.h"

#include "ReadbackManagerD3D10.h"

namespace mozilla {
namespace layers {

class DummyRoot;
class Nv3DVUtils;








struct ShaderConstantRectD3D10
{
  float mX, mY, mWidth, mHeight;
  ShaderConstantRectD3D10(float aX, float aY, float aWidth, float aHeight)
    : mX(aX), mY(aY), mWidth(aWidth), mHeight(aHeight)
  { }

  
  operator float* () { return &mX; }
};

extern cairo_user_data_key_t gKeyD3D10Texture;








class LayerManagerD3D10 : public LayerManager {
public:
  LayerManagerD3D10(nsIWidget *aWidget);
  virtual ~LayerManagerD3D10();

  







  bool Initialize(bool force = false, HRESULT* aHresultPtr = nullptr);

  


  virtual void Destroy();

  virtual void SetRoot(Layer *aLayer);

  virtual void BeginTransaction();

  virtual void BeginTransactionWithTarget(gfxContext* aTarget);

  virtual bool EndEmptyTransaction(EndTransactionFlags aFlags = END_DEFAULT);

  struct CallbackInfo {
    DrawThebesLayerCallback Callback;
    void *CallbackData;
  };

  virtual void EndTransaction(DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags aFlags = END_DEFAULT);

  const CallbackInfo &GetCallbackInfo() { return mCurrentCallbackInfo; }

  
  enum {
    MAX_TEXTURE_SIZE = 8192
  };
  virtual bool CanUseCanvasLayerForSize(const gfx::IntSize &aSize)
  {
    return aSize <= gfx::IntSize(MAX_TEXTURE_SIZE, MAX_TEXTURE_SIZE);
  }

  virtual int32_t GetMaxTextureSize() const
  {
    return MAX_TEXTURE_SIZE;
  }

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer();
  virtual already_AddRefed<ContainerLayer> CreateContainerLayer();
  virtual already_AddRefed<ImageLayer> CreateImageLayer();
  virtual already_AddRefed<ColorLayer> CreateColorLayer();
  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer();
  virtual already_AddRefed<ReadbackLayer> CreateReadbackLayer();

  virtual already_AddRefed<gfxASurface>
    CreateOptimalSurface(const gfx::IntSize &aSize,
                         gfxImageFormat imageFormat);

  virtual already_AddRefed<gfxASurface>
    CreateOptimalMaskSurface(const gfx::IntSize &aSize);

  virtual TemporaryRef<mozilla::gfx::DrawTarget>
    CreateDrawTarget(const gfx::IntSize &aSize,
                     mozilla::gfx::SurfaceFormat aFormat);

  virtual LayersBackend GetBackendType() { return LayersBackend::LAYERS_D3D10; }
  virtual void GetBackendName(nsAString& name) { name.AssignLiteral("Direct3D 10"); }

  virtual const char* Name() const { return "D3D10"; }

  

  ID3D10Device1 *device() const { return mDevice; }

  ID3D10Effect *effect() const { return mEffect; }
  IDXGISwapChain *SwapChain() const
  {
    return mSwapChain;
  }
  ReadbackManagerD3D10 *readbackManager();

  void SetupInputAssembler();
  void SetViewport(const nsIntSize &aViewport);
  const nsIntSize &GetViewport() { return mViewport; }

  


  Nv3DVUtils *GetNv3DVUtils()  { return mNv3DVUtils; }

  static void ReportFailure(const nsACString &aMsg, HRESULT aCode);

private:
  void SetupPipeline();
  void UpdateRenderTarget();
  void VerifyBufferSize();
  void EnsureReadbackManager();

  void Render(EndTransactionFlags aFlags);

  nsRefPtr<ID3D10Device1> mDevice;

  nsRefPtr<ID3D10Effect> mEffect;
  nsRefPtr<ID3D10InputLayout> mInputLayout;
  nsRefPtr<ID3D10Buffer> mVertexBuffer;
  nsRefPtr<ReadbackManagerD3D10> mReadbackManager;

  nsRefPtr<ID3D10RenderTargetView> mRTView;

  nsRefPtr<IDXGISwapChain> mSwapChain;

  nsIWidget *mWidget;

  bool mDisableSequenceForNextFrame;

  CallbackInfo mCurrentCallbackInfo;

  nsIntSize mViewport;

   
  nsAutoPtr<Nv3DVUtils> mNv3DVUtils; 

  


  nsRefPtr<gfxContext> mTarget;

  


  void PaintToTarget();
};




class LayerD3D10
{
public:
  LayerD3D10(LayerManagerD3D10 *aManager);

  virtual LayerD3D10 *GetFirstChildD3D10() { return nullptr; }

  void SetFirstChild(LayerD3D10 *aParent);

  virtual Layer* GetLayer() = 0;

  



  virtual void RenderLayer() = 0;
  virtual void Validate() {}

  ID3D10Device1 *device() const { return mD3DManager->device(); }
  ID3D10Effect *effect() const { return mD3DManager->effect(); }

  
  virtual void LayerManagerDestroyed() {}

  


  Nv3DVUtils *GetNv3DVUtils()  { return mD3DManager->GetNv3DVUtils(); }

  









  virtual already_AddRefed<ID3D10ShaderResourceView> GetAsTexture(gfx::IntSize* aSize)
  {
    return nullptr;
  }

  void SetEffectTransformAndOpacity()
  {
    Layer* layer = GetLayer();
    const gfx::Matrix4x4& transform = layer->GetEffectiveTransform();
    void* raw = &const_cast<gfx::Matrix4x4&>(transform)._11;
    effect()->GetVariableByName("mLayerTransform")->SetRawValue(raw, 0, 64);
    effect()->GetVariableByName("fLayerOpacity")->AsScalar()->SetFloat(layer->GetEffectiveOpacity());
  }

protected:
  





  uint8_t LoadMaskTexture();

  






  ID3D10EffectTechnique* SelectShader(uint8_t aFlags);
  const static uint8_t SHADER_NO_MASK = 0;
  const static uint8_t SHADER_MASK = 0x1;
  const static uint8_t SHADER_MASK_3D = 0x2;
  
  const static uint8_t SHADER_RGB = 0;
  const static uint8_t SHADER_RGBA = 0x4;
  const static uint8_t SHADER_NON_PREMUL = 0;
  const static uint8_t SHADER_PREMUL = 0x8;
  const static uint8_t SHADER_LINEAR = 0;
  const static uint8_t SHADER_POINT = 0x10;
  
  const static uint8_t SHADER_YCBCR = 0x20;
  const static uint8_t SHADER_COMPONENT_ALPHA = 0x24;
  const static uint8_t SHADER_SOLID = 0x28;

  LayerManagerD3D10 *mD3DManager;
};

} 
} 

#endif 
