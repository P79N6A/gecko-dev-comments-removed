






#ifndef MOZILLA_LAYERS_LAYERTRANSACTIONCHILD_H
#define MOZILLA_LAYERS_LAYERTRANSACTIONCHILD_H

#include "mozilla/layers/PLayerTransactionChild.h"

namespace mozilla {
namespace layers {

class LayerTransactionChild : public PLayerTransactionChild
{
public:
  LayerTransactionChild() { }
  ~LayerTransactionChild() { }

  






  void Destroy();

protected:
  virtual PGrallocBufferChild*
  AllocPGrallocBuffer(const gfxIntSize&, const gfxContentType&,
                      MaybeMagicGrallocBufferHandle*) MOZ_OVERRIDE;
  virtual bool
  DeallocPGrallocBuffer(PGrallocBufferChild* actor) MOZ_OVERRIDE;

  virtual PLayerChild* AllocPLayer() MOZ_OVERRIDE;
  virtual bool DeallocPLayer(PLayerChild* actor) MOZ_OVERRIDE;

  virtual PCompositableChild* AllocPCompositable(const TextureInfo& aInfo) MOZ_OVERRIDE;
  virtual bool DeallocPCompositable(PCompositableChild* actor) MOZ_OVERRIDE;
  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;
};

} 
} 

#endif 
