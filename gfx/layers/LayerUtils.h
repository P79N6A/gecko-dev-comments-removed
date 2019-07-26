




#ifndef MOZILLA_LAYERS_LAYERUTILS_H_
#define MOZILLA_LAYERS_LAYERUTILS_H_

#include "mozilla/gfx/2D.h"

namespace mozilla {
namespace layers {

void
PremultiplySurface(gfx::DataSourceSurface* srcSurface,
                   gfx::DataSourceSurface* destSurface = nullptr);

}
}

#endif 
