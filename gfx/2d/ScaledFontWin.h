




































#ifndef MOZILLA_GFX_SCALEDFONTWIN_H_
#define MOZILLA_GFX_SCALEDFONTWIN_H_

#include "ScaledFontSkia.h"
#include "gfxGDIFont.h"

namespace mozilla {
namespace gfx {

class ScaledFontWin : public ScaledFontSkia
{
public:
  ScaledFontWin(gfxGDIFont* aFont, Float aSize);

  virtual FontType GetType() const { return FONT_GDI; }

private:
  friend class DrawTargetSkia;
};

}
}

#endif 
