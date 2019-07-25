




































#include "ScaledFontBase.h"
#ifdef USE_SKIA
#include "PathSkia.h"
#include "skia/SkPaint.h"
#include "skia/SkPath.h"
#endif
#include <vector>
#include <cmath>
using namespace std;
#include "gfxFont.h"

namespace mozilla {
namespace gfx {
#ifdef USE_SKIA
static SkTypeface::Style gfxFontStyleToSkia(const gfxFontStyle* aStyle)
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

ScaledFontBase::ScaledFontBase(gfxFont* aFont, Float aSize)
  : mSize(aSize)
{
  NS_LossyConvertUTF16toASCII name(aFont->GetName());
  mTypeface = SkTypeface::CreateFromName(name.get(), gfxFontStyleToSkia(aFont->GetStyle()));
}
#endif

ScaledFontBase::~ScaledFontBase()
{
#ifdef USE_SKIA
  SkSafeUnref(mTypeface);
#endif
}

ScaledFontBase::ScaledFontBase(Float aSize)
  : mSize(aSize)
{
#ifdef USE_SKIA
  mTypeface = NULL;
#endif
}


TemporaryRef<Path>
ScaledFontBase::GetPathForGlyphs(const GlyphBuffer &aBuffer, const DrawTarget *aTarget)
{
#ifdef USE_SKIA
  if (aTarget->GetType() == BACKEND_SKIA) {
    SkPaint paint;
    paint.setTypeface(GetSkTypeface());
    paint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
    paint.setTextSize(SkFloatToScalar(mSize));

    std::vector<uint16_t> indices;
    std::vector<SkPoint> offsets;
    indices.resize(aBuffer.mNumGlyphs);
    offsets.resize(aBuffer.mNumGlyphs);

    for (unsigned int i = 0; i < aBuffer.mNumGlyphs; i++) {
      indices[i] = aBuffer.mGlyphs[i].mIndex;
      offsets[i].fX = SkFloatToScalar(aBuffer.mGlyphs[i].mPosition.x);
      offsets[i].fY = SkFloatToScalar(aBuffer.mGlyphs[i].mPosition.y);
    }

    SkPath path;
    paint.getPosTextPath(&indices.front(), aBuffer.mNumGlyphs*2, &offsets.front(), &path);
    return new PathSkia(path, FILL_WINDING);
  }
#endif
  return NULL;
}

}
}
