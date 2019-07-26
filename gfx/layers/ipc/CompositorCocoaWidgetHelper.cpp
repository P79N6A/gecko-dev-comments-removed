





#include "CompositorParent.h"
#include "CompositorCocoaWidgetHelper.h"
#include "nsDebug.h"

namespace mozilla {
namespace layers {
namespace compositor {

LayerManagerComposite*
GetLayerManager(CompositorParent* aParent)
{
  return aParent->GetLayerManager();
}


}
}
}
