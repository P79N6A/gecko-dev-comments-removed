




#include "ScaledFontWin.h"
#include "ScaledFontBase.h"

#ifdef USE_SKIA
#include "skia/SkTypeface_win.h"
#endif

namespace mozilla {
namespace gfx {

ScaledFontWin::ScaledFontWin(LOGFONT* aFont, Float aSize)
  : ScaledFontBase(aSize)
  , mLogFont(*aFont)
{
}

#ifdef USE_SKIA
SkTypeface* ScaledFontWin::GetSkTypeface()
{
  if (!mTypeface) {
    mTypeface = SkCreateTypefaceFromLOGFONT(mLogFont);
  }
  return mTypeface;
}
#endif


}
}
