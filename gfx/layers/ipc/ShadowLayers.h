






#ifndef mozilla_layers_ShadowLayers_h
#define mozilla_layers_ShadowLayers_h 1

#include <stddef.h>                     
#include <stdint.h>                     
#include "gfxTypes.h"
#include "gfxPoint.h"                   
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
class gfxASurface;

namespace mozilla {
namespace layers {

class BasicTiledLayerBuffer;
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
class RefLayerComposite;
class ShadowableLayer;
class Shmem;
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
  friend class AutoOpenSurface;
  friend class DeprecatedTextureClientShmem;
  friend class ContentClientIncremental;

public:
  virtual ~ShadowLayerForwarder();

  



  void Connect(CompositableClient* aCompositable);

  virtual void CreatedSingleBuffer(CompositableClient* aCompositable,
                                   const SurfaceDescriptor& aDescriptor,
                                   const TextureInfo& aTextureInfo,
                                   const SurfaceDescriptor* aDescriptorOnWhite = nullptr) MOZ_OVERRIDE;
  virtual void CreatedIncrementalBuffer(CompositableClient* aCompositable,
                                        const TextureInfo& aTextureInfo,
                                        const nsIntRect& aBufferRect) MOZ_OVERRIDE;
  virtual void CreatedDoubleBuffer(CompositableClient* aCompositable,
                                   const SurfaceDescriptor& aFrontDescriptor,
                                   const SurfaceDescriptor& aBackDescriptor,
                                   const TextureInfo& aTextureInfo,
                                   const SurfaceDescriptor* aFrontDescriptorOnWhite = nullptr,
                                   const SurfaceDescriptor* aBackDescriptorOnWhite = nullptr) MOZ_OVERRIDE;
  virtual void DestroyThebesBuffer(CompositableClient* aCompositable) MOZ_OVERRIDE;

  




  void Attach(CompositableClient* aCompositable,
              ShadowableLayer* aLayer);

  







  void AttachAsyncCompositable(uint64_t aCompositableID,
                               ShadowableLayer* aLayer);

  



  void BeginTransaction(const nsIntRect& aTargetBounds,
                        ScreenRotation aRotation,
                        const nsIntRect& aClientBounds,
                        mozilla::dom::ScreenOrientation aOrientation);

  





  




  void CreatedThebesLayer(ShadowableLayer* aThebes);
  void CreatedContainerLayer(ShadowableLayer* aContainer);
  void CreatedImageLayer(ShadowableLayer* aImage);
  void CreatedColorLayer(ShadowableLayer* aColor);
  void CreatedCanvasLayer(ShadowableLayer* aCanvas);
  void CreatedRefLayer(ShadowableLayer* aRef);

  







  virtual void DestroyedThebesBuffer(const SurfaceDescriptor& aBackBufferToDestroy) MOZ_OVERRIDE;

  




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

  






  virtual void PaintedTiledLayerBuffer(CompositableClient* aCompositable,
                                       const SurfaceDescriptorTiles& aTileLayerDescriptor) MOZ_OVERRIDE;

  




  void AttachAsyncCompositable(PLayerTransactionChild* aLayer, uint64_t aID);

  



  virtual void UpdateTexture(CompositableClient* aCompositable,
                             TextureIdentifier aTextureId,
                             SurfaceDescriptor* aDescriptor) MOZ_OVERRIDE;

  


  virtual void UpdateTextureNoSwap(CompositableClient* aCompositable,
                                   TextureIdentifier aTextureId,
                                   SurfaceDescriptor* aDescriptor) MOZ_OVERRIDE;

  



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

  


  virtual bool AddTexture(CompositableClient* aCompositable,
                          TextureClient* aClient) MOZ_OVERRIDE;

  


  virtual void RemoveTexture(CompositableClient* aCompositable,
                             uint64_t aTextureID,
                             TextureFlags aFlags) MOZ_OVERRIDE;

  


  virtual void UpdatedTexture(CompositableClient* aCompositable,
                              TextureClient* aTexture,
                              nsIntRegion* aRegion) MOZ_OVERRIDE;

  


  virtual void UseTexture(CompositableClient* aCompositable,
                          TextureClient* aClient) MOZ_OVERRIDE;

  




  bool EndTransaction(InfallibleTArray<EditReply>* aReplies);

  


  void SetShadowManager(PLayerTransactionChild* aShadowManager)
  {
    mShadowManager = aShadowManager;
  }

  


  bool HasShadowManager() const { return !!mShadowManager; }
  PLayerTransactionChild* GetShadowManager() const { return mShadowManager; }

  virtual void WindowOverlayChanged() { mWindowOverlayChanged = true; }

  
































  
  virtual bool AllocUnsafeShmem(size_t aSize,
                                ipc::SharedMemory::SharedMemoryType aType,
                                ipc::Shmem* aShmem) MOZ_OVERRIDE;
  virtual bool AllocShmem(size_t aSize,
                          ipc::SharedMemory::SharedMemoryType aType,
                          ipc::Shmem* aShmem) MOZ_OVERRIDE;
  virtual void DeallocShmem(ipc::Shmem& aShmem) MOZ_OVERRIDE;

  



  PLayerChild* ConstructShadowFor(ShadowableLayer* aLayer);

  


  void SetIsFirstPaint() { mIsFirstPaint = true; }

  static void PlatformSyncBeforeUpdate();

  static already_AddRefed<gfxASurface>
  OpenDescriptor(OpenMode aMode, const SurfaceDescriptor& aSurface);

protected:
  ShadowLayerForwarder();

  PLayerTransactionChild* mShadowManager;

#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
  
  virtual PGrallocBufferChild* AllocGrallocBuffer(const gfxIntSize& aSize,
                                                  uint32_t aFormat,
                                                  uint32_t aUsage,
                                                  MaybeMagicGrallocBufferHandle* aHandle) MOZ_OVERRIDE;
#endif

private:
  



  static gfxContentType
  GetDescriptorSurfaceContentType(const SurfaceDescriptor& aDescriptor,
                                  OpenMode aMode,
                                  gfxASurface** aSurface);
  




  static bool
  PlatformGetDescriptorSurfaceContentType(const SurfaceDescriptor& aDescriptor,
                                          OpenMode aMode,
                                          gfxContentType* aContent,
                                          gfxASurface** aSurface);
  
  static gfxIntSize
  GetDescriptorSurfaceSize(const SurfaceDescriptor& aDescriptor,
                           OpenMode aMode,
                           gfxASurface** aSurface);
  static bool
  PlatformGetDescriptorSurfaceSize(const SurfaceDescriptor& aDescriptor,
                                   OpenMode aMode,
                                   gfxIntSize* aSize,
                                   gfxASurface** aSurface);
  
  
  
  static gfxImageFormat
  GetDescriptorSurfaceImageFormat(const SurfaceDescriptor& aDescriptor,
                                  OpenMode aMode,
                                  gfxASurface** aSurface);
  static bool
  PlatformGetDescriptorSurfaceImageFormat(const SurfaceDescriptor& aDescriptor,
                                          OpenMode aMode,
                                          gfxImageFormat* aContent,
                                          gfxASurface** aSurface);

  static already_AddRefed<gfxASurface>
  PlatformOpenDescriptor(OpenMode aMode, const SurfaceDescriptor& aDescriptor);

  



  static void
  CloseDescriptor(const SurfaceDescriptor& aDescriptor);

  static bool
  PlatformCloseDescriptor(const SurfaceDescriptor& aDescriptor);

  bool PlatformDestroySharedSurface(SurfaceDescriptor* aSurface);

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
