




#ifndef _MOZILLA_GFX_OP_SOURCESURFACE_CAIRO_H
#define _MOZILLA_GFX_OP_SOURCESURFACE_CAIRO_H

#include "2D.h"

namespace mozilla {
namespace gfx {

class DrawTargetCairo;

class SourceSurfaceCairo : public SourceSurface
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(SourceSurfaceCairo)
  
  
  
  
  
  SourceSurfaceCairo(cairo_surface_t* aSurface, const IntSize& aSize,
                     const SurfaceFormat& aFormat,
                     DrawTargetCairo* aDrawTarget = nullptr);
  virtual ~SourceSurfaceCairo();

  virtual SurfaceType GetType() const { return SurfaceType::CAIRO; }
  virtual IntSize GetSize() const;
  virtual SurfaceFormat GetFormat() const;
  virtual TemporaryRef<DataSourceSurface> GetDataSurface();

  cairo_surface_t* GetSurface() const;

private: 
  friend class DrawTargetCairo;
  void DrawTargetWillChange();

private: 
  IntSize mSize;
  SurfaceFormat mFormat;
  cairo_surface_t* mSurface;
  DrawTargetCairo* mDrawTarget;
};

class DataSourceSurfaceCairo : public DataSourceSurface
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(DataSourceSurfaceCairo)
  explicit DataSourceSurfaceCairo(cairo_surface_t* imageSurf);
  virtual ~DataSourceSurfaceCairo();
  virtual unsigned char *GetData();
  virtual int32_t Stride();

  virtual SurfaceType GetType() const { return SurfaceType::CAIRO_IMAGE; }
  virtual IntSize GetSize() const;
  virtual SurfaceFormat GetFormat() const;

  cairo_surface_t* GetSurface() const;

private:
  cairo_surface_t* mImageSurface;
};

}
}

#endif 
