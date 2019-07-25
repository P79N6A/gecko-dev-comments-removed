




































#ifndef MOZILLA_GFX_SCALEDFONTWIN_H_
#define MOZILLA_GFX_SCALEDFONTWIN_H_

#include "ScaledFontBase.h"
#include <windows.h>

namespace mozilla {
namespace gfx {

class ScaledFontWin : public ScaledFontBase
{
public:
  ScaledFontWin(LOGFONT* aFont, Float aSize);

  virtual FontType GetType() const { return FONT_GDI; }
#ifdef USE_SKIA
  virtual SkTypeface* GetSkTypeface();
#endif
private:
#ifdef USE_SKIA
  friend class DrawTargetSkia;
#endif
  LOGFONT mLogFont;
};

}
}

#endif 
