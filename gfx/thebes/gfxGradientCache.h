





#ifndef GFX_GRADIENT_CACHE_H
#define GFX_GRADIENT_CACHE_H

#include "nsTArray.h"
#include "gfxPattern.h"
#include "mozilla/gfx/2D.h"

namespace mozilla {
namespace gfx {

class gfxGradientCache {
public:
    static gfx::GradientStops*
    GetGradientStops(gfx::DrawTarget *aDT,
                     nsTArray<gfx::GradientStop>& aStops,
                     gfx::ExtendMode aExtend);

    static gfx::GradientStops*
    GetOrCreateGradientStops(gfx::DrawTarget *aDT,
                             nsTArray<gfx::GradientStop>& aStops,
                             gfx::ExtendMode aExtend);

    static void PurgeAllCaches();
    static void Shutdown();
};

}
}

#endif
