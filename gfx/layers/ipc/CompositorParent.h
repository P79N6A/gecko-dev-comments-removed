






































#ifndef mozilla_layers_CompositorParent_h
#define mozilla_layers_CompositorParent_h

#include "mozilla/layers/PCompositorParent.h"
#include "mozilla/layers/PLayersParent.h"

namespace mozilla {
namespace layers {

class CompositorParent : public PCompositorParent
{
  NS_INLINE_DECL_REFCOUNTING(CompositorParent)
public:
  CompositorParent();
  virtual ~CompositorParent();

  bool AnswerInit();

protected:
  virtual PLayersParent* AllocPLayers(const LayersBackend &backend, const WidgetDescriptor &widget);

  virtual bool DeallocPLayers(PLayersParent* aLayers);

private:
  void Composite();

  DISALLOW_EVIL_CONSTRUCTORS(CompositorParent);
};

} 
} 

#endif 
