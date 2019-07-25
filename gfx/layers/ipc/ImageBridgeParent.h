




#include "mozilla/layers/PImageBridgeParent.h"

class MessageLoop;

namespace mozilla {
namespace layers {

class CompositorParent;





class ImageBridgeParent : public PImageBridgeParent
{
public:

  ImageBridgeParent(MessageLoop* aLoop);
  ~ImageBridgeParent();


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
  MessageLoop * mMessageLoop;
};

} 
} 

