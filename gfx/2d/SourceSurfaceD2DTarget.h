




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
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(SourceSurfaceD2DTarget)
  SourceSurfaceD2DTarget(DrawTargetD2D* aDrawTarget, ID3D10Texture2D* aTexture,
                         SurfaceFormat aFormat);
  ~SourceSurfaceD2DTarget();

  virtual SurfaceType GetType() const { return SurfaceType::D2D1_DRAWTARGET; }
  virtual IntSize GetSize() const;
  virtual SurfaceFormat GetFormat() const;
  virtual already_AddRefed<DataSourceSurface> GetDataSurface();
  virtual void *GetNativeSurface(NativeSurfaceType aType);

  DrawTargetD2D* GetDT() { return mDrawTarget; }
  ID2D1Bitmap *GetBitmap(ID2D1RenderTarget *aRT);

private:
  friend class DrawTargetD2D;

  ID3D10ShaderResourceView *GetSRView();

  
  
  
  void DrawTargetWillChange();

  
  
  void MarkIndependent();

  RefPtr<ID3D10ShaderResourceView> mSRView;
  RefPtr<ID2D1Bitmap> mBitmap;
  
  
  
  
  DrawTargetD2D* mDrawTarget;
  mutable RefPtr<ID3D10Texture2D> mTexture;
  SurfaceFormat mFormat;
  bool mOwnsCopy;
};

class DataSourceSurfaceD2DTarget : public DataSourceSurface
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(DataSourceSurfaceD2DTarget)
  DataSourceSurfaceD2DTarget(SurfaceFormat aFormat);
  ~DataSourceSurfaceD2DTarget();

  virtual SurfaceType GetType() const { return SurfaceType::DATA; }
  virtual IntSize GetSize() const;
  virtual SurfaceFormat GetFormat() const;
  virtual uint8_t *GetData();
  virtual int32_t Stride();
  virtual bool Map(MapType, MappedSurface *aMappedSurface);
  virtual void Unmap();

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
