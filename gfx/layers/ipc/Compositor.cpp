







































#include "CompositorParent.h"
#include "Compositor.h"
#include "nsDebug.h"

namespace mozilla {
namespace layers {
namespace compositor {

LayerManager*
GetLayerManager(CompositorParent* aParent)
{
  return aParent->GetLayerManager();
}


}
}
}
