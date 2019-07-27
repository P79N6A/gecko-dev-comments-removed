




#ifndef MOZILLA_GFX_SCALEDFONTCAIRO_H_
#define MOZILLA_GFX_SCALEDFONTCAIRO_H_

#include "ScaledFontBase.h"

#include "cairo.h"

namespace mozilla {
namespace gfx {

class ScaledFontCairo : public ScaledFontBase
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(ScaledFontCairo)

  ScaledFontCairo(cairo_scaled_font_t* aScaledFont, Float aSize);

#if defined(USE_SKIA) && defined(MOZ_ENABLE_FREETYPE)
  virtual SkTypeface* GetSkTypeface();
#endif
};






class GlyphRenderingOptionsCairo : public GlyphRenderingOptions
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(GlyphRenderingOptionsCairo)
  GlyphRenderingOptionsCairo()
    : mHinting(FontHinting::NORMAL)
    , mAutoHinting(false)
  {
  }

  void SetHinting(FontHinting aHinting) { mHinting = aHinting; }
  void SetAutoHinting(bool aAutoHinting) { mAutoHinting = aAutoHinting; }
  FontHinting GetHinting() const { return mHinting; }
  bool GetAutoHinting() const { return mAutoHinting; }
  virtual FontType GetType() const { return FontType::CAIRO; }
private:
  FontHinting mHinting;
  bool mAutoHinting;
};

} 
} 

#endif 
