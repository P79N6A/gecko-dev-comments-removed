




#include "ScaledFontCairo.h"
#include "Logging.h"

#include "gfxFont.h"

#ifdef MOZ_ENABLE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#include "cairo-ft.h"
#endif

#if defined(USE_SKIA) && defined(MOZ_ENABLE_FREETYPE)
#include "skia/SkTypeface.h"
#include "skia/SkTypeface_cairo.h"
#endif

#include <string>

typedef struct FT_FaceRec_* FT_Face;

using namespace std;

namespace mozilla {
namespace gfx {





ScaledFontCairo::ScaledFontCairo(cairo_scaled_font_t* aScaledFont, Float aSize)
  : ScaledFontBase(aSize)
{
  mScaledFont = aScaledFont;
#if defined(USE_SKIA) && defined(MOZ_ENABLE_FREETYPE)
  cairo_font_face_t* fontFace = cairo_scaled_font_get_font_face(aScaledFont);
  FT_Face face = cairo_ft_scaled_font_lock_face(aScaledFont);

  int style = SkTypeface::kNormal;

  if (face->style_flags & FT_STYLE_FLAG_ITALIC)
    style |= SkTypeface::kItalic;

  if (face->style_flags & FT_STYLE_FLAG_BOLD)
    style |= SkTypeface::kBold;

  bool isFixedWidth = face->face_flags & FT_FACE_FLAG_FIXED_WIDTH;
  cairo_ft_scaled_font_unlock_face(aScaledFont);

  mTypeface = SkCreateTypefaceFromCairoFont(fontFace, (SkTypeface::Style)style, isFixedWidth);
#endif
}

}
}
