




#ifndef MOZILLA_GFX_SCALEDFONTDWRITE_H_
#define MOZILLA_GFX_SCALEDFONTDWRITE_H_

#include <dwrite.h>
#include "ScaledFontBase.h"

struct ID2D1GeometrySink;

namespace mozilla {
namespace gfx {

class ScaledFontDWrite final : public ScaledFontBase
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(ScaledFontDwrite)
  ScaledFontDWrite(IDWriteFontFace *aFont, Float aSize)
    : mFontFace(aFont)
    , ScaledFontBase(aSize)
  {}
  ScaledFontDWrite(uint8_t *aData, uint32_t aSize, uint32_t aIndex, Float aGlyphSize);

  virtual FontType GetType() const { return FontType::DWRITE; }

  virtual already_AddRefed<Path> GetPathForGlyphs(const GlyphBuffer &aBuffer, const DrawTarget *aTarget);
  virtual void CopyGlyphsToBuilder(const GlyphBuffer &aBuffer, PathBuilder *aBuilder, BackendType aBackendType, const Matrix *aTransformHint);

  void CopyGlyphsToSink(const GlyphBuffer &aBuffer, ID2D1GeometrySink *aSink);

  virtual bool GetFontFileData(FontFileDataOutput aDataCallback, void *aBaton);

  virtual AntialiasMode GetDefaultAAMode();

#ifdef USE_SKIA
  virtual SkTypeface* GetSkTypeface()
  {
    MOZ_ASSERT(false, "Skia and DirectWrite do not mix");
    return nullptr;
  }
#endif

  RefPtr<IDWriteFontFace> mFontFace;
};

class GlyphRenderingOptionsDWrite : public GlyphRenderingOptions
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(GlyphRenderingOptionsDWrite)
  GlyphRenderingOptionsDWrite(IDWriteRenderingParams *aParams)
    : mParams(aParams)
  {
  }

  virtual FontType GetType() const { return FontType::DWRITE; }

private:
  friend class DrawTargetD2D;
  friend class DrawTargetD2D1;

  RefPtr<IDWriteRenderingParams> mParams;
};

}
}

#endif 
