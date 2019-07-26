




#ifndef GFX_TextRenderer_H
#define GFX_TextRenderer_H

#include "mozilla/gfx/2D.h"
#include <string>

namespace mozilla {
namespace layers {

class Compositor;

class TextRenderer : public RefCounted<TextRenderer>
{
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(TextRenderer)
  TextRenderer(Compositor *aCompositor)
    : mCompositor(aCompositor)
  {
  }

  ~TextRenderer();

  void RenderText(const std::string& aText, const gfx::IntPoint& aOrigin,
                  const gfx::Matrix4x4& aTransform, uint32_t aTextSize,
                  uint32_t aTargetPixelWidth);

  gfx::DataSourceSurface::MappedSurface& GetSurfaceMap() { return mMap; }

private:

  void EnsureInitialized();

  RefPtr<Compositor> mCompositor;
  RefPtr<gfx::DataSourceSurface> mGlyphBitmaps;
  gfx::DataSourceSurface::MappedSurface mMap;
};

}
}

#endif
