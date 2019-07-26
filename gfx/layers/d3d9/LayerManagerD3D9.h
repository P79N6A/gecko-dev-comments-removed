




#ifndef GFX_LAYERMANAGERD3D9_H
#define GFX_LAYERMANAGERD3D9_H

#include "Layers.h"

#include <windows.h>
#include <d3d9.h>

#include "gfxContext.h"
#include "nsIWidget.h"

#include "DeviceManagerD3D9.h"

namespace mozilla {
namespace layers {

class LayerD3D9;
class ThebesLayerD3D9;





class LayerManagerD3D9 : public LayerManager {
public:
  LayerManagerD3D9(nsIWidget *aWidget);
  virtual ~LayerManagerD3D9();

  







  bool Initialize(bool force = false);

  








  void SetClippingRegion(const nsIntRegion& aClippingRegion);

  


  virtual void Destroy();

  virtual void BeginTransaction();

  virtual void BeginTransactionWithTarget(gfxContext* aTarget);

  void EndConstruction();

  virtual bool EndEmptyTransaction(EndTransactionFlags aFlags = END_DEFAULT);

  struct CallbackInfo {
    DrawThebesLayerCallback Callback;
    void *CallbackData;
  };

  virtual void EndTransaction(DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags aFlags = END_DEFAULT);

  const CallbackInfo &GetCallbackInfo() { return mCurrentCallbackInfo; }

  void SetRoot(Layer* aLayer);

  virtual bool CanUseCanvasLayerForSize(const gfxIntSize &aSize)
  {
    if (!mDeviceManager)
      return false;
    int32_t maxSize = mDeviceManager->GetMaxTextureSize();
    return aSize <= gfxIntSize(maxSize, maxSize);
  }

  virtual int32_t GetMaxTextureSize() const
  {
    return mDeviceManager->GetMaxTextureSize();
  }

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer();

  virtual already_AddRefed<ContainerLayer> CreateContainerLayer();

  virtual already_AddRefed<ImageLayer> CreateImageLayer();

  virtual already_AddRefed<ColorLayer> CreateColorLayer();

  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer();

  virtual already_AddRefed<ReadbackLayer> CreateReadbackLayer();

  virtual LayersBackend GetBackendType() { return LAYERS_D3D9; }
  virtual void GetBackendName(nsAString& name) { name.AssignLiteral("Direct3D 9"); }
  bool DeviceWasRemoved() { return deviceManager()->DeviceWasRemoved(); }

  


  void SetClippingEnabled(bool aEnabled);

  void SetShaderMode(DeviceManagerD3D9::ShaderMode aMode,
                     Layer* aMask, bool aIs2D = true)
    { mDeviceManager->SetShaderMode(aMode, aMask, aIs2D); }

  IDirect3DDevice9 *device() const { return mDeviceManager->device(); }
  DeviceManagerD3D9 *deviceManager() const { return mDeviceManager; }

  

 
  Nv3DVUtils *GetNv3DVUtils() { return mDeviceManager ? mDeviceManager->GetNv3DVUtils() : nullptr; } 

  static void OnDeviceManagerDestroy(DeviceManagerD3D9 *aDeviceManager) {
    if(aDeviceManager == mDefaultDeviceManager)
      mDefaultDeviceManager = nullptr;
  }

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() const { return "D3D9"; }
#endif 

  void ReportFailure(const nsACString &aMsg, HRESULT aCode);

  bool CompositingDisabled() { return mCompositingDisabled; }
  void SetCompositingDisabled(bool aCompositingDisabled) { mCompositingDisabled = aCompositingDisabled; }

private:
  
  static DeviceManagerD3D9 *mDefaultDeviceManager;

  
  nsRefPtr<DeviceManagerD3D9> mDeviceManager;

  
  nsRefPtr<SwapChainD3D9> mSwapChain;

  
  nsIWidget *mWidget;

  


  nsRefPtr<gfxContext> mTarget;

  
  CallbackInfo mCurrentCallbackInfo;

  


  nsIntRegion mClippingRegion;

  



  uint32_t mDeviceResetCount;

  



  bool mCompositingDisabled;

  


  void Render();

  


  void SetupPipeline();

  


  void PaintToTarget();

};




class LayerD3D9
{
public:
  LayerD3D9(LayerManagerD3D9 *aManager);

  virtual LayerD3D9 *GetFirstChildD3D9() { return nullptr; }

  void SetFirstChild(LayerD3D9 *aParent);

  virtual Layer* GetLayer() = 0;

  virtual void RenderLayer() = 0;

  



  virtual void CleanResources() {}

  IDirect3DDevice9 *device() const { return mD3DManager->device(); }

  
  virtual void LayerManagerDestroyed() {}

  void ReportFailure(const nsACString &aMsg, HRESULT aCode) {
    return mD3DManager->ReportFailure(aMsg, aCode);
  }

  void SetShaderTransformAndOpacity()
  {
    Layer* layer = GetLayer();
    const gfx3DMatrix& transform = layer->GetEffectiveTransform();
    device()->SetVertexShaderConstantF(CBmLayerTransform, &transform._11, 4);

    float opacity[4];
    



    opacity[0] = layer->GetEffectiveOpacity();
    device()->SetPixelShaderConstantF(CBfLayerOpacity, opacity, 1);
  }

  










  virtual already_AddRefed<IDirect3DTexture9> GetAsTexture(gfxIntSize* aSize)
  {
    return nullptr;
  }
 
protected:
  LayerManagerD3D9 *mD3DManager;
};




class LockTextureRectD3D9 
{
public:
  LockTextureRectD3D9(IDirect3DTexture9* aTexture) 
    : mTexture(aTexture)
  {
    mLockResult = mTexture->LockRect(0, &mR, nullptr, 0);
  }

  ~LockTextureRectD3D9()
  {
    mTexture->UnlockRect(0);
  }

  bool HasLock() {
    return SUCCEEDED(mLockResult);
  }

  D3DLOCKED_RECT GetLockRect() 
  {
    return mR;
  }
private:
  LockTextureRectD3D9 (const LockTextureRectD3D9&);
  LockTextureRectD3D9& operator= (const LockTextureRectD3D9&);

  IDirect3DTexture9* mTexture;
  D3DLOCKED_RECT mR;
  HRESULT mLockResult;
};

} 
} 

#endif 
