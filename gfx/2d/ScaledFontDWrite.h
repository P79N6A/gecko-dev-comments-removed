




































#ifndef MOZILLA_GFX_SCALEDFONTDWRITE_H_
#define MOZILLA_GFX_SCALEDFONTDWRITE_H_

#include "2D.h"
#include <dwrite.h>

namespace mozilla {
namespace gfx {

class ScaledFontDWrite : public ScaledFont
{
public:
  ScaledFontDWrite(IDWriteFontFace *aFont, Float aSize)
    : mFontFace(aFont)
    , mSize(aSize)
  {}

  virtual FontType GetType() const { return FONT_DWRITE; }

  virtual TemporaryRef<Path> GetPathForGlyphs(const GlyphBuffer &aBuffer, const DrawTarget *aTarget);

private:
  friend class DrawTargetD2D;

  RefPtr<IDWriteFontFace> mFontFace;
  Float mSize;
};

}
}

#endif 
