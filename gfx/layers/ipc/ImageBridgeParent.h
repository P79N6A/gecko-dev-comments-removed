




#ifndef gfx_layers_ipc_ImageBridgeParent_h_
#define gfx_layers_ipc_ImageBridgeParent_h_

#include <stddef.h>                     
#include <stdint.h>                     
#include "CompositableTransactionParent.h"
#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/ipc/ProtocolUtils.h"
#include "mozilla/ipc/SharedMemory.h"   
#include "mozilla/layers/CompositorParent.h"
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






class ImageBridgeParent final : public PImageBridgeParent,
                                public CompositableParentManager
{
public:
  typedef InfallibleTArray<CompositableOperation> EditArray;
  typedef InfallibleTArray<EditReply> EditReplyArray;
  typedef InfallibleTArray<AsyncChildMessageData> AsyncChildMessageArray;

  ImageBridgeParent(MessageLoop* aLoop, Transport* aTransport, ProcessId aChildProcessId);
  ~ImageBridgeParent();

  virtual LayersBackend GetCompositorBackendType() const override;

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  static PImageBridgeParent*
  Create(Transport* aTransport, ProcessId aChildProcessId);

  
  virtual void SendFenceHandleIfPresent(PTextureParent* aTexture,
                                        CompositableHost* aCompositableHost) override;

  virtual void SendAsyncMessage(const InfallibleTArray<AsyncParentMessageData>& aMessage) override;

  virtual base::ProcessId GetChildProcessId() override
  {
    return OtherPid();
  }

  
  virtual bool RecvUpdate(EditArray&& aEdits, EditReplyArray* aReply) override;
  virtual bool RecvUpdateNoSwap(EditArray&& aEdits) override;

  virtual bool IsAsync() const override { return true; }

  PCompositableParent* AllocPCompositableParent(const TextureInfo& aInfo,
                                                uint64_t*) override;
  bool DeallocPCompositableParent(PCompositableParent* aActor) override;

  virtual PTextureParent* AllocPTextureParent(const SurfaceDescriptor& aSharedData,
                                              const TextureFlags& aFlags) override;
  virtual bool DeallocPTextureParent(PTextureParent* actor) override;

  virtual bool
  RecvChildAsyncMessages(InfallibleTArray<AsyncChildMessageData>&& aMessages) override;

  
  virtual bool RecvWillStop() override;
  
  virtual bool RecvStop() override;

  virtual MessageLoop* GetMessageLoop() const override;


  

  bool AllocShmem(size_t aSize,
                  ipc::SharedMemory::SharedMemoryType aType,
                  ipc::Shmem* aShmem) override
  {
    return PImageBridgeParent::AllocShmem(aSize, aType, aShmem);
  }

  bool AllocUnsafeShmem(size_t aSize,
                        ipc::SharedMemory::SharedMemoryType aType,
                        ipc::Shmem* aShmem) override
  {
    return PImageBridgeParent::AllocUnsafeShmem(aSize, aType, aShmem);
  }

  void DeallocShmem(ipc::Shmem& aShmem) override
  {
    PImageBridgeParent::DeallocShmem(aShmem);
  }

  virtual bool IsSameProcess() const override;

  virtual void ReplyRemoveTexture(const OpReplyRemoveTexture& aReply) override;

  static void ReplyRemoveTexture(base::ProcessId aChildProcessId,
                                 const OpReplyRemoveTexture& aReply);

  void AppendDeliverFenceMessage(uint64_t aDestHolderId,
                                 uint64_t aTransactionId,
                                 PTextureParent* aTexture,
                                 CompositableHost* aCompositableHost);

  static void AppendDeliverFenceMessage(base::ProcessId aChildProcessId,
                                        uint64_t aDestHolderId,
                                        uint64_t aTransactionId,
                                        PTextureParent* aTexture,
                                        CompositableHost* aCompositableHost);

  using CompositableParentManager::SendPendingAsyncMessages;
  static void SendPendingAsyncMessages(base::ProcessId aChildProcessId);

  static ImageBridgeParent* GetInstance(ProcessId aId);

  
  IToplevelProtocol*
  CloneToplevel(const InfallibleTArray<ProtocolFdMapping>& aFds,
                base::ProcessHandle aPeerProcess,
                mozilla::ipc::ProtocolCloneContext* aCtx) override;

private:
  void DeferredDestroy();

  MessageLoop* mMessageLoop;
  Transport* mTransport;
  
  
  nsRefPtr<ImageBridgeParent> mSelfRef;

  


  static std::map<base::ProcessId, ImageBridgeParent*> sImageBridges;

  static MessageLoop* sMainLoop;

  nsRefPtr<CompositorThreadHolder> mCompositorThreadHolder;
};

} 
} 

#endif 
