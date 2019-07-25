




































#ifndef MOZILLA_GFX_SOURCESURFACED2DTARGET_H_
#define MOZILLA_GFX_SOURCESURFACED2DTARGET_H_

#include "2D.h"
#include "HelpersD2D.h"
#include <vector>
#include <d3d10_1.h>

namespace mozilla {
namespace gfx {

class DrawTargetD2D;

class SourceSurfaceD2DTarget : public SourceSurface
{
public:
  SourceSurfaceD2DTarget();
  ~SourceSurfaceD2DTarget();

  virtual SurfaceType GetType() const { return SURFACE_D2D1_DRAWTARGET; }
  virtual IntSize GetSize() const;
  virtual SurfaceFormat GetFormat() const;
  virtual TemporaryRef<DataSourceSurface> GetDataSurface();

private:
  friend class DrawTargetD2D;
  ID3D10ShaderResourceView *GetSRView();

  
  
  bool IsCopy() { return mIsCopy; }

  
  
  
  void DrawTargetWillChange();

  
  
  void MarkIndependent();

  ID2D1Bitmap *GetBitmap(ID2D1RenderTarget *aRT);

  RefPtr<ID3D10ShaderResourceView> mSRView;
  RefPtr<ID2D1Bitmap> mBitmap;
  RefPtr<DrawTargetD2D> mDrawTarget;
  mutable RefPtr<ID3D10Texture2D> mTexture;
  SurfaceFormat mFormat;

  
  
  std::vector<RefPtr<DrawTargetD2D>> mDependentSurfaces;

  bool mIsCopy;
};

class DataSourceSurfaceD2DTarget : public DataSourceSurface
{
public:
  DataSourceSurfaceD2DTarget();
  ~DataSourceSurfaceD2DTarget();

  virtual SurfaceType GetType() const { return SURFACE_DATA; }
  virtual IntSize GetSize() const;
  virtual SurfaceFormat GetFormat() const;
  virtual unsigned char *GetData();
  virtual int32_t Stride();

private:
  friend class SourceSurfaceD2DTarget;
  void EnsureMapped();

  mutable RefPtr<ID3D10Texture2D> mTexture;
  SurfaceFormat mFormat;
  D3D10_MAPPED_TEXTURE2D mMap;
  bool mMapped;
};

}
}

#endif 
