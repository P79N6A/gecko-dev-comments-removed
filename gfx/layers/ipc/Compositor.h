






































#ifndef mozilla_layers_Compositor_h
#define mozilla_layers_Compositor_h



#include "Layers.h"
#include "nsDebug.h"

namespace mozilla {
namespace layers {

class CompositorParent;
class LayerManager;

namespace compositor {




LayerManager* GetLayerManager(CompositorParent* aParent);

}
}
}
#endif
