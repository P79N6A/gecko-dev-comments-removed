




































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





class THEBES_API LayerManagerD3D10 : public LayerManager {
public:
  LayerManagerD3D10(nsIWidget *aWidget);
  virtual ~LayerManagerD3D10();

  







  bool Initialize();

  


  virtual void Destroy();

  virtual void SetRoot(Layer *aLayer);

  virtual void BeginTransaction();

  virtual void BeginTransactionWithTarget(gfxContext* aTarget);

  virtual bool EndEmptyTransaction();

  struct CallbackInfo {
    DrawThebesLayerCallback Callback;
    void *CallbackData;
  };

  virtual void EndTransaction(DrawThebesLayerCallback aCallback,
                              void* aCallbackData);

  const CallbackInfo &GetCallbackInfo() { return mCurrentCallbackInfo; }

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer();

  virtual already_AddRefed<ContainerLayer> CreateContainerLayer();

  virtual already_AddRefed<ImageLayer> CreateImageLayer();

  virtual already_AddRefed<ColorLayer> CreateColorLayer();

  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer();

  virtual already_AddRefed<ReadbackLayer> CreateReadbackLayer();

  virtual already_AddRefed<ImageContainer> CreateImageContainer();

  virtual already_AddRefed<gfxASurface>
    CreateOptimalSurface(const gfxIntSize &aSize,
                         gfxASurface::gfxImageFormat imageFormat);

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

  ReadbackManagerD3D10 *readbackManager();

  void SetViewport(const nsIntSize &aViewport);
  const nsIntSize &GetViewport() { return mViewport; }

  


  Nv3DVUtils *GetNv3DVUtils()  { return mNv3DVUtils; }

  static void LayerManagerD3D10::ReportFailure(const nsACString &aMsg, HRESULT aCode);

private:
  void SetupPipeline();
  void UpdateRenderTarget();
  void VerifyBufferSize();
  void EnsureReadbackManager();

  void Render();

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

  


  void PaintToTarget();
};




class LayerD3D10
{
public:
  LayerD3D10(LayerManagerD3D10 *aManager);

  virtual LayerD3D10 *GetFirstChildD3D10() { return nsnull; }

  void SetFirstChild(LayerD3D10 *aParent);

  virtual Layer* GetLayer() = 0;

  



  virtual void RenderLayer() = 0;
  virtual void Validate() {}

  ID3D10Device1 *device() const { return mD3DManager->device(); }
  ID3D10Effect *effect() const { return mD3DManager->effect(); }

  
  virtual void LayerManagerDestroyed() {}

  


  Nv3DVUtils *GetNv3DVUtils()  { return mD3DManager->GetNv3DVUtils(); }


  void SetEffectTransformAndOpacity()
  {
    Layer* layer = GetLayer();
    const gfx3DMatrix& transform = layer->GetEffectiveTransform();
    void* raw = &const_cast<gfx3DMatrix&>(transform)._11;
    effect()->GetVariableByName("mLayerTransform")->SetRawValue(raw, 0, 64);
    effect()->GetVariableByName("fLayerOpacity")->AsScalar()->SetFloat(layer->GetEffectiveOpacity());
  }

protected:
  LayerManagerD3D10 *mD3DManager;
};

} 
} 

#endif 
