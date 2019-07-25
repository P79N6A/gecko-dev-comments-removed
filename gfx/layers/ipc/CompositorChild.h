






































#ifndef mozilla_layers_CompositorChild_h
#define mozilla_layers_CompositorChild_h

#include "mozilla/layers/PCompositorChild.h"


namespace base {
  class Thread;
}

using base::Thread;

namespace mozilla {
namespace layers {

class LayerManager;

class CompositorChild : public PCompositorChild
{

public:
  virtual ~CompositorChild();

  static CompositorChild* CreateCompositor(LayerManager *aLayerManager);

protected:
  CompositorChild(Thread* aCompositorThread, LayerManager *aLayerManager);

  virtual PLayersChild* AllocPLayers(const LayersBackend &aBackend, const WidgetDescriptor &aWidget);
  virtual bool DeallocPLayers(PLayersChild *aChild);

  virtual bool RecvNativeContextCreated(const NativeContext &aNativeContext);

private:
  Thread *mCompositorThread;
  LayerManager *mLayerManager;

  DISALLOW_EVIL_CONSTRUCTORS(CompositorChild);
};

} 
} 

#endif 
