




































#ifndef MOZILLA_GFX_SCALEDFONTWIN_H_
#define MOZILLA_GFX_SCALEDFONTWIN_H_

#include "ScaledFontBase.h"
#include "gfxGDIFont.h"

namespace mozilla {
namespace gfx {

class ScaledFontWin : public ScaledFontBase
{
public:
  ScaledFontWin(gfxGDIFont* aFont, Float aSize);

  virtual FontType GetType() const { return FONT_GDI; }
#ifdef USE_SKIA
  virtual SkTypeface* GetSkTypeface();
#endif
private:
#ifdef USE_SKIA
  friend class DrawTargetSkia;
#endif
};

}
}

#endif 
