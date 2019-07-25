



































#ifndef MOZILLA_GFX_SCALEDFONTCAIRO_H_
#define MOZILLA_GFX_SCALEDFONTCAIRO_H_

#include "2D.h"

class gfxFont;
typedef struct _cairo_scaled_font cairo_scaled_font_t;

namespace mozilla {
namespace gfx {

class ScaledFontCairo : public ScaledFont
{
public:
  ScaledFontCairo(gfxFont* aFont);
  virtual ~ScaledFontCairo();

  virtual FontType GetType() const { return FONT_CAIRO; }

  virtual TemporaryRef<Path> GetPathForGlyphs(const GlyphBuffer &aBuffer, const DrawTarget *aTarget);

  cairo_scaled_font_t* GetCairoScaledFont();

private:
  cairo_scaled_font_t* mScaledFont;
};

}
}

#endif 
