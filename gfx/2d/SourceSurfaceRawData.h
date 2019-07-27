




#ifndef MOZILLA_GFX_SOURCESURFACERAWDATA_H_
#define MOZILLA_GFX_SOURCESURFACERAWDATA_H_

#include "2D.h"
#include "Tools.h"

namespace mozilla {
namespace gfx {

class SourceSurfaceRawData : public DataSourceSurface
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(DataSourceSurfaceRawData)
  SourceSurfaceRawData() {}
  ~SourceSurfaceRawData() { if(mOwnData) delete [] mRawData; }

  virtual uint8_t *GetData() { return mRawData; }
  virtual int32_t Stride() { return mStride; }

  virtual SurfaceType GetType() const { return SurfaceType::DATA; }
  virtual IntSize GetSize() const { return mSize; }
  virtual SurfaceFormat GetFormat() const { return mFormat; }

  bool InitWrappingData(unsigned char *aData,
                        const IntSize &aSize,
                        int32_t aStride,
                        SurfaceFormat aFormat,
                        bool aOwnData);

  virtual void GuaranteePersistance();

private:
  uint8_t *mRawData;
  int32_t mStride;
  SurfaceFormat mFormat;
  IntSize mSize;
  bool mOwnData;
};

class SourceSurfaceAlignedRawData : public DataSourceSurface
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(DataSourceSurfaceAlignedRawData)
  SourceSurfaceAlignedRawData() {}

  virtual uint8_t *GetData() { return mArray; }
  virtual int32_t Stride() { return mStride; }

  virtual SurfaceType GetType() const { return SurfaceType::DATA; }
  virtual IntSize GetSize() const { return mSize; }
  virtual SurfaceFormat GetFormat() const { return mFormat; }

  bool Init(const IntSize &aSize,
            SurfaceFormat aFormat,
            bool aZero);
  bool InitWithStride(const IntSize &aSize,
                      SurfaceFormat aFormat,
                      int32_t aStride,
                      bool aZero);

private:
  AlignedArray<uint8_t> mArray;
  int32_t mStride;
  SurfaceFormat mFormat;
  IntSize mSize;
};

}
}

#endif 
