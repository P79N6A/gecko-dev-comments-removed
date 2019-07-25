




































#include "ScaledFontFreetype.h"

#include "gfxFont.h"

#ifdef USE_SKIA
#include "skia/SkTypeface.h"
#endif

using namespace std;

namespace mozilla {
namespace gfx {

#ifdef USE_SKIA
static SkTypeface::Style
gfxFontStyleToSkia(const gfxFontStyle* aStyle)
{
  if (aStyle->style == NS_FONT_STYLE_ITALIC) {
    if (aStyle->weight == NS_FONT_WEIGHT_BOLD) {
      return SkTypeface::kBoldItalic;
    }
    return SkTypeface::kItalic;
  }
  if (aStyle->weight == NS_FONT_WEIGHT_BOLD) {
    return SkTypeface::kBold;
  }
  return SkTypeface::kNormal;
}
#endif



ScaledFontFreetype::ScaledFontFreetype(gfxFont* aFont, Float aSize)
  : ScaledFontBase(aSize)
{
#ifdef USE_SKIA
  NS_LossyConvertUTF16toASCII name(aFont->GetName());
  mTypeface = SkTypeface::CreateFromName(name.get(), gfxFontStyleToSkia(aFont->GetStyle()));
#endif
}

}
}
