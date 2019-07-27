




#ifndef _MOZILLA_GFX_SOURCESURFACECG_H
#define _MOZILLA_GFX_SOURCESURFACECG_H

#include <ApplicationServices/ApplicationServices.h>

#include "2D.h"

class MacIOSurface;

namespace mozilla {
namespace gfx {

CGImageRef
CreateCGImage(void *aInfo,
              const void *aData,
              const IntSize &aSize,
              int32_t aStride,
              SurfaceFormat aFormat);

class DrawTargetCG;

class SourceSurfaceCG : public SourceSurface
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(SourceSurfaceCG)
  SourceSurfaceCG() {}
  explicit SourceSurfaceCG(CGImageRef aImage) : mImage(aImage) {}
  ~SourceSurfaceCG();

  virtual SurfaceType GetType() const { return SurfaceType::COREGRAPHICS_IMAGE; }
  virtual IntSize GetSize() const;
  virtual SurfaceFormat GetFormat() const;
  virtual TemporaryRef<DataSourceSurface> GetDataSurface();

  CGImageRef GetImage() { return mImage; }

  bool InitFromData(unsigned char *aData,
                    const IntSize &aSize,
                    int32_t aStride,
                    SurfaceFormat aFormat);

private:
  CGImageRef mImage;

  


  SurfaceFormat mFormat;
};

class DataSourceSurfaceCG : public DataSourceSurface
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(DataSourceSurfaceCG)
  DataSourceSurfaceCG() {}
  explicit DataSourceSurfaceCG(CGImageRef aImage);
  ~DataSourceSurfaceCG();

  virtual SurfaceType GetType() const { return SurfaceType::DATA; }
  virtual IntSize GetSize() const;
  virtual SurfaceFormat GetFormat() const { return mFormat; }

  CGImageRef GetImage() { return mImage; }

  bool InitFromData(unsigned char *aData,
                    const IntSize &aSize,
                    int32_t aStride,
                    SurfaceFormat aFormat);

  virtual unsigned char *GetData();

  virtual int32_t Stride() { return CGImageGetBytesPerRow(mImage); }


private:
  CGContextRef mCg;
  CGImageRef mImage;
  SurfaceFormat mFormat;
  
  void *mData;
  


};

class SourceSurfaceCGContext : public DataSourceSurface
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(DataSourceSurfaceCGContext)
  virtual void DrawTargetWillChange() = 0;
  virtual CGImageRef GetImage() = 0;
};

class SourceSurfaceCGBitmapContext : public SourceSurfaceCGContext
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(DataSourceSurfaceCGBitmapContext)
  explicit SourceSurfaceCGBitmapContext(DrawTargetCG *);
  ~SourceSurfaceCGBitmapContext();

  virtual SurfaceType GetType() const { return SurfaceType::COREGRAPHICS_CGCONTEXT; }
  virtual IntSize GetSize() const;
  virtual SurfaceFormat GetFormat() const { return mFormat; }
  virtual TemporaryRef<DataSourceSurface> GetDataSurface()
  {
    
    
    
    
    
    
    
    
    
    DrawTargetWillChange();
    return this;
  }

  CGImageRef GetImage() { EnsureImage(); return mImage; }

  virtual unsigned char *GetData() { return static_cast<unsigned char*>(mData); }

  virtual int32_t Stride() { return mStride; }

private:
  
  friend class DrawTargetCG;
  virtual void DrawTargetWillChange();
  void EnsureImage() const;

  
  
  DrawTargetCG *mDrawTarget;
  CGContextRef mCg;
  SurfaceFormat mFormat;

  mutable CGImageRef mImage;

  
  
  void *mData;

  
  AlignedArray<uint8_t> mDataHolder;

  int32_t mStride;
  IntSize mSize;
};

class SourceSurfaceCGIOSurfaceContext : public SourceSurfaceCGContext
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(DataSourceSurfaceCGIOSurfaceContext)
  explicit SourceSurfaceCGIOSurfaceContext(DrawTargetCG *);
  ~SourceSurfaceCGIOSurfaceContext();

  virtual SurfaceType GetType() const { return SurfaceType::COREGRAPHICS_CGCONTEXT; }
  virtual IntSize GetSize() const;
  virtual SurfaceFormat GetFormat() const { return mFormat; }

  CGImageRef GetImage() { EnsureImage(); return mImage; }

  virtual unsigned char *GetData();

  virtual int32_t Stride() { return mStride; }

private:
  
  friend class DrawTargetCG;
  virtual void DrawTargetWillChange();
  void EnsureImage() const;

  SurfaceFormat mFormat;
  mutable CGImageRef mImage;
  MacIOSurface* mIOSurface;

  void *mData;
  int32_t mStride;

  IntSize mSize;
};


}
}

#endif 
