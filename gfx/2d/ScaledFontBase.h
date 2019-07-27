




#ifndef MOZILLA_GFX_SCALEDFONTBASE_H_
#define MOZILLA_GFX_SCALEDFONTBASE_H_

#include "2D.h"


#if defined(USE_SKIA) || defined(USE_CAIRO)
#define USE_CAIRO_SCALED_FONT
#endif

#ifdef USE_SKIA
#include "skia/SkPath.h"
#include "skia/SkTypeface.h"
#endif
#ifdef USE_CAIRO_SCALED_FONT
#include "cairo.h"
#endif

namespace mozilla {
namespace gfx {

class ScaledFontBase : public ScaledFont
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(ScaledFontBase)
  explicit ScaledFontBase(Float aSize);
  virtual ~ScaledFontBase();

  virtual TemporaryRef<Path> GetPathForGlyphs(const GlyphBuffer &aBuffer, const DrawTarget *aTarget);

  virtual void CopyGlyphsToBuilder(const GlyphBuffer &aBuffer, PathBuilder *aBuilder, BackendType aBackendType, const Matrix *aTransformHint);

  float GetSize() { return mSize; }

#ifdef USE_SKIA
  virtual SkTypeface* GetSkTypeface() { return mTypeface; }
#endif

  
  virtual FontType GetType() const { return FontType::SKIA; }

#ifdef USE_CAIRO_SCALED_FONT
  cairo_scaled_font_t* GetCairoScaledFont() { return mScaledFont; }
  void SetCairoScaledFont(cairo_scaled_font_t* font);
#endif

protected:
  friend class DrawTargetSkia;
#ifdef USE_SKIA
  SkTypeface* mTypeface;
  SkPath GetSkiaPathForGlyphs(const GlyphBuffer &aBuffer);
#endif
#ifdef USE_CAIRO_SCALED_FONT
  cairo_scaled_font_t* mScaledFont;
#endif
  Float mSize;
};

}
}

#endif 
