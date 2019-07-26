





#ifndef mozilla_layers_CompositorCocoaWidgetHelper_h
#define mozilla_layers_CompositorCocoaWidgetHelper_h




namespace mozilla {
namespace layers {

class CompositorParent;
class LayerManagerComposite;

namespace compositor {



LayerManagerComposite* GetLayerManager(CompositorParent* aParent);

}
}
}
#endif 

