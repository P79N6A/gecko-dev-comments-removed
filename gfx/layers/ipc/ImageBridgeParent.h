




#ifndef gfx_layers_ipc_ImageBridgeParent_h_
#define gfx_layers_ipc_ImageBridgeParent_h_

#include <stddef.h>                     
#include <stdint.h>                     
#include "CompositableTransactionParent.h"
#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/ipc/ProtocolUtils.h"
#include "mozilla/ipc/SharedMemory.h"   
#include "mozilla/layers/PImageBridgeParent.h"
#include "nsAutoPtr.h"                  
#include "nsISupportsImpl.h"
#include "nsTArrayForwardDeclare.h"     

class MessageLoop;

namespace base {
class Thread;
}

namespace mozilla {
namespace ipc {
class Shmem;
}

namespace layers {






class ImageBridgeParent : public PImageBridgeParent,
                          public CompositableParentManager
{
public:
  typedef InfallibleTArray<CompositableOperation> EditArray;
  typedef InfallibleTArray<EditReply> EditReplyArray;
  typedef InfallibleTArray<AsyncChildMessageData> AsyncChildMessageArray;

  ImageBridgeParent(MessageLoop* aLoop, Transport* aTransport);
  ~ImageBridgeParent();

  virtual LayersBackend GetCompositorBackendType() const MOZ_OVERRIDE;

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  static PImageBridgeParent*
  Create(Transport* aTransport, ProcessId aOtherProcess);

  
  virtual void SendFenceHandle(AsyncTransactionTracker* aTracker,
                               PTextureParent* aTexture,
                               const FenceHandle& aFence) MOZ_OVERRIDE;

  
  virtual bool RecvUpdate(const EditArray& aEdits, EditReplyArray* aReply) MOZ_OVERRIDE;
  virtual bool RecvUpdateNoSwap(const EditArray& aEdits) MOZ_OVERRIDE;

  virtual bool IsAsync() const MOZ_OVERRIDE { return true; }

  PCompositableParent* AllocPCompositableParent(const TextureInfo& aInfo,
                                                uint64_t*) MOZ_OVERRIDE;
  bool DeallocPCompositableParent(PCompositableParent* aActor) MOZ_OVERRIDE;

  virtual PTextureParent* AllocPTextureParent(const SurfaceDescriptor& aSharedData,
                                              const TextureFlags& aFlags) MOZ_OVERRIDE;
  virtual bool DeallocPTextureParent(PTextureParent* actor) MOZ_OVERRIDE;

  virtual bool
  RecvChildAsyncMessages(const InfallibleTArray<AsyncChildMessageData>& aMessages) MOZ_OVERRIDE;

  bool RecvStop() MOZ_OVERRIDE;

  MessageLoop * GetMessageLoop();


  

  bool AllocShmem(size_t aSize,
                  ipc::SharedMemory::SharedMemoryType aType,
                  ipc::Shmem* aShmem) MOZ_OVERRIDE
  {
    return AllocShmem(aSize, aType, aShmem);
  }

  bool AllocUnsafeShmem(size_t aSize,
                        ipc::SharedMemory::SharedMemoryType aType,
                        ipc::Shmem* aShmem) MOZ_OVERRIDE
  {
    return AllocUnsafeShmem(aSize, aType, aShmem);
  }

  void DeallocShmem(ipc::Shmem& aShmem) MOZ_OVERRIDE
  {
    PImageBridgeParent::DeallocShmem(aShmem);
  }

  virtual bool IsSameProcess() const MOZ_OVERRIDE;

  virtual void ReplyRemoveTexture(const OpReplyRemoveTexture& aReply) MOZ_OVERRIDE;

  
  IToplevelProtocol*
  CloneToplevel(const InfallibleTArray<ProtocolFdMapping>& aFds,
                base::ProcessHandle aPeerProcess,
                mozilla::ipc::ProtocolCloneContext* aCtx) MOZ_OVERRIDE;

private:
  void DeferredDestroy();

  MessageLoop* mMessageLoop;
  Transport* mTransport;
  
  
  nsRefPtr<ImageBridgeParent> mSelfRef;
};

} 
} 

#endif 
