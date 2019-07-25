







































#ifndef mozilla_layers_CompositorCocoaWidgetHelper_h
#define mozilla_layers_CompositorCocoaWidgetHelper_h




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

