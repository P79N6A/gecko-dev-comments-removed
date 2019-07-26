






#ifndef mozilla_layers_ShadowLayerParent_h
#define mozilla_layers_ShadowLayerParent_h

#include "mozilla/layers/PLayerParent.h"

namespace mozilla {
namespace layers {

class ContainerLayer;
class Layer;
class LayerManager;

class ShadowLayerParent : public PLayerParent
{
public:
  ShadowLayerParent();

  virtual ~ShadowLayerParent();

  void Bind(Layer* layer);
  void Destroy();

  Layer* AsLayer() const { return mLayer; }
  ContainerLayer* AsContainer() const;

private:
  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

  nsRefPtr<Layer> mLayer;
};

} 
} 

#endif 
