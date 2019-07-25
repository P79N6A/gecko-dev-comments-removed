











































#define MOZ_RENDERTRACE

#ifndef GFX_RENDERTRACE_H


#include "gfx3DMatrix.h"
#include "nsRect.h"

namespace mozilla {
namespace layers {

class Layer;

void RenderTraceLayers(Layer *aLayer, const char *aColor, const gfx3DMatrix aRootTransform = gfx3DMatrix(), bool aReset = true);

void RenderTraceInvalidateStart(Layer *aLayer, const char *aColor, const nsIntRect aRect);
void RenderTraceInvalidateEnd(Layer *aLayer, const char *aColor);

#ifndef MOZ_RENDERTRACE
inline void RenderTraceLayers(Layer *aLayer, const char *aColor, const gfx3DMatrix aRootTransform, bool aReset)
{}

inline void RenderTraceInvalidateStart(Layer *aLayer, const char *aColor, const nsIntRect aRect)
{}

inline void RenderTraceInvalidateEnd(Layer *aLayer, const char *aColor)
{}

#endif 

}
}

#endif 
