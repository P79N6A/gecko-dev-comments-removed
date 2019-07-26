




#include <stddef.h>                     
#include <stdint.h>                     
#include "CompositableTransactionParent.h"
#include "gfxPoint.h"                   
#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/ipc/ProtocolUtils.h"
#include "mozilla/ipc/SharedMemory.h"   
#include "mozilla/layers/PImageBridgeParent.h"
#include "nsAutoPtr.h"                  
#include "nsISupportsImpl.h"
#include "nsTArrayForwardDeclare.h"     

class MessageLoop;

namespace mozilla {
namespace ipc {
class Shmem;
}

namespace layers {






class ImageBridgeParent : public PImageBridgeParent,
                          public CompositableParentManager
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ImageBridgeParent)

public:
  typedef InfallibleTArray<CompositableOperation> EditArray;
  typedef InfallibleTArray<EditReply> EditReplyArray;

  ImageBridgeParent(MessageLoop* aLoop, Transport* aTransport);
  ~ImageBridgeParent();

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  static PImageBridgeParent*
  Create(Transport* aTransport, ProcessId aOtherProcess);

  virtual PGrallocBufferParent*
  AllocPGrallocBufferParent(const gfxIntSize&, const uint32_t&, const uint32_t&,
                            MaybeMagicGrallocBufferHandle*) MOZ_OVERRIDE;

  virtual bool
  DeallocPGrallocBufferParent(PGrallocBufferParent* actor) MOZ_OVERRIDE;

  
  virtual bool RecvUpdate(const EditArray& aEdits, EditReplyArray* aReply);
  virtual bool RecvUpdateNoSwap(const EditArray& aEdits);

  PCompositableParent* AllocPCompositableParent(const TextureInfo& aInfo,
                                                uint64_t*) MOZ_OVERRIDE;
  bool DeallocPCompositableParent(PCompositableParent* aActor) MOZ_OVERRIDE;

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

