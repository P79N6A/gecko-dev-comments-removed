




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

  
  PImageContainerParent* AllocPImageContainer(PRUint64* aID) MOZ_OVERRIDE;
  
  bool DeallocPImageContainer(PImageContainerParent* toDealloc) MOZ_OVERRIDE;
  
  bool RecvStop() MOZ_OVERRIDE;

  MessageLoop * GetMessageLoop();

private:
  MessageLoop * mMessageLoop;
};

} 
} 

