






#ifndef mozilla_layers_ShadowLayers_h
#define mozilla_layers_ShadowLayers_h 1

#include <stddef.h>                     
#include <stdint.h>                     
#include "gfxTypes.h"
#include "mozilla/Attributes.h"         
#include "mozilla/WidgetUtils.h"        
#include "mozilla/dom/ScreenOrientation.h"  
#include "mozilla/ipc/SharedMemory.h"   
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/layers/CompositorTypes.h"  
#include "nsCOMPtr.h"                   
#include "nsRegion.h"                   
#include "nsTArrayForwardDeclare.h"     
#include "nsIWidget.h"

struct nsIntPoint;
struct nsIntRect;

namespace mozilla {
namespace layers {

class ClientTiledLayerBuffer;
class CanvasClient;
class CanvasLayerComposite;
class CanvasSurface;
class ColorLayerComposite;
class CompositableChild;
class ContainerLayerComposite;
class ContentClient;
class ContentClientRemote;
class EditReply;
class ImageClient;
class ImageLayerComposite;
class Layer;
class OptionalThebesBuffer;
class PLayerChild;
class PLayerTransactionChild;
class PLayerTransactionParent;
class LayerTransactionChild;
class RefLayerComposite;
class ShadowableLayer;
class ShmemTextureClient;
class SurfaceDescriptor;
class TextureClient;
class PaintedLayerComposite;
class ThebesBuffer;
class ThebesBufferData;
class TiledLayerComposer;
class Transaction;











































































class ShadowLayerForwarder final : public CompositableForwarder
{
  friend class ClientLayerManager;

public:
  virtual ~ShadowLayerForwarder();

  



  void Connect(CompositableClient* aCompositable) override;

  virtual PTextureChild* CreateTexture(const SurfaceDescriptor& aSharedData,
                                       TextureFlags aFlags) override;

  




  void Attach(CompositableClient* aCompositable,
              ShadowableLayer* aLayer);

  







  void AttachAsyncCompositable(uint64_t aCompositableID,
                               ShadowableLayer* aLayer);

  



  void BeginTransaction(const nsIntRect& aTargetBounds,
                        ScreenRotation aRotation,
                        mozilla::dom::ScreenOrientation aOrientation);

  





  




  void CreatedPaintedLayer(ShadowableLayer* aThebes);
  void CreatedContainerLayer(ShadowableLayer* aContainer);
  void CreatedImageLayer(ShadowableLayer* aImage);
  void CreatedColorLayer(ShadowableLayer* aColor);
  void CreatedCanvasLayer(ShadowableLayer* aCanvas);
  void CreatedRefLayer(ShadowableLayer* aRef);

  




  void Mutated(ShadowableLayer* aMutant);

  void SetRoot(ShadowableLayer* aRoot);
  




  void InsertAfter(ShadowableLayer* aContainer,
                   ShadowableLayer* aChild,
                   ShadowableLayer* aAfter = nullptr);
  void RemoveChild(ShadowableLayer* aContainer,
                   ShadowableLayer* aChild);
  void RepositionChild(ShadowableLayer* aContainer,
                       ShadowableLayer* aChild,
                       ShadowableLayer* aAfter = nullptr);

  






  void SetMask(ShadowableLayer* aLayer,
               ShadowableLayer* aMaskLayer);

  


  virtual void UseTiledLayerBuffer(CompositableClient* aCompositable,
                                   const SurfaceDescriptorTiles& aTileLayerDescriptor) override;

  




  void AttachAsyncCompositable(PLayerTransactionChild* aLayer, uint64_t aID);

  virtual void RemoveTextureFromCompositable(CompositableClient* aCompositable,
                                             TextureClient* aTexture) override;

  virtual void RemoveTextureFromCompositableAsync(AsyncTransactionTracker* aAsyncTransactionTracker,
                                                  CompositableClient* aCompositable,
                                                  TextureClient* aTexture) override;

  virtual void RemoveTexture(TextureClient* aTexture) override;

  



  virtual void UpdateTextureRegion(CompositableClient* aCompositable,
                                   const ThebesBufferData& aThebesBufferData,
                                   const nsIntRegion& aUpdatedRegion) override;

  


  void UpdatePictureRect(CompositableClient* aCompositable,
                         const nsIntRect& aRect) override;

  


  virtual void UseTexture(CompositableClient* aCompositable,
                          TextureClient* aClient) override;
  virtual void UseComponentAlphaTextures(CompositableClient* aCompositable,
                                         TextureClient* aClientOnBlack,
                                         TextureClient* aClientOnWhite) override;
#ifdef MOZ_WIDGET_GONK
  virtual void UseOverlaySource(CompositableClient* aCompositable,
                                const OverlaySource& aOverlay) override;
#endif

  




  bool EndTransaction(InfallibleTArray<EditReply>* aReplies,
                      const nsIntRegion& aRegionToClear,
                      uint64_t aId,
                      bool aScheduleComposite,
                      uint32_t aPaintSequenceNumber,
                      bool aIsRepeatTransaction,
                      const mozilla::TimeStamp& aTransactionStart,
                      bool* aSent);

  


  void SetShadowManager(PLayerTransactionChild* aShadowManager);

  




  void StorePluginWidgetConfigurations(const nsTArray<nsIWidget::Configuration>&
                                       aConfigurations);

  void StopReceiveAsyncParentMessge();

  void ClearCachedResources();

  void Composite();

  virtual void SendPendingAsyncMessges() override;

  


  bool HasShadowManager() const { return !!mShadowManager; }
  LayerTransactionChild* GetShadowManager() const { return mShadowManager.get(); }

  virtual void WindowOverlayChanged() { mWindowOverlayChanged = true; }

  
































  
  virtual bool AllocUnsafeShmem(size_t aSize,
                                mozilla::ipc::SharedMemory::SharedMemoryType aType,
                                mozilla::ipc::Shmem* aShmem) override;
  virtual bool AllocShmem(size_t aSize,
                          mozilla::ipc::SharedMemory::SharedMemoryType aType,
                          mozilla::ipc::Shmem* aShmem) override;
  virtual void DeallocShmem(mozilla::ipc::Shmem& aShmem) override;

  virtual bool IPCOpen() const override;
  virtual bool IsSameProcess() const override;

  



  PLayerChild* ConstructShadowFor(ShadowableLayer* aLayer);

  


  void SetIsFirstPaint() { mIsFirstPaint = true; }

  static void PlatformSyncBeforeUpdate();

protected:
  ShadowLayerForwarder();

#ifdef DEBUG
  void CheckSurfaceDescriptor(const SurfaceDescriptor* aDescriptor) const;
#else
  void CheckSurfaceDescriptor(const SurfaceDescriptor* aDescriptor) const {}
#endif

  bool InWorkerThread();

  RefPtr<LayerTransactionChild> mShadowManager;

private:

  Transaction* mTxn;
  std::vector<AsyncChildMessageData> mPendingAsyncMessages;
  DiagnosticTypes mDiagnosticTypes;
  bool mIsFirstPaint;
  bool mWindowOverlayChanged;
  InfallibleTArray<PluginWindowData> mPluginWindowData;
};

class CompositableClient;








class ShadowableLayer
{
public:
  virtual ~ShadowableLayer() {}

  virtual Layer* AsLayer() = 0;

  


  bool HasShadow() { return !!mShadow; }

  



  PLayerChild* GetShadow() { return mShadow; }

  virtual CompositableClient* GetCompositableClient() { return nullptr; }
protected:
  ShadowableLayer() : mShadow(nullptr) {}

  PLayerChild* mShadow;
};

} 
} 

#endif 
