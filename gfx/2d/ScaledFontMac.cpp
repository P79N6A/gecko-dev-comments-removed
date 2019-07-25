




































#include "ScaledFontMac.h"
#include "PathSkia.h"
#include "skia/SkPaint.h"
#include "skia/SkPath.h"
#include "skia/SkTypeface_mac.h"
#include <vector>

namespace mozilla {
namespace gfx {

ScaledFontMac::ScaledFontMac(CGFontRef aFont, Float aSize)
  : ScaledFontSkia(aSize)
{
  mFontFace = CTFontCreateWithGraphicsFont(aFont, aSize, NULL, NULL);
  mTypeface = SkCreateTypefaceFromCTFont(mFontFace);
}

ScaledFontMac::~ScaledFontMac()
{
  CFRelease(mFontFace);
}

}
}
