




#ifndef GFX_LAYERSORTER_H
#define GFX_LAYERSORTER_H

#include "nsTArray.h"

namespace mozilla {
namespace layers {

class Layer;

void SortLayersBy3DZOrder(nsTArray<Layer*>& aLayers);

} 
} 

#endif 
