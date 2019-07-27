




#ifndef MOZILLA_GFX_TEXTURED3D9_H
#define MOZILLA_GFX_TEXTURED3D9_H

#include "mozilla/layers/Compositor.h"
#include "mozilla/layers/TextureClient.h"
#include "mozilla/layers/TextureHost.h"
#include "mozilla/GfxMessageUtils.h"
#include "gfxWindowsPlatform.h"
#include "d3d9.h"
#include <vector>
#include "DeviceManagerD3D9.h"

namespace mozilla {
namespace gfxs {
class DrawTarget;
}
}

namespace mozilla {
namespace layers {

class CompositorD3D9;

class TextureSourceD3D9
{
  friend class DeviceManagerD3D9;

public:
  TextureSourceD3D9()
    : mPreviousHost(nullptr)
    , mNextHost(nullptr)
    , mCreatingDeviceManager(nullptr)
  {}
  virtual ~TextureSourceD3D9();

  virtual IDirect3DTexture9* GetD3D9Texture() { return mTexture; }

  StereoMode GetStereoMode() const { return mStereoMode; };

  
  virtual void ReleaseTextureResources()
  {
    mTexture = nullptr;
  }

protected:
  virtual gfx::IntSize GetSize() const { return mSize; }
  void SetSize(const gfx::IntSize& aSize) { mSize = aSize; }

  
  TemporaryRef<IDirect3DTexture9> InitTextures(
    DeviceManagerD3D9* aDeviceManager,
    const gfx::IntSize &aSize,
    _D3DFORMAT aFormat,
    RefPtr<IDirect3DSurface9>& aSurface,
    D3DLOCKED_RECT& aLockedRect);

  TemporaryRef<IDirect3DTexture9> DataToTexture(
    DeviceManagerD3D9* aDeviceManager,
    unsigned char *aData,
    int aStride,
    const gfx::IntSize &aSize,
    _D3DFORMAT aFormat,
    uint32_t aBPP);

  
  
  TemporaryRef<IDirect3DTexture9> TextureToTexture(
    DeviceManagerD3D9* aDeviceManager,
    IDirect3DTexture9* aTexture,
    const gfx::IntSize& aSize,
    _D3DFORMAT aFormat);

  TemporaryRef<IDirect3DTexture9> SurfaceToTexture(
    DeviceManagerD3D9* aDeviceManager,
    gfxWindowsSurface* aSurface,
    const gfx::IntSize& aSize,
    _D3DFORMAT aFormat);

  gfx::IntSize mSize;

  
  TextureSourceD3D9* mPreviousHost;
  TextureSourceD3D9* mNextHost;
  
  DeviceManagerD3D9* mCreatingDeviceManager;

  StereoMode mStereoMode;
  RefPtr<IDirect3DTexture9> mTexture;
};






class DataTextureSourceD3D9 : public DataTextureSource
                            , public TextureSourceD3D9
                            , public BigImageIterator
{
public:
  DataTextureSourceD3D9(gfx::SurfaceFormat aFormat,
                        CompositorD3D9* aCompositor,
                        TextureFlags aFlags = TextureFlags::DEFAULT,
                        StereoMode aStereoMode = StereoMode::MONO);

  DataTextureSourceD3D9(gfx::SurfaceFormat aFormat,
                        gfx::IntSize aSize,
                        CompositorD3D9* aCompositor,
                        IDirect3DTexture9* aTexture,
                        TextureFlags aFlags = TextureFlags::DEFAULT);

  virtual ~DataTextureSourceD3D9();

  

  virtual bool Update(gfx::DataSourceSurface* aSurface,
                      nsIntRegion* aDestRegion = nullptr,
                      gfx::IntPoint* aSrcOffset = nullptr) MOZ_OVERRIDE;

  

  virtual TextureSourceD3D9* AsSourceD3D9() MOZ_OVERRIDE { return this; }

  virtual IDirect3DTexture9* GetD3D9Texture() MOZ_OVERRIDE;

  virtual DataTextureSource* AsDataTextureSource() MOZ_OVERRIDE { return this; }

  virtual void DeallocateDeviceData() MOZ_OVERRIDE { mTexture = nullptr; }

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE { return mFormat; }

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  

  virtual BigImageIterator* AsBigImageIterator() MOZ_OVERRIDE { return mIsTiled ? this : nullptr; }

  virtual size_t GetTileCount() MOZ_OVERRIDE { return mTileTextures.size(); }

  virtual bool NextTile() MOZ_OVERRIDE { return (++mCurrentTile < mTileTextures.size()); }

  virtual nsIntRect GetTileRect() MOZ_OVERRIDE;

  virtual void EndBigImageIteration() MOZ_OVERRIDE { mIterating = false; }

  virtual void BeginBigImageIteration() MOZ_OVERRIDE
  {
    mIterating = true;
    mCurrentTile = 0;
  }

  


  bool UpdateFromTexture(IDirect3DTexture9* aTexture, const nsIntRegion* aRegion);

  

  bool Update(gfxWindowsSurface* aSurface);

protected:
  gfx::IntRect GetTileRect(uint32_t aTileIndex) const;

  void Reset();

  std::vector< RefPtr<IDirect3DTexture9> > mTileTextures;
  RefPtr<CompositorD3D9> mCompositor;
  gfx::SurfaceFormat mFormat;
  uint32_t mCurrentTile;
  TextureFlags mFlags;
  bool mIsTiled;
  bool mIterating;
};





class CairoTextureClientD3D9 : public TextureClient
{
public:
  CairoTextureClientD3D9(ISurfaceAllocator* aAllocator, gfx::SurfaceFormat aFormat,
                         TextureFlags aFlags);

  virtual ~CairoTextureClientD3D9();

  

  virtual bool IsAllocated() const MOZ_OVERRIDE { return !!mTexture; }

  virtual bool Lock(OpenMode aOpenMode) MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual bool IsLocked() const MOZ_OVERRIDE { return mIsLocked; }

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) MOZ_OVERRIDE;

  virtual gfx::IntSize GetSize() const { return mSize; }

  virtual gfx::SurfaceFormat GetFormat() const { return mFormat; }

  virtual bool CanExposeDrawTarget() const MOZ_OVERRIDE { return true; }

  virtual gfx::DrawTarget* BorrowDrawTarget() MOZ_OVERRIDE;

  virtual bool AllocateForSurface(gfx::IntSize aSize,
                                  TextureAllocationFlags aFlags = ALLOC_DEFAULT) MOZ_OVERRIDE;

  virtual bool HasInternalBuffer() const MOZ_OVERRIDE { return true; }

  virtual TemporaryRef<TextureClient>
  CreateSimilar(TextureFlags aFlags = TextureFlags::DEFAULT,
                TextureAllocationFlags aAllocFlags = ALLOC_DEFAULT) const MOZ_OVERRIDE;

private:
  RefPtr<IDirect3DTexture9> mTexture;
  nsRefPtr<IDirect3DSurface9> mD3D9Surface;
  RefPtr<gfx::DrawTarget> mDrawTarget;
  nsRefPtr<gfxASurface> mSurface;
  gfx::IntSize mSize;
  gfx::SurfaceFormat mFormat;
  bool mIsLocked;
  bool mNeedsClear;
  bool mNeedsClearWhite;
  bool mLockRect;
};






class SharedTextureClientD3D9 : public TextureClient
{
public:
  SharedTextureClientD3D9(ISurfaceAllocator* aAllocator,
                          gfx::SurfaceFormat aFormat,
                          TextureFlags aFlags);

  virtual ~SharedTextureClientD3D9();

  

  virtual bool IsAllocated() const MOZ_OVERRIDE { return !!mTexture; }

  virtual bool Lock(OpenMode aOpenMode) MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual bool IsLocked() const MOZ_OVERRIDE { return mIsLocked; }

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) MOZ_OVERRIDE;

  void InitWith(IDirect3DTexture9* aTexture, HANDLE aSharedHandle, D3DSURFACE_DESC aDesc)
  {
    MOZ_ASSERT(!mTexture);
    mTexture = aTexture;
    mHandle = aSharedHandle;
    mDesc = aDesc;
    if (mTexture) {
      gfxWindowsPlatform::sD3D9SharedTextureUsed += mDesc.Width * mDesc.Height * 4;
    }
  }

  virtual gfx::IntSize GetSize() const
  {
    return gfx::IntSize(mDesc.Width, mDesc.Height);
  }

  virtual bool HasInternalBuffer() const MOZ_OVERRIDE { return true; }

  
  
  
  virtual TemporaryRef<TextureClient>
  CreateSimilar(TextureFlags, TextureAllocationFlags) const MOZ_OVERRIDE { return nullptr; }

private:
  RefPtr<IDirect3DTexture9> mTexture;
  gfx::SurfaceFormat mFormat;
  HANDLE mHandle;
  D3DSURFACE_DESC mDesc;
  bool mIsLocked;
};

class TextureHostD3D9 : public TextureHost
{
public:
  TextureHostD3D9(TextureFlags aFlags,
                  const SurfaceDescriptorD3D9& aDescriptor);

  virtual TextureSource* GetTextureSources() MOZ_OVERRIDE;

  virtual void DeallocateDeviceData() MOZ_OVERRIDE;

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE { return mFormat; }

  virtual bool Lock() MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual void Updated(const nsIntRegion* aRegion) MOZ_OVERRIDE;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() MOZ_OVERRIDE
  {
    return nullptr;
  }

  virtual bool HasInternalBuffer() const MOZ_OVERRIDE { return true; }

protected:
  TextureHostD3D9(TextureFlags aFlags);
  IDirect3DDevice9* GetDevice();

  RefPtr<DataTextureSourceD3D9> mTextureSource;
  RefPtr<IDirect3DTexture9> mTexture;
  RefPtr<CompositorD3D9> mCompositor;
  gfx::IntSize mSize;
  gfx::SurfaceFormat mFormat;
  bool mIsLocked;
};

class DXGITextureHostD3D9 : public TextureHost
{
public:
  DXGITextureHostD3D9(TextureFlags aFlags,
    const SurfaceDescriptorD3D10& aDescriptor);

  virtual TextureSource* GetTextureSources() MOZ_OVERRIDE;

  virtual void DeallocateDeviceData() MOZ_OVERRIDE;

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE { return mFormat; }

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  virtual bool Lock() MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() MOZ_OVERRIDE
  {
    return nullptr; 
  }

protected:
  void OpenSharedHandle();
  IDirect3DDevice9* GetDevice();

  RefPtr<DataTextureSourceD3D9> mTextureSource;
  RefPtr<CompositorD3D9> mCompositor;
  WindowsHandle mHandle;
  gfx::SurfaceFormat mFormat;
  gfx::IntSize mSize;
  bool mIsLocked;
};

class CompositingRenderTargetD3D9 : public CompositingRenderTarget,
                                    public TextureSourceD3D9
{
public:
  CompositingRenderTargetD3D9(IDirect3DTexture9* aTexture,
                              SurfaceInitMode aInit,
                              const gfx::IntRect& aRect);
  
  CompositingRenderTargetD3D9(IDirect3DSurface9* aSurface,
                              SurfaceInitMode aInit,
                              const gfx::IntRect& aRect);
  virtual ~CompositingRenderTargetD3D9();

  virtual TextureSourceD3D9* AsSourceD3D9() MOZ_OVERRIDE
  {
    MOZ_ASSERT(mTexture,
               "No texture, can't be indirectly rendered. Is this the screen backbuffer?");
    return this;
  }

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE;

  void BindRenderTarget(IDirect3DDevice9* aDevice);

  IDirect3DSurface9* GetD3D9Surface() const { return mSurface; }

private:
  friend class CompositorD3D9;

  nsRefPtr<IDirect3DSurface9> mSurface;
  SurfaceInitMode mInitMode;
  bool mInitialized;
};

}
}

#endif 
