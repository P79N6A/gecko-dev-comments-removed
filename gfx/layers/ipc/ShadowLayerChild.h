







































#ifndef mozilla_layers_ShadowLayerChild_h
#define mozilla_layers_ShadowLayerChild_h

#include "mozilla/layers/PLayerChild.h"

namespace mozilla {
namespace layers {

class ShadowableLayer;

class ShadowLayerChild : public PLayerChild
{
public:
  ShadowLayerChild(ShadowableLayer* aLayer);
  virtual ~ShadowLayerChild();

  ShadowableLayer* layer() const { return mLayer; }

protected:
  NS_OVERRIDE
  virtual void ActorDestroy(ActorDestroyReason why);

private:
  ShadowableLayer* mLayer;
};

} 
} 

#endif 
