




































#include "ScaledFontBase.h"

#include "gfxFont.h"

#ifdef USE_SKIA
#include "PathSkia.h"
#include "skia/SkPaint.h"
#include "skia/SkPath.h"
#endif

#ifdef USE_CAIRO
#include "PathCairo.h"
#endif

#include <vector>
#include <cmath>

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

#ifdef USE_SKIA
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
#ifdef USE_CAIRO
  cairo_scaled_font_destroy(mScaledFont);
#endif
}

ScaledFontBase::ScaledFontBase(Float aSize)
  : mSize(aSize)
{
#ifdef USE_SKIA
  mTypeface = NULL;
#endif
#ifdef USE_CAIRO
  mScaledFont = NULL;
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
#ifdef USE_CAIRO
  if (aTarget->GetType() == BACKEND_CAIRO) {
    MOZ_ASSERT(mScaledFont);

    RefPtr<PathBuilder> builder_iface = aTarget->CreatePathBuilder();
    PathBuilderCairo* builder = static_cast<PathBuilderCairo*>(builder_iface.get());

    
    RefPtr<CairoPathContext> context = builder->GetPathContext();

    cairo_set_scaled_font(*context, mScaledFont);

    
    std::vector<cairo_glyph_t> glyphs(aBuffer.mNumGlyphs);
    for (uint32_t i = 0; i < aBuffer.mNumGlyphs; ++i) {
      glyphs[i].index = aBuffer.mGlyphs[i].mIndex;
      glyphs[i].x = aBuffer.mGlyphs[i].mPosition.x;
      glyphs[i].y = aBuffer.mGlyphs[i].mPosition.y;
    }

    cairo_glyph_path(*context, &glyphs[0], aBuffer.mNumGlyphs);

    return builder->Finish();
  }
#endif
  return NULL;
}

#ifdef USE_CAIRO
void
ScaledFontBase::SetCairoScaledFont(cairo_scaled_font_t* font)
{
  MOZ_ASSERT(!mScaledFont);

  mScaledFont = font;
  cairo_scaled_font_reference(mScaledFont);
}
#endif

}
}
