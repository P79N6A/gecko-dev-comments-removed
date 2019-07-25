



































#include "ScaledFontCairo.h"

#include "cairo.h"

#include "PathCairo.h"
#include "gfxFont.h"

#include <vector>

using namespace std;

namespace mozilla {
namespace gfx {

ScaledFontCairo::ScaledFontCairo(gfxFont* aFont)
{
  mScaledFont = aFont->GetCairoScaledFont();
  cairo_scaled_font_reference(mScaledFont);
}

ScaledFontCairo::~ScaledFontCairo()
{
  cairo_scaled_font_destroy(mScaledFont);
}

TemporaryRef<Path>
ScaledFontCairo::GetPathForGlyphs(const GlyphBuffer &aBuffer, const DrawTarget *aTarget)
{
  if (aTarget->GetType() != BACKEND_CAIRO) {
    return NULL;
  }

  RefPtr<PathBuilder> builder_iface = aTarget->CreatePathBuilder();
  PathBuilderCairo* builder = static_cast<PathBuilderCairo*>(builder_iface.get());

  
  RefPtr<CairoPathContext> context = builder->GetPathContext();

  cairo_set_scaled_font(*context, mScaledFont);

  
  std::vector<cairo_glyph_t> glyphs(aBuffer.mNumGlyphs);
  for (uint32_t i = 0; i < aBuffer.mNumGlyphs; ++i) {
    glyphs[i].index = aBuffer.mGlyphs[i].mIndex;
    glyphs[i].x = aBuffer.mGlyphs[i].mPosition.x;
    glyphs[i].y = aBuffer.mGlyphs[i].mPosition.y;
  }

  cairo_glyph_path(*context, &glyphs[0], aBuffer.mNumGlyphs);

  return builder->Finish();
}

cairo_scaled_font_t*
ScaledFontCairo::GetCairoScaledFont()
{
  return mScaledFont;
}

}
}
