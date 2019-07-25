




































#ifndef GFX_LAYERSORTER_H
#define GFX_LAYERSORTER_H

#include "Layers.h"

namespace mozilla {
namespace layers {

void SortLayersBy3DZOrder(nsTArray<Layer*>& aLayers);

}
}
#endif 
