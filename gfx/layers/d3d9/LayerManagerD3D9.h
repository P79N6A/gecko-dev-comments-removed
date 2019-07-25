




































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

extern cairo_user_data_key_t gKeyD3D9Texture;

class LayerD3D9;
class ThebesLayerD3D9;








struct ShaderConstantRect
{
  float mX, mY, mWidth, mHeight;
  ShaderConstantRect(float aX, float aY, float aWidth, float aHeight)
    : mX(aX), mY(aY), mWidth(aWidth), mHeight(aHeight)
  { }

  
  operator float* () { return &mX; }
};





class THEBES_API LayerManagerD3D9 : public LayerManager {
public:
  LayerManagerD3D9(nsIWidget *aWidget);
  virtual ~LayerManagerD3D9();

  







  PRBool Initialize();

  








  void SetClippingRegion(const nsIntRegion& aClippingRegion);

  


  virtual void Destroy();

  void BeginTransaction();

  void BeginTransactionWithTarget(gfxContext* aTarget);

  void EndConstruction();

  struct CallbackInfo {
    DrawThebesLayerCallback Callback;
    void *CallbackData;
  };

  void EndTransaction(DrawThebesLayerCallback aCallback,
                      void* aCallbackData);

  const CallbackInfo &GetCallbackInfo() { return mCurrentCallbackInfo; }

  void SetRoot(Layer* aLayer);

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer();

  virtual already_AddRefed<ContainerLayer> CreateContainerLayer();

  virtual already_AddRefed<ImageLayer> CreateImageLayer();

  virtual already_AddRefed<ColorLayer> CreateColorLayer();

  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer();

  virtual already_AddRefed<ImageContainer> CreateImageContainer();

  virtual already_AddRefed<gfxASurface>
    CreateOptimalSurface(const gfxIntSize &aSize,
                         gfxASurface::gfxImageFormat imageFormat);

  virtual LayersBackend GetBackendType() { return LAYERS_D3D9; }
  virtual void GetBackendName(nsAString& name) { name.AssignLiteral("Direct3D 9"); }
  bool DeviceWasRemoved() { return deviceManager()->DeviceWasRemoved(); }

  


  void SetClippingEnabled(PRBool aEnabled);

  void SetShaderMode(DeviceManagerD3D9::ShaderMode aMode)
    { mDeviceManager->SetShaderMode(aMode); }

  IDirect3DDevice9 *device() const { return mDeviceManager->device(); }
  DeviceManagerD3D9 *deviceManager() const { return mDeviceManager; }

  

 
  Nv3DVUtils *GetNv3DVUtils()  { return mDeviceManager ? mDeviceManager->GetNv3DVUtils() : NULL; } 

  

 
  PRBool Is3DEnabled() { return mIs3DEnabled; } 

  static void OnDeviceManagerDestroy(DeviceManagerD3D9 *aDeviceManager) {
    if(aDeviceManager == mDefaultDeviceManager)
      mDefaultDeviceManager = nsnull;
  }

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() const { return "D3D9"; }
#endif 

private:
  
  static DeviceManagerD3D9 *mDefaultDeviceManager;

  
  nsRefPtr<DeviceManagerD3D9> mDeviceManager;

  
  nsRefPtr<SwapChainD3D9> mSwapChain;

  
  nsIWidget *mWidget;

  


  nsRefPtr<gfxContext> mTarget;

  
  CallbackInfo mCurrentCallbackInfo;

   
  PRBool mIs3DEnabled; 

  


  nsIntRegion mClippingRegion;

  


  void Render();

  


  void SetupPipeline();

  


  void PaintToTarget();

};




class LayerD3D9
{
public:
  LayerD3D9(LayerManagerD3D9 *aManager);

  virtual LayerD3D9 *GetFirstChildD3D9() { return nsnull; }

  void SetFirstChild(LayerD3D9 *aParent);

  virtual Layer* GetLayer() = 0;

  virtual void RenderLayer() = 0;

  


  virtual void CleanResources() {}

  IDirect3DDevice9 *device() const { return mD3DManager->device(); }

  
  virtual void LayerManagerDestroyed() {}
protected:
  LayerManagerD3D9 *mD3DManager;
};

} 
} 

#endif 
