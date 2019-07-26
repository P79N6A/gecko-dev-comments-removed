






#ifndef mozilla_layers_ShadowLayersChild_h
#define mozilla_layers_ShadowLayersChild_h

#include "mozilla/layers/PLayersChild.h"

namespace mozilla {
namespace layers {

class ShadowLayersChild : public PLayersChild
{
public:
  ShadowLayersChild() { }
  ~ShadowLayersChild() { }

  






  void Destroy();

protected:
  virtual PGrallocBufferChild*
  AllocPGrallocBuffer(const gfxIntSize&, const gfxContentType&,
                      MaybeMagicGrallocBufferHandle*) MOZ_OVERRIDE;
  virtual bool
  DeallocPGrallocBuffer(PGrallocBufferChild* actor) MOZ_OVERRIDE;

  virtual PLayerChild* AllocPLayer() MOZ_OVERRIDE;
  virtual bool DeallocPLayer(PLayerChild* actor) MOZ_OVERRIDE;

  virtual PCompositableChild* AllocPCompositable(const CompositableType& aType) MOZ_OVERRIDE;
  virtual bool DeallocPCompositable(PCompositableChild* actor) MOZ_OVERRIDE;
};

} 
} 

#endif 
