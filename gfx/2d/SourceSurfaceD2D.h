




































#ifndef MOZILLA_GFX_SOURCESURFACED2D_H_
#define MOZILLA_GFX_SOURCESURFACED2D_H_

#include "2D.h"
#include "HelpersD2D.h"
#include <vector>

namespace mozilla {
namespace gfx {

class SourceSurfaceD2D : public SourceSurface
{
public:
  SourceSurfaceD2D();
  ~SourceSurfaceD2D();

  virtual SurfaceType GetType() const { return SURFACE_D2D1_BITMAP; }
  virtual IntSize GetSize() const;
  virtual SurfaceFormat GetFormat() const;
  virtual TemporaryRef<DataSourceSurface> GetDataSurface();

  ID2D1Bitmap *GetBitmap() { return mBitmap; }

  bool InitFromData(unsigned char *aData,
                    const IntSize &aSize,
                    int32_t aStride,
                    SurfaceFormat aFormat,
                    ID2D1RenderTarget *aRT);
  bool InitFromTexture(ID3D10Texture2D *aTexture,
                       SurfaceFormat aFormat,
                       ID2D1RenderTarget *aRT);

private:
  friend class DrawTargetD2D;

  RefPtr<ID2D1Bitmap> mBitmap;
  std::vector<unsigned char> mRawData;
  SurfaceFormat mFormat;
  IntSize mSize;
};

}
}

#endif 
