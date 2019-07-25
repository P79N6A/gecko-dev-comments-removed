




#ifndef MOZILLA_GFX_SOURCESURFACERAWDATA_H_
#define MOZILLA_GFX_SOURCESURFACERAWDATA_H_

#include "2D.h"

namespace mozilla {
namespace gfx {

class SourceSurfaceRawData : public DataSourceSurface
{
public:
  SourceSurfaceRawData() {}
  ~SourceSurfaceRawData() { if(mOwnData) delete [] mRawData; }

  virtual uint8_t *GetData() { return mRawData; }
  virtual int32_t Stride() { return mStride; }

  virtual SurfaceType GetType() const { return SURFACE_DATA; }
  virtual IntSize GetSize() const { return mSize; }
  virtual SurfaceFormat GetFormat() const { return mFormat; }

  bool InitWrappingData(unsigned char *aData,
                        const IntSize &aSize,
                        int32_t aStride,
                        SurfaceFormat aFormat,
                        bool aOwnData);

private:
  uint8_t *mRawData;
  int32_t mStride;
  SurfaceFormat mFormat;
  IntSize mSize;
  bool mOwnData;
};

}
}

#endif 
