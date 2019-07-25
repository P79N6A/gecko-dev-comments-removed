




































#include "ScaledFontDWrite.h"
#include "PathD2D.h"

#include <vector>

namespace mozilla {
namespace gfx {

TemporaryRef<Path>
ScaledFontDWrite::GetPathForGlyphs(const GlyphBuffer &aBuffer, const DrawTarget *aTarget)
{
  if (aTarget->GetType() != BACKEND_DIRECT2D) {
    
    return NULL;
  }

  RefPtr<PathBuilder> pathBuilder = aTarget->CreatePathBuilder();

  PathBuilderD2D *pathBuilderD2D =
    static_cast<PathBuilderD2D*>(pathBuilder.get());

  std::vector<UINT16> indices;
  std::vector<FLOAT> advances;
  std::vector<DWRITE_GLYPH_OFFSET> offsets;
  indices.resize(aBuffer.mNumGlyphs);
  advances.resize(aBuffer.mNumGlyphs);
  offsets.resize(aBuffer.mNumGlyphs);

  memset(&advances.front(), 0, sizeof(FLOAT) * aBuffer.mNumGlyphs);
  for (unsigned int i = 0; i < aBuffer.mNumGlyphs; i++) {
    indices[i] = aBuffer.mGlyphs[i].mIndex;
    offsets[i].advanceOffset = aBuffer.mGlyphs[i].mPosition.x;
    offsets[i].ascenderOffset = -aBuffer.mGlyphs[i].mPosition.y;
  }

  mFontFace->GetGlyphRunOutline(mSize, &indices.front(), &advances.front(),
                                &offsets.front(), aBuffer.mNumGlyphs,
                                FALSE, FALSE, pathBuilderD2D->GetSink());

  return pathBuilder->Finish();
}

}
}
