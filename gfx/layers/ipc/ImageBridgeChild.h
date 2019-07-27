




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
#include "mozilla/gfx/Rect.h"

class MessageLoop;

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




















































class ImageBridgeChild final : public PImageBridgeChild
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

  




  virtual MessageLoop * GetMessageLoop() const override;

  PCompositableChild* AllocPCompositableChild(const TextureInfo& aInfo, uint64_t* aID) override;
  bool DeallocPCompositableChild(PCompositableChild* aActor) override;

  



  ~ImageBridgeChild();

  virtual PTextureChild*
  AllocPTextureChild(const SurfaceDescriptor& aSharedData, const TextureFlags& aFlags) override;

  virtual bool
  DeallocPTextureChild(PTextureChild* actor) override;

  virtual bool
  RecvParentAsyncMessages(InfallibleTArray<AsyncParentMessageData>&& aMessages) override;

  TemporaryRef<ImageClient> CreateImageClient(CompositableType aType);
  TemporaryRef<ImageClient> CreateImageClientNow(CompositableType aType);

  static void DispatchReleaseImageClient(ImageClient* aClient);
  static void DispatchReleaseTextureClient(TextureClient* aClient);
  static void DispatchImageClientUpdate(ImageClient* aClient, ImageContainer* aContainer);

  


  static void FlushAllImages(ImageClient* aClient, ImageContainer* aContainer, bool aExceptFront);

  

  virtual void Connect(CompositableClient* aCompositable) override;

  virtual bool IsImageBridgeChild() const override { return true; }

  


  virtual void UseTexture(CompositableClient* aCompositable,
                          TextureClient* aClient) override;
  virtual void UseComponentAlphaTextures(CompositableClient* aCompositable,
                                         TextureClient* aClientOnBlack,
                                         TextureClient* aClientOnWhite) override;
#ifdef MOZ_WIDGET_GONK
  virtual void UseOverlaySource(CompositableClient* aCompositable,
                                const OverlaySource& aOverlay) override;
#endif

  virtual void RemoveTextureFromCompositable(CompositableClient* aCompositable,
                                             TextureClient* aTexture) override;

  virtual void RemoveTextureFromCompositableAsync(AsyncTransactionTracker* aAsyncTransactionTracker,
                                                  CompositableClient* aCompositable,
                                                  TextureClient* aTexture) override;

  virtual void RemoveTexture(TextureClient* aTexture) override;

  virtual void UseTiledLayerBuffer(CompositableClient* aCompositable,
                                   const SurfaceDescriptorTiles& aTileLayerDescriptor) override
  {
    NS_RUNTIMEABORT("should not be called");
  }

  


  virtual void UpdatePictureRect(CompositableClient* aCompositable,
                                 const gfx::IntRect& aRect) override;


  virtual void UpdateTextureRegion(CompositableClient* aCompositable,
                                   const ThebesBufferData& aThebesBufferData,
                                   const nsIntRegion& aUpdatedRegion) override {
    NS_RUNTIMEABORT("should not be called");
  }

  

  





  virtual bool AllocUnsafeShmem(size_t aSize,
                                mozilla::ipc::SharedMemory::SharedMemoryType aType,
                                mozilla::ipc::Shmem* aShmem) override;
  





  virtual bool AllocShmem(size_t aSize,
                          mozilla::ipc::SharedMemory::SharedMemoryType aType,
                          mozilla::ipc::Shmem* aShmem) override;
  





  virtual void DeallocShmem(mozilla::ipc::Shmem& aShmem) override;

  virtual PTextureChild* CreateTexture(const SurfaceDescriptor& aSharedData,
                                       TextureFlags aFlags) override;

  virtual bool IsSameProcess() const override;

  virtual void SendPendingAsyncMessges() override;

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
