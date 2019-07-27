




#ifndef gfx_layers_ipc_ImageBridgeParent_h_
#define gfx_layers_ipc_ImageBridgeParent_h_

#include <stddef.h>                     
#include <stdint.h>                     
#include "CompositableTransactionParent.h"
#include "CompositorParent.h"
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






class ImageBridgeParent MOZ_FINAL : public PImageBridgeParent,
                                    public CompositableParentManager
{
public:
  typedef InfallibleTArray<CompositableOperation> EditArray;
  typedef InfallibleTArray<EditReply> EditReplyArray;
  typedef InfallibleTArray<AsyncChildMessageData> AsyncChildMessageArray;

  ImageBridgeParent(MessageLoop* aLoop, Transport* aTransport, ProcessId aChildProcessId);
  ~ImageBridgeParent();

  virtual LayersBackend GetCompositorBackendType() const MOZ_OVERRIDE;

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  static PImageBridgeParent*
  Create(Transport* aTransport, ProcessId aChildProcessId);

  
  virtual void SendFenceHandle(AsyncTransactionTracker* aTracker,
                               PTextureParent* aTexture,
                               const FenceHandle& aFence) MOZ_OVERRIDE;

  virtual void SendAsyncMessage(const InfallibleTArray<AsyncParentMessageData>& aMessage) MOZ_OVERRIDE;

  virtual base::ProcessId GetChildProcessId() MOZ_OVERRIDE
  {
    return mChildProcessId;
  }

  
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

  
  virtual bool RecvWillStop() MOZ_OVERRIDE;
  
  virtual bool RecvStop() MOZ_OVERRIDE;

  virtual MessageLoop* GetMessageLoop() const MOZ_OVERRIDE;


  

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

  static void ReplyRemoveTexture(base::ProcessId aChildProcessId,
                                 const OpReplyRemoveTexture& aReply);

  void SendFenceHandleToTrackerIfPresent(uint64_t aDestHolderId,
                                         uint64_t aTransactionId,
                                         PTextureParent* aTexture);

  static void SendFenceHandleToTrackerIfPresent(base::ProcessId aChildProcessId,
                                                uint64_t aDestHolderId,
                                                uint64_t aTransactionId,
                                                PTextureParent* aTexture);

  static ImageBridgeParent* GetInstance(ProcessId aId);

  
  IToplevelProtocol*
  CloneToplevel(const InfallibleTArray<ProtocolFdMapping>& aFds,
                base::ProcessHandle aPeerProcess,
                mozilla::ipc::ProtocolCloneContext* aCtx) MOZ_OVERRIDE;

private:
  void DeferredDestroy();

  MessageLoop* mMessageLoop;
  Transport* mTransport;
  
  base::ProcessId mChildProcessId;
  
  
  nsRefPtr<ImageBridgeParent> mSelfRef;

  


  static std::map<base::ProcessId, ImageBridgeParent*> sImageBridges;

  static MessageLoop* sMainLoop;

  nsRefPtr<CompositorThreadHolder> mCompositorThreadHolder;
};

} 
} 

#endif 
