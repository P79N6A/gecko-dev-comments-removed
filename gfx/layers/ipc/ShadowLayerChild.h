






#ifndef mozilla_layers_ShadowLayerChild_h
#define mozilla_layers_ShadowLayerChild_h

#include "mozilla/Attributes.h"         
#include "mozilla/ipc/ProtocolUtils.h"
#include "mozilla/layers/PLayerChild.h"  

namespace mozilla {
namespace layers {

class ShadowableLayer;

class ShadowLayerChild : public PLayerChild
{
public:
  explicit ShadowLayerChild(ShadowableLayer* aLayer);
  virtual ~ShadowLayerChild();

  ShadowableLayer* layer() const { return mLayer; }

protected:
  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

private:
  ShadowableLayer* mLayer;
};

} 
} 

#endif 
