




#include "ScaledFontBase.h"

#ifdef USE_SKIA
#include "PathSkia.h"
#include "skia/SkEmptyShader.h"
#include "skia/SkPaint.h"
#endif

#ifdef USE_CAIRO
#include "PathCairo.h"
#include "DrawTargetCairo.h"
#include "HelpersCairo.h"
#endif

#ifdef CAIRO_HAS_FT_FONT
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include "cairo-ft.h"
#endif

#include <vector>
#include <cmath>

using namespace std;

namespace mozilla {
namespace gfx {

ScaledFontBase::~ScaledFontBase()
{
#ifdef USE_SKIA
  SkSafeUnref(mTypeface);
#endif
#ifdef USE_CAIRO_SCALED_FONT
  cairo_scaled_font_destroy(mScaledFont);
#endif
}

ScaledFontBase::ScaledFontBase(Float aSize)
  : mSize(aSize)
{
#ifdef USE_SKIA
  mTypeface = nullptr;
#endif
#ifdef USE_CAIRO_SCALED_FONT
  mScaledFont = nullptr;
#endif
}

#ifdef USE_SKIA
SkPath
ScaledFontBase::GetSkiaPathForGlyphs(const GlyphBuffer &aBuffer)
{
  SkTypeface *typeFace = GetSkTypeface();
  MOZ_ASSERT(typeFace);

  SkPaint paint;
  paint.setTypeface(typeFace);
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
  return path;
}
#endif

TemporaryRef<Path>
ScaledFontBase::GetPathForGlyphs(const GlyphBuffer &aBuffer, const DrawTarget *aTarget)
{
#ifdef USE_SKIA
  if (aTarget->GetType() == BackendType::SKIA) {
    SkPath path = GetSkiaPathForGlyphs(aBuffer);
    return new PathSkia(path, FillRule::FILL_WINDING);
  }
#endif
#ifdef USE_CAIRO
  if (aTarget->GetType() == BackendType::CAIRO) {
    MOZ_ASSERT(mScaledFont);

    DrawTarget *dt = const_cast<DrawTarget*>(aTarget);
    cairo_t *ctx = static_cast<cairo_t*>(dt->GetNativeSurface(NativeSurfaceType::CAIRO_CONTEXT));

    bool isNewContext = !ctx;
    if (!ctx) {
      ctx = cairo_create(DrawTargetCairo::GetDummySurface());
      cairo_matrix_t mat;
      GfxMatrixToCairoMatrix(aTarget->GetTransform(), mat);
      cairo_set_matrix(ctx, &mat);
    }

    cairo_set_scaled_font(ctx, mScaledFont);

    
    std::vector<cairo_glyph_t> glyphs(aBuffer.mNumGlyphs);
    for (uint32_t i = 0; i < aBuffer.mNumGlyphs; ++i) {
      glyphs[i].index = aBuffer.mGlyphs[i].mIndex;
      glyphs[i].x = aBuffer.mGlyphs[i].mPosition.x;
      glyphs[i].y = aBuffer.mGlyphs[i].mPosition.y;
    }

    cairo_new_path(ctx);

    cairo_glyph_path(ctx, &glyphs[0], aBuffer.mNumGlyphs);

    RefPtr<PathCairo> newPath = new PathCairo(ctx);
    if (isNewContext) {
      cairo_destroy(ctx);
    }

    return newPath;
  }
#endif
  return nullptr;
}

void
ScaledFontBase::CopyGlyphsToBuilder(const GlyphBuffer &aBuffer, PathBuilder *aBuilder, BackendType aBackendType, const Matrix *aTransformHint)
{
#ifdef USE_SKIA
  if (aBackendType == BackendType::SKIA) {
    PathBuilderSkia *builder = static_cast<PathBuilderSkia*>(aBuilder);
    builder->AppendPath(GetSkiaPathForGlyphs(aBuffer));
    return;
  }
#endif
#ifdef USE_CAIRO
  if (aBackendType == BackendType::CAIRO) {
    MOZ_ASSERT(mScaledFont);

    PathBuilderCairo* builder = static_cast<PathBuilderCairo*>(aBuilder);
    cairo_t *ctx = cairo_create(DrawTargetCairo::GetDummySurface());

    if (aTransformHint) {
      cairo_matrix_t mat;
      GfxMatrixToCairoMatrix(*aTransformHint, mat);
      cairo_set_matrix(ctx, &mat);
    }

    
    std::vector<cairo_glyph_t> glyphs(aBuffer.mNumGlyphs);
    for (uint32_t i = 0; i < aBuffer.mNumGlyphs; ++i) {
      glyphs[i].index = aBuffer.mGlyphs[i].mIndex;
      glyphs[i].x = aBuffer.mGlyphs[i].mPosition.x;
      glyphs[i].y = aBuffer.mGlyphs[i].mPosition.y;
    }

    cairo_set_scaled_font(ctx, mScaledFont);
    cairo_glyph_path(ctx, &glyphs[0], aBuffer.mNumGlyphs);

    RefPtr<PathCairo> cairoPath = new PathCairo(ctx);
    cairo_destroy(ctx);

    cairoPath->AppendPathToBuilder(builder);
    return;
  }
#endif

  MOZ_CRASH("The specified backend type is not supported by CopyGlyphsToBuilder");
}

#ifdef USE_CAIRO_SCALED_FONT
void
ScaledFontBase::SetCairoScaledFont(cairo_scaled_font_t* font)
{
  MOZ_ASSERT(!mScaledFont);

  if (font == mScaledFont)
    return;
 
  if (mScaledFont)
    cairo_scaled_font_destroy(mScaledFont);

  mScaledFont = font;
  cairo_scaled_font_reference(mScaledFont);
}

bool
ScaledFontBase::GetFontFileData(FontFileDataOutput aDataCallback, void *aBaton)
{
#ifdef USE_CAIRO_SCALED_FONT

  switch (cairo_scaled_font_get_type(mScaledFont)) {
#ifdef CAIRO_HAS_FT_FONT
  case CAIRO_FONT_TYPE_FT:
    {
      FT_Face face = cairo_ft_scaled_font_lock_face(mScaledFont);

      if (!face) {
	gfxWarning() << "Failed to lock FT_Face for cairo font.";
	cairo_ft_scaled_font_unlock_face(mScaledFont);	
	return false;
      }

      if (!FT_IS_SFNT(face)) {
	if (face->stream && face->stream->base) {
	  aDataCallback((uint8_t*)&face->stream->base, face->stream->size, 0, mSize, aBaton);
	  cairo_ft_scaled_font_unlock_face(mScaledFont);
	  return true;
	}

	gfxWarning() << "Non sfnt font with non-memory stream.";
	cairo_ft_scaled_font_unlock_face(mScaledFont);
	return false;
      }

      
      FT_ULong length = 0;
      FT_Error error = FT_Load_Sfnt_Table(face, 0, 0, nullptr, &length);
      if (error || !length) {
	cairo_ft_scaled_font_unlock_face(mScaledFont);
	gfxWarning() << "Failed to get font file from Freetype font. " << error;
	return false;
      }

      
      std::vector<FT_Byte> buffer(length);
      FT_Load_Sfnt_Table(face, 0, 0, &buffer.front(), &length);

      aDataCallback((uint8_t*)&buffer.front(), length, 0, mSize, aBaton);

      cairo_ft_scaled_font_unlock_face(mScaledFont);

      return true;
    }
#endif
  default:
    return false;
  }
#endif

  return false;
}
#endif

}
}
