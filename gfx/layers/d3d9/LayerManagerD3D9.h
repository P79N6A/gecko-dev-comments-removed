




































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





class THEBES_API LayerManagerD3D9 : public LayerManager {
public:
  LayerManagerD3D9(nsIWidget *aWidget);
  virtual ~LayerManagerD3D9();

  







  PRBool Initialize();

  








  void SetClippingRegion(const nsIntRegion& aClippingRegion);

  


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

  virtual LayersBackend GetBackendType() { return LAYERS_D3D9; }

  


  void SetClippingEnabled(PRBool aEnabled);

  void SetShaderMode(DeviceManagerD3D9::ShaderMode aMode)
    { mDeviceManager->SetShaderMode(aMode); }

  IDirect3DDevice9 *device() const { return mDeviceManager->device(); }
  DeviceManagerD3D9 *deviceManager() const { return mDeviceManager; }

  static void OnDeviceManagerDestroy(DeviceManagerD3D9 *aDeviceManager) {
    if(aDeviceManager == mDeviceManager)
      mDeviceManager = nsnull;
  }

private:
  
  static DeviceManagerD3D9 *mDeviceManager;

  
  nsRefPtr<SwapChainD3D9> mSwapChain;

  
  nsIWidget *mWidget;

  


  nsRefPtr<gfxContext> mTarget;

  
  LayerD3D9 *mRootLayer;

  
  CallbackInfo mCurrentCallbackInfo;

  


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

  IDirect3DDevice9 *device() const { return mD3DManager->device(); }
protected:
  LayerManagerD3D9 *mD3DManager;
};

} 
} 

#endif 
