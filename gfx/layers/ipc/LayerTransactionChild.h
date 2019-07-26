






#ifndef MOZILLA_LAYERS_LAYERTRANSACTIONCHILD_H
#define MOZILLA_LAYERS_LAYERTRANSACTIONCHILD_H

#include <stdint.h>                     
#include "gfxPoint.h"                   
#include "mozilla/Attributes.h"         
#include "mozilla/ipc/ProtocolUtils.h"
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
  AllocPGrallocBufferChild(const gfx::IntSize&,
                      const uint32_t&, const uint32_t&,
                      MaybeMagicGrallocBufferHandle*) MOZ_OVERRIDE;
  virtual bool
  DeallocPGrallocBufferChild(PGrallocBufferChild* actor) MOZ_OVERRIDE;

  virtual PLayerChild* AllocPLayerChild() MOZ_OVERRIDE;
  virtual bool DeallocPLayerChild(PLayerChild* actor) MOZ_OVERRIDE;

  virtual PCompositableChild* AllocPCompositableChild(const TextureInfo& aInfo) MOZ_OVERRIDE;
  virtual bool DeallocPCompositableChild(PCompositableChild* actor) MOZ_OVERRIDE;
  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;
};

} 
} 

#endif 
