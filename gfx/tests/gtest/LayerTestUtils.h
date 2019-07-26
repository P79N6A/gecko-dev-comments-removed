




#ifndef TEST_GFX_TEXTURES
#define TEST_GFX_TEXTURES

#include "CompositorTypes.h"

class gfxImageSurface;
class nsIWidget;

namespace mozilla {
namespace gfx {
  class DataSourceSurface;
}

namespace layers {

class PlanarYCbCrData;
class Compositor;

Compositor* CreateCompositor(LayersBackend aBakend, nsIWidget* aWidget);


void SetupSurface(gfxImageSurface* surface);


void SetupSurface(gfx::DataSourceSurface* surface);


void AssertSurfacesEqual(gfx::DataSourceSurface* surface1,
                         gfx::DataSourceSurface* surface2);


void AssertSurfacesEqual(gfxImageSurface* surface1,
                         gfxImageSurface* surface2);


void AssertYCbCrSurfacesEqual(PlanarYCbCrData* surface1,
                              PlanarYCbCrData* surface2);


} 
} 

#endif
