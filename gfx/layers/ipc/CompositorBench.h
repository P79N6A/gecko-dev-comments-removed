





#ifndef mozilla_layers_CompositorBench_h
#define mozilla_layers_CompositorBench_h

#include "mozilla/gfx/Rect.h"           

namespace mozilla {
namespace layers {

class Compositor;




#ifdef MOZ_COMPOSITOR_BENCH
void CompositorBench(Compositor* aCompositor, const gfx::Rect& aScreenRect);
#else
static inline void CompositorBench(Compositor* aCompositor, const gfx::Rect& aScreenRect) {}
#endif

}
}

#endif


