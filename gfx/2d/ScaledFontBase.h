




































#ifndef MOZILLA_GFX_SCALEDFONTBASE_H_
#define MOZILLA_GFX_SCALEDFONTBASE_H_

#include "2D.h"
#ifdef USE_SKIA
#include "skia/SkTypeface.h"
#endif

class gfxFont;

namespace mozilla {
namespace gfx {

class ScaledFontBase : public ScaledFont
{
public:
  ScaledFontBase(Float aSize);
  virtual ~ScaledFontBase();

  virtual TemporaryRef<Path> GetPathForGlyphs(const GlyphBuffer &aBuffer, const DrawTarget *aTarget);
#ifdef USE_SKIA
  ScaledFontBase(gfxFont* aFont, Float aSize);
  virtual SkTypeface* GetSkTypeface() { return mTypeface; }
  virtual FontType GetType() const { return FONT_SKIA; }
#endif

protected:
  friend class DrawTargetSkia;
#ifdef USE_SKIA
  SkTypeface* mTypeface;
#endif
  Float mSize;
};

}
}

#endif 
