




































#include "ScaledFontSkia.h"
#include "PathSkia.h"
#include "skia/SkPaint.h"
#include "skia/SkPath.h"
#include <vector>
#include <cmath>
using namespace std;
#include "gfxFont.h"

namespace mozilla {
namespace gfx {

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

ScaledFontSkia::ScaledFontSkia(gfxFont* aFont, Float aSize)
  : mSize(aSize)
{
  NS_LossyConvertUTF16toASCII name(aFont->GetName());
  mTypeface = SkTypeface::CreateFromName(name.get(), gfxFontStyleToSkia(aFont->GetStyle()));
}

ScaledFontSkia::ScaledFontSkia(Float aSize)
  : mSize(aSize)
{
}

ScaledFontSkia::~ScaledFontSkia()
{
  SkSafeUnref(mTypeface);
}

TemporaryRef<Path>
ScaledFontSkia::GetPathForGlyphs(const GlyphBuffer &aBuffer, const DrawTarget *aTarget)
{
  if (aTarget->GetType() != BACKEND_SKIA) {
    return NULL;
  }

  SkPaint paint;
  paint.setTypeface(mTypeface);
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

}
}
