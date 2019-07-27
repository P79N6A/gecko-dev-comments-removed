






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
class ThebesLayerComposite;
class ThebesBuffer;
class ThebesBufferData;
class TiledLayerComposer;
class Transaction;











































































class ShadowLayerForwarder : public CompositableForwarder
{
  friend class ContentClientIncremental;
  friend class ClientLayerManager;

public:
  virtual ~ShadowLayerForwarder();

  



  void Connect(CompositableClient* aCompositable);

  virtual PTextureChild* CreateTexture(const SurfaceDescriptor& aSharedData,
                                       TextureFlags aFlags) MOZ_OVERRIDE;

  virtual void CreatedIncrementalBuffer(CompositableClient* aCompositable,
                                        const TextureInfo& aTextureInfo,
                                        const nsIntRect& aBufferRect) MOZ_OVERRIDE;

  




  void Attach(CompositableClient* aCompositable,
              ShadowableLayer* aLayer);

  







  void AttachAsyncCompositable(uint64_t aCompositableID,
                               ShadowableLayer* aLayer);

  



  void BeginTransaction(const nsIntRect& aTargetBounds,
                        ScreenRotation aRotation,
                        mozilla::dom::ScreenOrientation aOrientation);

  





  




  void CreatedThebesLayer(ShadowableLayer* aThebes);
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
                                   const SurfaceDescriptorTiles& aTileLayerDescriptor) MOZ_OVERRIDE;

  




  void AttachAsyncCompositable(PLayerTransactionChild* aLayer, uint64_t aID);

  virtual void RemoveTextureFromCompositable(CompositableClient* aCompositable,
                                             TextureClient* aTexture) MOZ_OVERRIDE;

  virtual void RemoveTextureFromCompositableAsync(AsyncTransactionTracker* aAsyncTransactionTracker,
                                                  CompositableClient* aCompositable,
                                                  TextureClient* aTexture) MOZ_OVERRIDE;

  virtual void RemoveTexture(TextureClient* aTexture) MOZ_OVERRIDE;

  



  virtual void UpdateTextureRegion(CompositableClient* aCompositable,
                                   const ThebesBufferData& aThebesBufferData,
                                   const nsIntRegion& aUpdatedRegion) MOZ_OVERRIDE;

  virtual void UpdateTextureIncremental(CompositableClient* aCompositable,
                                        TextureIdentifier aTextureId,
                                        SurfaceDescriptor& aDescriptor,
                                        const nsIntRegion& aUpdatedRegion,
                                        const nsIntRect& aBufferRect,
                                        const nsIntPoint& aBufferRotation) MOZ_OVERRIDE;

  


  void UpdatePictureRect(CompositableClient* aCompositable,
                         const nsIntRect& aRect);

  


  virtual void UpdatedTexture(CompositableClient* aCompositable,
                              TextureClient* aTexture,
                              nsIntRegion* aRegion) MOZ_OVERRIDE;

  


  virtual void UseTexture(CompositableClient* aCompositable,
                          TextureClient* aClient) MOZ_OVERRIDE;
  virtual void UseComponentAlphaTextures(CompositableClient* aCompositable,
                                         TextureClient* aClientOnBlack,
                                         TextureClient* aClientOnWhite) MOZ_OVERRIDE;
#ifdef MOZ_WIDGET_GONK
  virtual void UseOverlaySource(CompositableClient* aCompositable,
                                const OverlaySource& aOverlay) MOZ_OVERRIDE;
#endif
  virtual void SendFenceHandle(AsyncTransactionTracker* aTracker,
                               PTextureChild* aTexture,
                               const FenceHandle& aFence) MOZ_OVERRIDE;

  




  bool EndTransaction(InfallibleTArray<EditReply>* aReplies,
                      const nsIntRegion& aRegionToClear,
                      uint64_t aId,
                      bool aScheduleComposite,
                      uint32_t aPaintSequenceNumber,
                      bool aIsRepeatTransaction,
                      const mozilla::TimeStamp& aTransactionStart,
                      bool* aSent);

  


  void SetShadowManager(PLayerTransactionChild* aShadowManager);

  void StopReceiveAsyncParentMessge();

  void ClearCachedResources();

  void Composite();

  void SendPendingAsyncMessge();

  


  bool HasShadowManager() const { return !!mShadowManager; }
  LayerTransactionChild* GetShadowManager() const { return mShadowManager.get(); }

  virtual void WindowOverlayChanged() { mWindowOverlayChanged = true; }

  
































  
  virtual bool AllocUnsafeShmem(size_t aSize,
                                mozilla::ipc::SharedMemory::SharedMemoryType aType,
                                mozilla::ipc::Shmem* aShmem) MOZ_OVERRIDE;
  virtual bool AllocShmem(size_t aSize,
                          mozilla::ipc::SharedMemory::SharedMemoryType aType,
                          mozilla::ipc::Shmem* aShmem) MOZ_OVERRIDE;
  virtual void DeallocShmem(mozilla::ipc::Shmem& aShmem) MOZ_OVERRIDE;

  virtual bool IPCOpen() const MOZ_OVERRIDE;
  virtual bool IsSameProcess() const MOZ_OVERRIDE;

  



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
  DiagnosticTypes mDiagnosticTypes;
  bool mIsFirstPaint;
  bool mWindowOverlayChanged;
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
