




#include "mozilla/layers/PImageBridgeParent.h"

class MessageLoop;

namespace mozilla {
namespace layers {

class CompositorParent;





class ImageBridgeParent : public PImageBridgeParent
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ImageBridgeParent)

public:
  ImageBridgeParent(MessageLoop* aLoop, Transport* aTransport);
  ~ImageBridgeParent();

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  static PImageBridgeParent*
  Create(Transport* aTransport, ProcessId aOtherProcess);

  virtual PGrallocBufferParent*
  AllocPGrallocBuffer(const gfxIntSize&, const uint32_t&, const uint32_t&,
                      MaybeMagicGrallocBufferHandle*) MOZ_OVERRIDE;

  virtual bool
  DeallocPGrallocBuffer(PGrallocBufferParent* actor) MOZ_OVERRIDE;

  
  PImageContainerParent* AllocPImageContainer(uint64_t* aID) MOZ_OVERRIDE;
  
  bool DeallocPImageContainer(PImageContainerParent* toDealloc) MOZ_OVERRIDE;
  
  bool RecvStop() MOZ_OVERRIDE;

  MessageLoop * GetMessageLoop();

private:
  void DeferredDestroy();

  MessageLoop* mMessageLoop;
  Transport* mTransport;
  
  
  nsRefPtr<ImageBridgeParent> mSelfRef;
};

} 
} 

