






#ifndef mozilla_layers_ShadowLayers_h
#define mozilla_layers_ShadowLayers_h 1

#include "gfxASurface.h"
#include "GLDefs.h"

#include "ImageLayers.h"
#include "mozilla/layers/Compositor.h"
#include "mozilla/ipc/SharedMemory.h"
#include "mozilla/WidgetUtils.h"
#include "mozilla/layers/ISurfaceAllocator.h"
#include "mozilla/dom/ScreenOrientation.h"
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/layers/CompositorTypes.h"

class gfxSharedImageSurface;

namespace mozilla {

namespace gl {
class GLContext;
class TextureImage;
}

namespace layers {

class CompositableClient;
class Edit;
class EditReply;
class OptionalThebesBuffer;
class PLayerChild;
class PLayerTransactionChild;
class PLayerTransactionParent;
class ShadowableLayer;
class ThebesLayerComposite;
class ContainerLayerComposite;
class ImageLayerComposite;
class ColorLayerComposite;
class CanvasLayerComposite;
class RefLayerComposite;
class SurfaceDescriptor;
class ThebesBuffer;
class TiledLayerComposer;
class Transaction;
class SurfaceDescriptor;
class CanvasSurface;
class TextureClientShmem;
class ContentClientRemote;
class CompositableChild;
class ImageClient;
class CanvasClient;
class ContentClient;











































































class ShadowLayerForwarder : public CompositableForwarder
{
  friend class AutoOpenSurface;
  friend class TextureClientShmem;

public:
  typedef gfxASurface::gfxContentType gfxContentType;

  virtual ~ShadowLayerForwarder();

  



  void Connect(CompositableClient* aCompositable);

  virtual void CreatedSingleBuffer(CompositableClient* aCompositable,
                                   const SurfaceDescriptor& aDescriptor,
                                   const TextureInfo& aTextureInfo,
                                   const SurfaceDescriptor* aDescriptorOnWhite = nullptr) MOZ_OVERRIDE;
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
                   ShadowableLayer* aAfter=NULL);
  void RemoveChild(ShadowableLayer* aContainer,
                   ShadowableLayer* aChild);
  void RepositionChild(ShadowableLayer* aContainer,
                       ShadowableLayer* aChild,
                       ShadowableLayer* aAfter=NULL);

  






  void SetMask(ShadowableLayer* aLayer,
               ShadowableLayer* aMaskLayer);

  






  virtual void PaintedTiledLayerBuffer(CompositableClient* aCompositable,
                                       BasicTiledLayerBuffer* aTiledLayerBuffer) MOZ_OVERRIDE;

  




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

  


  void UpdatePictureRect(CompositableClient* aCompositable,
                         const nsIntRect& aRect);

  




  bool EndTransaction(InfallibleTArray<EditReply>* aReplies);

  


  void SetShadowManager(PLayerTransactionChild* aShadowManager)
  {
    mShadowManager = aShadowManager;
  }

  


  bool HasShadowManager() const { return !!mShadowManager; }
  PLayerTransactionChild* GetShadowManager() const { return mShadowManager; }

  
































  
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
                                                  gfxASurface::gfxContentType aContent,
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

  static already_AddRefed<gfxASurface>
  PlatformOpenDescriptor(OpenMode aMode, const SurfaceDescriptor& aDescriptor);

  



  static void
  CloseDescriptor(const SurfaceDescriptor& aDescriptor);

  static bool
  PlatformCloseDescriptor(const SurfaceDescriptor& aDescriptor);

  bool PlatformDestroySharedSurface(SurfaceDescriptor* aSurface);

  Transaction* mTxn;

  bool mIsFirstPaint;
  bool mDrawColoredBorders;
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
  ShadowableLayer() : mShadow(NULL) {}

  PLayerChild* mShadow;
};

} 
} 

#endif 
