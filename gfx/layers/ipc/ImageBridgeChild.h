




#ifndef MOZILLA_GFX_IMAGEBRIDGECHILD_H
#define MOZILLA_GFX_IMAGEBRIDGECHILD_H

#include <stddef.h>                     
#include <stdint.h>                     
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/ipc/SharedMemory.h"   
#include "mozilla/layers/AsyncTransactionTracker.h" 
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/PImageBridgeChild.h"
#include "nsDebug.h"                    
#include "nsRegion.h"                   

class MessageLoop;
struct nsIntPoint;
struct nsIntRect;

namespace base {
class Thread;
}

namespace mozilla {
namespace ipc {
class Shmem;
}

namespace layers {

class ClientTiledLayerBuffer;
class AsyncTransactionTracker;
class ImageClient;
class ImageContainer;
class ImageBridgeParent;
class CompositableClient;
struct CompositableTransaction;
class Image;
class TextureClient;






bool InImageBridgeChildThread();




















































class ImageBridgeChild MOZ_FINAL : public PImageBridgeChild
                                 , public CompositableForwarder
                                 , public AsyncTransactionTrackersHolder
{
  friend class ImageContainer;
  typedef InfallibleTArray<AsyncParentMessageData> AsyncParentMessageArray;
public:

  





  static void StartUp();

  static PImageBridgeChild*
  StartUpInChildProcess(Transport* aTransport, ProcessId aOtherProcess);

  






  static void ShutDown();

  


  static bool StartUpOnThread(base::Thread* aThread);

  




  static bool IsCreated();

  




  static ImageBridgeChild* GetSingleton();


  


  void ConnectAsync(ImageBridgeParent* aParent);

  static void IdentifyCompositorTextureHost(const TextureFactoryIdentifier& aIdentifier);

  void BeginTransaction();
  void EndTransaction();

  




  base::Thread * GetThread() const;

  




  virtual MessageLoop * GetMessageLoop() const MOZ_OVERRIDE;

  PCompositableChild* AllocPCompositableChild(const TextureInfo& aInfo, uint64_t* aID) MOZ_OVERRIDE;
  bool DeallocPCompositableChild(PCompositableChild* aActor) MOZ_OVERRIDE;

  



  ~ImageBridgeChild();

  virtual PTextureChild*
  AllocPTextureChild(const SurfaceDescriptor& aSharedData, const TextureFlags& aFlags) MOZ_OVERRIDE;

  virtual bool
  DeallocPTextureChild(PTextureChild* actor) MOZ_OVERRIDE;

  virtual bool
  RecvParentAsyncMessages(const InfallibleTArray<AsyncParentMessageData>& aMessages) MOZ_OVERRIDE;

  TemporaryRef<ImageClient> CreateImageClient(CompositableType aType);
  TemporaryRef<ImageClient> CreateImageClientNow(CompositableType aType);

  static void DispatchReleaseImageClient(ImageClient* aClient);
  static void DispatchReleaseTextureClient(TextureClient* aClient);
  static void DispatchImageClientUpdate(ImageClient* aClient, ImageContainer* aContainer);

  


  static void FlushAllImages(ImageClient* aClient, ImageContainer* aContainer, bool aExceptFront);

  

  virtual void Connect(CompositableClient* aCompositable) MOZ_OVERRIDE;

  


  virtual void UpdatedTexture(CompositableClient* aCompositable,
                              TextureClient* aTexture,
                              nsIntRegion* aRegion) MOZ_OVERRIDE;

  virtual bool IsImageBridgeChild() const MOZ_OVERRIDE { return true; }

  


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

  virtual void RemoveTextureFromCompositable(CompositableClient* aCompositable,
                                             TextureClient* aTexture) MOZ_OVERRIDE;

  virtual void RemoveTextureFromCompositableAsync(AsyncTransactionTracker* aAsyncTransactionTracker,
                                                  CompositableClient* aCompositable,
                                                  TextureClient* aTexture) MOZ_OVERRIDE;

  virtual void RemoveTexture(TextureClient* aTexture) MOZ_OVERRIDE;

  virtual void UseTiledLayerBuffer(CompositableClient* aCompositable,
                                   const SurfaceDescriptorTiles& aTileLayerDescriptor) MOZ_OVERRIDE
  {
    NS_RUNTIMEABORT("should not be called");
  }

  virtual void UpdateTextureIncremental(CompositableClient* aCompositable,
                                        TextureIdentifier aTextureId,
                                        SurfaceDescriptor& aDescriptor,
                                        const nsIntRegion& aUpdatedRegion,
                                        const nsIntRect& aBufferRect,
                                        const nsIntPoint& aBufferRotation) MOZ_OVERRIDE
  {
    NS_RUNTIMEABORT("should not be called");
  }

  


  virtual void UpdatePictureRect(CompositableClient* aCompositable,
                                 const nsIntRect& aRect) MOZ_OVERRIDE;


  virtual void CreatedIncrementalBuffer(CompositableClient* aCompositable,
                                        const TextureInfo& aTextureInfo,
                                        const nsIntRect& aBufferRect) MOZ_OVERRIDE
  {
    NS_RUNTIMEABORT("should not be called");
  }
  virtual void UpdateTextureRegion(CompositableClient* aCompositable,
                                   const ThebesBufferData& aThebesBufferData,
                                   const nsIntRegion& aUpdatedRegion) MOZ_OVERRIDE {
    NS_RUNTIMEABORT("should not be called");
  }

  

  





  virtual bool AllocUnsafeShmem(size_t aSize,
                                mozilla::ipc::SharedMemory::SharedMemoryType aType,
                                mozilla::ipc::Shmem* aShmem) MOZ_OVERRIDE;
  





  virtual bool AllocShmem(size_t aSize,
                          mozilla::ipc::SharedMemory::SharedMemoryType aType,
                          mozilla::ipc::Shmem* aShmem) MOZ_OVERRIDE;
  





  virtual void DeallocShmem(mozilla::ipc::Shmem& aShmem);

  virtual PTextureChild* CreateTexture(const SurfaceDescriptor& aSharedData,
                                       TextureFlags aFlags) MOZ_OVERRIDE;

  virtual bool IsSameProcess() const MOZ_OVERRIDE;

  void SendPendingAsyncMessge();

  void MarkShutDown();
protected:
  ImageBridgeChild();
  bool DispatchAllocShmemInternal(size_t aSize,
                                  SharedMemory::SharedMemoryType aType,
                                  Shmem* aShmem,
                                  bool aUnsafe);

  CompositableTransaction* mTxn;
  bool mShuttingDown;
};

} 
} 

#endif
