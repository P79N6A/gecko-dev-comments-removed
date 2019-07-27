




#include "SurfaceTypes.h"

#include "mozilla/layers/ISurfaceAllocator.h"

namespace mozilla {
namespace gl {

SurfaceCaps::SurfaceCaps()
{
    Clear();
}

SurfaceCaps::SurfaceCaps(const SurfaceCaps& other)
{
    *this = other;
}

SurfaceCaps&
SurfaceCaps::operator=(const SurfaceCaps& other)
{
    any = other.any;
    color = other.color;
    alpha = other.alpha;
    bpp16 = other.bpp16;
    depth = other.depth;
    stencil = other.stencil;
    antialias = other.antialias;
    premultAlpha = other.premultAlpha;
    preserve = other.preserve;
    surfaceAllocator = other.surfaceAllocator;

    return *this;
}

void
SurfaceCaps::Clear()
{
    any = false;
    color = false;
    alpha = false;
    bpp16 = false;
    depth = false;
    stencil = false;
    antialias = false;
    premultAlpha = false;
    preserve = false;
    surfaceAllocator = nullptr;
}

SurfaceCaps::~SurfaceCaps()
{
}

} 
} 
