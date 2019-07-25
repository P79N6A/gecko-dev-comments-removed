




































#include "ScaledFontWin.h"
#include "skia/SkTypeface_win.h"

namespace mozilla {
namespace gfx {

ScaledFontWin::ScaledFontWin(gfxGDIFont* aFont, Float aSize)
  : ScaledFontSkia(aSize)
{
  LOGFONT lf;
  GetObject(aFont->GetHFONT(), sizeof(LOGFONT), &lf);
  mTypeface = SkCreateTypefaceFromLOGFONT(lf);
}

}
}
