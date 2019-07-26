




#include "ScaledFontFreetype.h"
#include "Logging.h"

#include "gfxFont.h"

#ifdef USE_SKIA
#include "skia/SkTypeface.h"
#endif

#include <string>

using namespace std;

namespace mozilla {
namespace gfx {

#ifdef USE_SKIA
static SkTypeface::Style
fontStyleToSkia(FontStyle aStyle)
{
  switch (aStyle) {
  case FONT_STYLE_NORMAL:
    return SkTypeface::kNormal;
  case FONT_STYLE_ITALIC:
    return SkTypeface::kItalic;
  case FONT_STYLE_BOLD:
    return SkTypeface::kBold;
  case FONT_STYLE_BOLD_ITALIC:
    return SkTypeface::kBoldItalic;
   }

  gfxWarning() << "Unknown font style";
  return SkTypeface::kNormal;
}
#endif



ScaledFontFreetype::ScaledFontFreetype(FontOptions* aFont, Float aSize)
  : ScaledFontBase(aSize)
{
#ifdef USE_SKIA
  mTypeface = SkTypeface::CreateFromName(aFont->mName.c_str(), fontStyleToSkia(aFont->mStyle));
#endif
}

}
}
