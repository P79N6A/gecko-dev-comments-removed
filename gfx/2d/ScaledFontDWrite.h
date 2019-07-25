




































#ifndef MOZILLA_GFX_SCALEDFONTDWRITE_H_
#define MOZILLA_GFX_SCALEDFONTDWRITE_H_

#include "2D.h"
#include <dwrite.h>

struct ID2D1GeometrySink;

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
  virtual void CopyGlyphsToBuilder(const GlyphBuffer &aBuffer, PathBuilder *aBuilder);

private:
  friend class DrawTargetD2D;

  void CopyGlyphsToSink(const GlyphBuffer &aBuffer, ID2D1GeometrySink *aSink);

  RefPtr<IDWriteFontFace> mFontFace;
  Float mSize;
};

class GlyphRenderingOptionsDWrite : public GlyphRenderingOptions
{
public:
  GlyphRenderingOptionsDWrite(IDWriteRenderingParams *aParams)
    : mParams(aParams)
  {
  }

  virtual FontType GetType() const { return FONT_DWRITE; }

private:
  friend class DrawTargetD2D;

  RefPtr<IDWriteRenderingParams> mParams;
};

}
}

#endif 
