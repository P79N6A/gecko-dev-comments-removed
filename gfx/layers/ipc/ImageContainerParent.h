






#ifndef mozilla_layers_ImageContainerParent_h
#define mozilla_layers_ImageContainerParent_h

#include "mozilla/Attributes.h"         
#include "mozilla/ipc/ProtocolUtils.h"
#include "mozilla/layers/PImageContainerParent.h"
#include "nsAutoPtr.h"                  

namespace mozilla {
namespace layers {

class ImageHost;

class ImageContainerParent : public PImageContainerParent
{
public:
  ImageContainerParent() {}
  ~ImageContainerParent();

  virtual bool RecvAsyncDelete() override;

  nsAutoTArray<ImageHost*,1> mImageHosts;

private:
  virtual void ActorDestroy(ActorDestroyReason why) override {}
};

} 
} 

#endif 
