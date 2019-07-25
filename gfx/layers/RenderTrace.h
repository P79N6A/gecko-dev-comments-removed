












































#ifdef MOZ_RENDERTRACE

#include "gfx3DMatrix.h"
#include "nsRect.h"

#ifndef GFX_RENDERTRACE_H
#define GFX_RENDERTRACE_H

namespace mozilla {
namespace layers {

class Layer;

void RenderTraceLayers(Layer *aLayer, const char *aColor, gfx3DMatrix aRootTransform = gfx3DMatrix(), bool aReset = true);

void RenderTraceInvalidateStart(Layer *aLayer, const char *aColor, nsIntRect aRect);
void RenderTraceInvalidateEnd(Layer *aLayer, const char *aColor);

}
}

#endif 

#endif 
