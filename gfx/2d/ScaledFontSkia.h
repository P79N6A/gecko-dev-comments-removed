




































#ifndef MOZILLA_GFX_SCALEDFONTSKIA_H_
#define MOZILLA_GFX_SCALEDFONTSKIA_H_

#include "2D.h"
#include "skia/SkTypeface.h"

class gfxFont;

namespace mozilla {
namespace gfx {

class ScaledFontSkia : public ScaledFont
{
public:
  ScaledFontSkia(gfxFont* aFont, Float aSize);
  ScaledFontSkia(Float aSize);
  virtual ~ScaledFontSkia();

  virtual FontType GetType() const { return FONT_SKIA; }

  virtual TemporaryRef<Path> GetPathForGlyphs(const GlyphBuffer &aBuffer, const DrawTarget *aTarget);

protected:
  friend class DrawTargetSkia;

  SkTypeface* mTypeface;
  Float mSize;
};

}
}

#endif 
