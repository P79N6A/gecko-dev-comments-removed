




#ifndef MOZILLA_GFX_SOURCESURFACERAWDATA_H_
#define MOZILLA_GFX_SOURCESURFACERAWDATA_H_

#include "2D.h"
#include "Tools.h"
#include "mozilla/Atomics.h"

namespace mozilla {
namespace gfx {

class SourceSurfaceRawData : public DataSourceSurface
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(DataSourceSurfaceRawData, override)
  SourceSurfaceRawData()
    : mMapCount(0)
  {}
  ~SourceSurfaceRawData()
  {
    if(mOwnData) delete [] mRawData;
    MOZ_ASSERT(mMapCount == 0);
  }

  virtual uint8_t *GetData() override { return mRawData; }
  virtual int32_t Stride() override { return mStride; }

  virtual SurfaceType GetType() const override { return SurfaceType::DATA; }
  virtual IntSize GetSize() const override { return mSize; }
  virtual SurfaceFormat GetFormat() const override { return mFormat; }

  bool InitWrappingData(unsigned char *aData,
                        const IntSize &aSize,
                        int32_t aStride,
                        SurfaceFormat aFormat,
                        bool aOwnData);

  virtual void GuaranteePersistance() override;

  
  
  
  
  
  
  
  
  
  virtual bool Map(MapType, MappedSurface *aMappedSurface) override
  {
    aMappedSurface->mData = GetData();
    aMappedSurface->mStride = Stride();
    bool success = !!aMappedSurface->mData;
    if (success) {
      mMapCount++;
    }
    return success;
  }

  virtual void Unmap() override
  {
    mMapCount--;
    MOZ_ASSERT(mMapCount >= 0);
  }

private:
  uint8_t *mRawData;
  int32_t mStride;
  SurfaceFormat mFormat;
  IntSize mSize;
  Atomic<int32_t> mMapCount;
  bool mOwnData;
};

class SourceSurfaceAlignedRawData : public DataSourceSurface
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(DataSourceSurfaceAlignedRawData, override)
  SourceSurfaceAlignedRawData()
    : mMapCount(0)
  {}
  ~SourceSurfaceAlignedRawData()
  {
    MOZ_ASSERT(mMapCount == 0);
  }

  virtual uint8_t *GetData() override { return mArray; }
  virtual int32_t Stride() override { return mStride; }

  virtual SurfaceType GetType() const override { return SurfaceType::DATA; }
  virtual IntSize GetSize() const override { return mSize; }
  virtual SurfaceFormat GetFormat() const override { return mFormat; }

  bool Init(const IntSize &aSize,
            SurfaceFormat aFormat,
            bool aZero);
  bool InitWithStride(const IntSize &aSize,
                      SurfaceFormat aFormat,
                      int32_t aStride,
                      bool aZero);

  virtual bool Map(MapType, MappedSurface *aMappedSurface) override
  {
    aMappedSurface->mData = GetData();
    aMappedSurface->mStride = Stride();
    bool success = !!aMappedSurface->mData;
    if (success) {
      mMapCount++;
    }
    return success;
  }

  virtual void Unmap() override
  {
    mMapCount--;
    MOZ_ASSERT(mMapCount >= 0);
  }

private:
  AlignedArray<uint8_t> mArray;
  int32_t mStride;
  SurfaceFormat mFormat;
  IntSize mSize;
  Atomic<int32_t> mMapCount;
};

} 
} 

#endif 
