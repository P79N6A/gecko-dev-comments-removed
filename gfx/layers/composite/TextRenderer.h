




#ifndef GFX_TextRenderer_H
#define GFX_TextRenderer_H

#include "mozilla/gfx/2D.h"
#include "nsISupportsImpl.h"
#include <string>

namespace mozilla {
namespace layers {

class Compositor;

class TextRenderer
{
  ~TextRenderer();

public:
  NS_INLINE_DECL_REFCOUNTING(TextRenderer)

  explicit TextRenderer(Compositor *aCompositor)
    : mCompositor(aCompositor)
  {
  }

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
