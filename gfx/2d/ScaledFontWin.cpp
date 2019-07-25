




































#include "ScaledFontWin.h"
#include "ScaeldFontBase.h"

#ifdef USE_SKIA
#include "skia/SkTypeface_win.h"
#endif

namespace mozilla {
namespace gfx {

ScaledFontWin::ScaledFontWin(gfxGDIFont* aFont, Float aSize)
  : ScaledFontBase(aSize)
{
  LOGFONT lf;
  GetObject(aFont->GetHFONT(), sizeof(LOGFONT), &lf);
}

#ifdef USE_SKIA
SkTypeface* ScaledFontWin::GetSkTypeface()
{
  if (!mTypeface) {
    mTypeface = SkCreateTypefaceFromLOGFONT(lf);
  }
  return mTypeface;
}
#endif


}
}
