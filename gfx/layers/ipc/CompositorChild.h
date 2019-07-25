






































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
class CompositorParent;

class CompositorChild : public PCompositorChild
{
  NS_INLINE_DECL_REFCOUNTING(CompositorChild)
public:
  virtual ~CompositorChild();

  void Destroy();

  static CompositorChild* CreateCompositor(LayerManager *aLayerManager,
                                           CompositorParent *aCompositorParent);

protected:
  CompositorChild(Thread* aCompositorThread, LayerManager *aLayerManager);

  virtual PLayersChild* AllocPLayers(const LayersBackend &aBackend, const WidgetDescriptor &aWidget);
  virtual bool DeallocPLayers(PLayersChild *aChild);

  virtual bool RecvNativeContextCreated(const NativeContext &aNativeContext);

private:
  Thread *mCompositorThread;
  LayerManager *mLayerManager;
  nsRefPtr<CompositorParent> mCompositorParent;

  DISALLOW_EVIL_CONSTRUCTORS(CompositorChild);
};

} 
} 

#endif 
