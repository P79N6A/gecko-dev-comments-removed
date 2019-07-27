






#ifndef mozilla_layers_ShadowLayerParent_h
#define mozilla_layers_ShadowLayerParent_h

#include "mozilla/Attributes.h"         
#include "mozilla/ipc/ProtocolUtils.h"
#include "mozilla/layers/PLayerParent.h"  
#include "nsAutoPtr.h"                  

namespace mozilla {
namespace layers {

class ContainerLayer;
class Layer;

class CanvasLayerComposite;
class ColorLayerComposite;
class ContainerLayerComposite;
class ImageLayerComposite;
class RefLayerComposite;
class PaintedLayerComposite;

class ShadowLayerParent : public PLayerParent
{
public:
  ShadowLayerParent();

  virtual ~ShadowLayerParent();

  void Bind(Layer* layer);
  void Destroy();

  Layer* AsLayer() const { return mLayer; }

  ContainerLayerComposite* AsContainerLayerComposite() const;
  CanvasLayerComposite* AsCanvasLayerComposite() const;
  ColorLayerComposite* AsColorLayerComposite() const;
  ImageLayerComposite* AsImageLayerComposite() const;
  RefLayerComposite* AsRefLayerComposite() const;
  PaintedLayerComposite* AsPaintedLayerComposite() const;

private:
  virtual void ActorDestroy(ActorDestroyReason why) override;

  nsRefPtr<Layer> mLayer;
};

} 
} 

#endif 
