




#ifndef GFX_LAYERMANAGERD3D10_H
#define GFX_LAYERMANAGERD3D10_H

#include "mozilla/layers/PLayers.h"
#include "mozilla/layers/ShadowLayers.h"
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









class THEBES_API LayerManagerD3D10 : public ShadowLayerManager,
                                     public ShadowLayerForwarder {
public:
  LayerManagerD3D10(nsIWidget *aWidget);
  virtual ~LayerManagerD3D10();

  







  bool Initialize(bool force = false);

  


  virtual void Destroy();

  virtual ShadowLayerForwarder* AsShadowForwarder()
  { return this; }

  virtual ShadowLayerManager* AsShadowManager()
  { return this; }

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
  virtual bool CanUseCanvasLayerForSize(const gfxIntSize &aSize)
  {
    return aSize <= gfxIntSize(MAX_TEXTURE_SIZE, MAX_TEXTURE_SIZE);
  }

  virtual PRInt32 GetMaxTextureSize() const
  {
    return MAX_TEXTURE_SIZE;
  }

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer();
  virtual already_AddRefed<ShadowThebesLayer> CreateShadowThebesLayer();

  virtual already_AddRefed<ContainerLayer> CreateContainerLayer();
  virtual already_AddRefed<ShadowContainerLayer> CreateShadowContainerLayer();

  virtual already_AddRefed<ImageLayer> CreateImageLayer();
  virtual already_AddRefed<ShadowImageLayer> CreateShadowImageLayer()
  { return nullptr; }

  virtual already_AddRefed<ColorLayer> CreateColorLayer();
  virtual already_AddRefed<ShadowColorLayer> CreateShadowColorLayer()
  { return nullptr; }

  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer();
  virtual already_AddRefed<ShadowCanvasLayer> CreateShadowCanvasLayer()
  { return nullptr; }

  virtual already_AddRefed<ReadbackLayer> CreateReadbackLayer();

  virtual already_AddRefed<gfxASurface>
    CreateOptimalSurface(const gfxIntSize &aSize,
                         gfxASurface::gfxImageFormat imageFormat);

  virtual already_AddRefed<gfxASurface>
    CreateOptimalMaskSurface(const gfxIntSize &aSize);

  virtual TemporaryRef<mozilla::gfx::DrawTarget>
    CreateDrawTarget(const mozilla::gfx::IntSize &aSize,
                     mozilla::gfx::SurfaceFormat aFormat);

  virtual LayersBackend GetBackendType() { return LAYERS_D3D10; }
  virtual void GetBackendName(nsAString& name) { name.AssignLiteral("Direct3D 10"); }

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() const { return "D3D10"; }
#endif 

  

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

  CallbackInfo mCurrentCallbackInfo;

  nsIntSize mViewport;

   
  nsAutoPtr<Nv3DVUtils> mNv3DVUtils; 

  


  nsRefPtr<gfxContext> mTarget;

  













  nsRefPtr<ID3D10Texture2D> mBackBuffer;
  nsRefPtr<ID3D10Texture2D> mRemoteFrontBuffer;
  



  nsRefPtr<DummyRoot> mRootForShadowTree;

  


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

  









  virtual already_AddRefed<ID3D10ShaderResourceView> GetAsTexture(gfxIntSize* aSize)
  {
    return nullptr;
  }

  void SetEffectTransformAndOpacity()
  {
    Layer* layer = GetLayer();
    const gfx3DMatrix& transform = layer->GetEffectiveTransform();
    void* raw = &const_cast<gfx3DMatrix&>(transform)._11;
    effect()->GetVariableByName("mLayerTransform")->SetRawValue(raw, 0, 64);
    effect()->GetVariableByName("fLayerOpacity")->AsScalar()->SetFloat(layer->GetEffectiveOpacity());
  }

protected:
  





  PRUint8 LoadMaskTexture();

  






  ID3D10EffectTechnique* SelectShader(PRUint8 aFlags);
  const static PRUint8 SHADER_NO_MASK = 0;
  const static PRUint8 SHADER_MASK = 0x1;
  const static PRUint8 SHADER_MASK_3D = 0x2;
  
  const static PRUint8 SHADER_RGB = 0;
  const static PRUint8 SHADER_RGBA = 0x4;
  const static PRUint8 SHADER_NON_PREMUL = 0;
  const static PRUint8 SHADER_PREMUL = 0x8;
  const static PRUint8 SHADER_LINEAR = 0;
  const static PRUint8 SHADER_POINT = 0x10;
  
  const static PRUint8 SHADER_YCBCR = 0x20;
  const static PRUint8 SHADER_COMPONENT_ALPHA = 0x24;
  const static PRUint8 SHADER_SOLID = 0x28;

  LayerManagerD3D10 *mD3DManager;
};










class WindowLayer : public ThebesLayer, public ShadowableLayer {
public:
  WindowLayer(LayerManagerD3D10* aManager);
  virtual ~WindowLayer();

  void InvalidateRegion(const nsIntRegion&) {}
  Layer* AsLayer() { return this; }

  void SetShadow(PLayerChild* aChild) { mShadow = aChild; }
};






class DummyRoot : public ContainerLayer, public ShadowableLayer {
public:
  DummyRoot(LayerManagerD3D10* aManager);
  virtual ~DummyRoot();

  void ComputeEffectiveTransforms(const gfx3DMatrix&) {}
  void InsertAfter(Layer*, Layer*);
  void RemoveChild(Layer*);
  Layer* AsLayer() { return this; }

  void SetShadow(PLayerChild* aChild) { mShadow = aChild; }
};

} 
} 

#endif 
