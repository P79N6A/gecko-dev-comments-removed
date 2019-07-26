




#ifndef MOZILLA_GFX_IMAGEBRIDGECHILD_H
#define MOZILLA_GFX_IMAGEBRIDGECHILD_H

#include "mozilla/layers/PImageBridgeChild.h"
#include "nsAutoPtr.h"
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/layers/LayersTypes.h"

class gfxSharedImageSurface;

namespace base {
class Thread;
}

namespace mozilla {
namespace layers {

class ImageClient;
class ImageContainer;
class ImageBridgeParent;
class SurfaceDescriptor;
class CompositableClient;
class CompositableTransaction;
class ShadowableLayer;
class Image;







bool InImageBridgeChildThread();




















































class ImageBridgeChild : public PImageBridgeChild
                       , public CompositableForwarder
{
  friend class ImageContainer;
public:

  





  static void StartUp();

  static PImageBridgeChild*
  StartUpInChildProcess(Transport* aTransport, ProcessId aOtherProcess);

  






  static void ShutDown();

  


  static bool StartUpOnThread(base::Thread* aThread);

  






  static void DestroyBridge();

  




  static bool IsCreated();

  




  static ImageBridgeChild* GetSingleton();


  


  void ConnectAsync(ImageBridgeParent* aParent);

  void BeginTransaction();
  void EndTransaction();

  




  base::Thread * GetThread() const;

  




  MessageLoop * GetMessageLoop() const;

  PCompositableChild* AllocPCompositable(const TextureInfo& aInfo, uint64_t* aID) MOZ_OVERRIDE;
  bool DeallocPCompositable(PCompositableChild* aActor) MOZ_OVERRIDE;

  



  ~ImageBridgeChild();

  virtual PGrallocBufferChild*
  AllocPGrallocBuffer(const gfxIntSize&, const uint32_t&, const uint32_t&,
                      MaybeMagicGrallocBufferHandle*) MOZ_OVERRIDE;

  virtual bool
  DeallocPGrallocBuffer(PGrallocBufferChild* actor) MOZ_OVERRIDE;

  


  bool
  AllocSurfaceDescriptorGralloc(const gfxIntSize& aSize,
                                const uint32_t& aFormat,
                                const uint32_t& aUsage,
                                SurfaceDescriptor* aBuffer);

  






  bool
  AllocSurfaceDescriptorGrallocNow(const gfxIntSize& aSize,
                                   const uint32_t& aContent,
                                   const uint32_t& aUsage,
                                   SurfaceDescriptor* aBuffer);

  


  bool
  DeallocSurfaceDescriptorGralloc(const SurfaceDescriptor& aBuffer);

  






  bool
  DeallocSurfaceDescriptorGrallocNow(const SurfaceDescriptor& aBuffer);

  TemporaryRef<ImageClient> CreateImageClient(CompositableType aType);
  TemporaryRef<ImageClient> CreateImageClientNow(CompositableType aType);

  static void DispatchReleaseImageClient(ImageClient* aClient);
  static void DispatchImageClientUpdate(ImageClient* aClient, ImageContainer* aContainer);


  

  virtual void Connect(CompositableClient* aCompositable) MOZ_OVERRIDE;

  virtual void PaintedTiledLayerBuffer(CompositableClient* aCompositable,
                                       BasicTiledLayerBuffer* aTiledLayerBuffer) MOZ_OVERRIDE
  {
    NS_RUNTIMEABORT("should not be called");
  }

  



  virtual void UpdateTexture(CompositableClient* aCompositable,
                             TextureIdentifier aTextureId,
                             SurfaceDescriptor* aDescriptor) MOZ_OVERRIDE;

  virtual void UpdateTextureNoSwap(CompositableClient* aCompositable,
                                   TextureIdentifier aTextureId,
                                   SurfaceDescriptor* aDescriptor) MOZ_OVERRIDE;

  


  virtual void UpdatePictureRect(CompositableClient* aCompositable,
                                 const nsIntRect& aRect) MOZ_OVERRIDE;


  
  
  virtual void CreatedSingleBuffer(CompositableClient* aCompositable,
                                   const SurfaceDescriptor& aDescriptor,
                                   const TextureInfo& aTextureInfo,
                                   const SurfaceDescriptor* aDescriptorOnWhite = nullptr) MOZ_OVERRIDE {
    NS_RUNTIMEABORT("should not be called");
  }
  virtual void CreatedDoubleBuffer(CompositableClient* aCompositable,
                                   const SurfaceDescriptor& aFrontDescriptor,
                                   const SurfaceDescriptor& aBackDescriptor,
                                   const TextureInfo& aTextureInfo,
                                   const SurfaceDescriptor* aFrontDescriptorOnWhite = nullptr,
                                   const SurfaceDescriptor* aBackDescriptorOnWhite = nullptr) MOZ_OVERRIDE {
    NS_RUNTIMEABORT("should not be called");
  }
  virtual void DestroyThebesBuffer(CompositableClient* aCompositable) MOZ_OVERRIDE {
    NS_RUNTIMEABORT("should not be called");
  }
  virtual void UpdateTextureRegion(CompositableClient* aCompositable,
                                   const ThebesBufferData& aThebesBufferData,
                                   const nsIntRegion& aUpdatedRegion) MOZ_OVERRIDE {
    NS_RUNTIMEABORT("should not be called");
  }
  virtual void DestroyedThebesBuffer(const SurfaceDescriptor& aBackBufferToDestroy) MOZ_OVERRIDE
  {
    NS_RUNTIMEABORT("should not be called");
  }


  

  





  virtual bool AllocUnsafeShmem(size_t aSize,
                                ipc::SharedMemory::SharedMemoryType aType,
                                ipc::Shmem* aShmem) MOZ_OVERRIDE;
  





  virtual bool AllocShmem(size_t aSize,
                          ipc::SharedMemory::SharedMemoryType aType,
                          ipc::Shmem* aShmem) MOZ_OVERRIDE;
  





  virtual void DeallocShmem(ipc::Shmem& aShmem);

protected:
  ImageBridgeChild();
  bool DispatchAllocShmemInternal(size_t aSize,
                                  SharedMemory::SharedMemoryType aType,
                                  Shmem* aShmem,
                                  bool aUnsafe);

  CompositableTransaction* mTxn;

  virtual PGrallocBufferChild* AllocGrallocBuffer(const gfxIntSize& aSize,
                                                  gfxASurface::gfxContentType aContent,
                                                  MaybeMagicGrallocBufferHandle* aHandle) MOZ_OVERRIDE;
};

} 
} 

#endif
