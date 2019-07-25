






































#ifndef mozilla_layers_CompositorChild_h
#define mozilla_layers_CompositorChild_h

#include "mozilla/layers/PCompositorChild.h"

namespace base {
  class Thread;
}

using base::Thread;

namespace mozilla {
namespace layers {

class CompositorChild : public PCompositorChild
{

public:
  virtual ~CompositorChild();

  static CompositorChild* CreateCompositor();

protected:
  CompositorChild(Thread* aCompositorThread);

  virtual PLayersChild* AllocPLayers(const LayersBackend &backend, const WidgetDescriptor &widget);
  virtual bool DeallocPLayers(PLayersChild *aChild);

private:
  Thread *mCompositorThread;

  DISALLOW_EVIL_CONSTRUCTORS(CompositorChild);
};

} 
} 

#endif 
