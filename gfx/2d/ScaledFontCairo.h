




#ifndef MOZILLA_GFX_SCALEDFONTCAIRO_H_
#define MOZILLA_GFX_SCALEDFONTCAIRO_H_

#include "ScaledFontBase.h"

#include "cairo.h"

namespace mozilla {
namespace gfx {

class ScaledFontCairo : public ScaledFontBase
{
public:

  ScaledFontCairo(cairo_scaled_font_t* aScaledFont, Float aSize);
};






class GlyphRenderingOptionsCairo : public GlyphRenderingOptions
{
public:
  GlyphRenderingOptionsCairo()
    : mHinting(FONT_HINTING_NORMAL)
    , mAutoHinting(false)
  {
  }

  void SetHinting(FontHinting aHinting) { mHinting = aHinting; }
  void SetAutoHinting(bool aAutoHinting) { mAutoHinting = aAutoHinting; }
  FontHinting GetHinting() const { return mHinting; }
  bool GetAutoHinting() const { return mAutoHinting; }
  virtual FontType GetType() const { return FONT_CAIRO; }
private:
  FontHinting mHinting;
  bool mAutoHinting;
};

}
}

#endif 
