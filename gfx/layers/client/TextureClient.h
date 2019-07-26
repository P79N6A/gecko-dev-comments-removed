




#ifndef MOZILLA_GFX_TEXTURECLIENT_H
#define MOZILLA_GFX_TEXTURECLIENT_H

#include "mozilla/layers/LayersSurfaces.h"
#include "gfxASurface.h"
#include "mozilla/layers/CompositorTypes.h" 
#include "mozilla/RefPtr.h"
#include "ImageContainer.h" 

class gfxReusableSurfaceWrapper;

namespace mozilla {

namespace gl {
class GLContext;
}

namespace layers {

class ContentClient;
class PlanarYCbCrImage;
class Image;
class CompositableForwarder;
class ISurfaceAllocator;
class CompositableClient;












class TextureClientSurface
{
public:
  virtual bool UpdateSurface(gfxASurface* aSurface) = 0;
  virtual bool AllocateForSurface(gfx::IntSize aSize) = 0;
};




class TextureClientYCbCr
{
public:
  virtual bool UpdateYCbCr(const PlanarYCbCrImage::Data& aData) = 0;
  virtual bool AllocateForYCbCr(gfx::IntSize aYSize,
                                gfx::IntSize aCbCrSize,
                                StereoMode aStereoMode) = 0;
};

























class TextureClient : public RefCounted<TextureClient>
{
public:
  TextureClient(TextureFlags aFlags = TEXTURE_FLAGS_DEFAULT);
  virtual ~TextureClient();

  virtual TextureClientSurface* AsTextureClientSurface() { return nullptr; }
  virtual TextureClientYCbCr* AsTextureClientYCbCr() { return nullptr; }

  virtual void MarkUnused() {}

  virtual bool Lock(OpenMode aMode)
  {
    return true;
  }

  virtual void Unlock() {}

  void SetID(uint64_t aID)
  {
    MOZ_ASSERT(mID == 0 || aID == 0);
    mID = aID;
  }

  uint64_t GetID() const
  {
    return mID;
  }

  void SetNextSibling(TextureClient* aNext)
  {
    mNextSibling = aNext;
  }

  TextureClient* GetNextSibling()
  {
    return mNextSibling;
  }

  virtual bool IsAllocated() const = 0;

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aDescriptor) = 0;

  void SetFlags(TextureFlags aFlags)
  {
    MOZ_ASSERT(!IsSharedWithCompositor());
    mFlags = aFlags;
  }

  void AddFlags(TextureFlags  aFlags)
  {
    MOZ_ASSERT(!IsSharedWithCompositor());
    
    MOZ_ASSERT(!(aFlags & TEXTURE_DEALLOCATE_CLIENT && aFlags & TEXTURE_DEALLOCATE_HOST));
    if (aFlags & TEXTURE_DEALLOCATE_CLIENT) {
      mFlags &= ~TEXTURE_DEALLOCATE_HOST;
    } else if (aFlags & TEXTURE_DEALLOCATE_HOST) {
      mFlags &= ~TEXTURE_DEALLOCATE_CLIENT;
    }
    mFlags |= aFlags;
  }

  void RemoveFlags(TextureFlags  aFlags)
  {
    MOZ_ASSERT(!IsSharedWithCompositor());
    mFlags &= (~aFlags);
  }

  TextureFlags GetFlags() const { return mFlags; }

  




  bool IsImmutable() const { return mFlags & TEXTURE_IMMUTABLE; }

  void MarkImmutable() { AddFlags(TEXTURE_IMMUTABLE); }

  bool IsSharedWithCompositor() const { return GetID() != 0; }

  bool ShouldDeallocateInDestructor() const;
protected:
  uint64_t mID;
  RefPtr<TextureClient> mNextSibling;
  TextureFlags mFlags;
};






class BufferTextureClient : public TextureClient
                          , public TextureClientSurface
                          , TextureClientYCbCr
{
public:
  BufferTextureClient(CompositableClient* aCompositable, gfx::SurfaceFormat aFormat);

  virtual ~BufferTextureClient();

  virtual bool IsAllocated() const = 0;

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aDescriptor) = 0;

  virtual bool Allocate(uint32_t aSize) = 0;

  virtual uint8_t* GetBuffer() const = 0;

  virtual size_t GetBufferSize() const = 0;

  

  virtual TextureClientSurface* AsTextureClientSurface() MOZ_OVERRIDE { return this; }

  virtual bool UpdateSurface(gfxASurface* aSurface) MOZ_OVERRIDE;

  virtual bool AllocateForSurface(gfx::IntSize aSize) MOZ_OVERRIDE;

  

  virtual TextureClientYCbCr* AsTextureClientYCbCr() MOZ_OVERRIDE { return this; }

  virtual bool UpdateYCbCr(const PlanarYCbCrImage::Data& aData) MOZ_OVERRIDE;

  virtual bool AllocateForYCbCr(gfx::IntSize aYSize,
                                gfx::IntSize aCbCrSize,
                                StereoMode aStereoMode) MOZ_OVERRIDE;

  gfx::SurfaceFormat GetFormat() const { return mFormat; }

protected:
  CompositableClient* mCompositable;
  gfx::SurfaceFormat mFormat;
};





class ShmemTextureClient : public BufferTextureClient
{
public:
  ShmemTextureClient(CompositableClient* aCompositable, gfx::SurfaceFormat aFormat);

  ~ShmemTextureClient();

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aDescriptor) MOZ_OVERRIDE;

  virtual bool Allocate(uint32_t aSize) MOZ_OVERRIDE;

  virtual uint8_t* GetBuffer() const MOZ_OVERRIDE;

  virtual size_t GetBufferSize() const MOZ_OVERRIDE;

  virtual bool IsAllocated() const MOZ_OVERRIDE { return mAllocated; }

  ISurfaceAllocator* GetAllocator() const;

  ipc::Shmem& GetShmem() { return mShmem; }

protected:
  ipc::Shmem mShmem;
  ISurfaceAllocator* mAllocator;
  bool mAllocated;
};






class MemoryTextureClient : public BufferTextureClient
{
public:
  MemoryTextureClient(CompositableClient* aCompositable, gfx::SurfaceFormat aFormat);

  ~MemoryTextureClient();

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aDescriptor) MOZ_OVERRIDE;

  virtual bool Allocate(uint32_t aSize) MOZ_OVERRIDE;

  virtual uint8_t* GetBuffer() const MOZ_OVERRIDE { return mBuffer; }

  virtual size_t GetBufferSize() const MOZ_OVERRIDE { return mBufSize; }

  virtual bool IsAllocated() const MOZ_OVERRIDE { return mBuffer != nullptr; }

protected:
  uint8_t* mBuffer;
  size_t mBufSize;
};


struct TextureClientAutoUnlock
{
  TextureClient* mTexture;

  TextureClientAutoUnlock(TextureClient* aTexture)
  : mTexture(aTexture) {}

  ~TextureClientAutoUnlock()
  {
    mTexture->Unlock();
  }
};

























class DeprecatedTextureClient : public RefCounted<DeprecatedTextureClient>
{
public:
  typedef gl::SharedTextureHandle SharedTextureHandle;
  typedef gl::GLContext GLContext;
  typedef gl::TextureImage TextureImage;

  virtual ~DeprecatedTextureClient();

  






  virtual const TextureInfo& GetTextureInfo() const
  {
    return mTextureInfo;
  }

  virtual bool SupportsType(DeprecatedTextureClientType aType) { return false; }

  




  virtual gfxImageSurface* LockImageSurface() { return nullptr; }
  virtual gfxASurface* LockSurface() { return nullptr; }
  virtual gfx::DrawTarget* LockDrawTarget() { return nullptr; }

  virtual SurfaceDescriptor* LockSurfaceDescriptor() { return GetDescriptor(); }
  virtual void ReleaseResources() {}
  



  virtual void Unlock() {}

  



  virtual void EnsureAllocated(gfx::IntSize aSize,
                               gfxASurface::gfxContentType aType) = 0;

  



  virtual void SetDescriptorFromReply(const SurfaceDescriptor& aDescriptor)
  {
    
    SetDescriptor(aDescriptor);
  }
  virtual void SetDescriptor(const SurfaceDescriptor& aDescriptor)
  {
    mDescriptor = aDescriptor;
  }
  SurfaceDescriptor* GetDescriptor() { return &mDescriptor; }

  CompositableForwarder* GetForwarder() const
  {
    return mForwarder;
  }

  void SetFlags(TextureFlags aFlags)
  {
    mTextureInfo.mTextureFlags = aFlags;
  }

  enum AccessMode
  {
    ACCESS_NONE = 0x0,
    ACCESS_READ_ONLY  = 0x1,
    ACCESS_READ_WRITE = 0x2
  };

  void SetAccessMode(AccessMode aAccessMode)
  {
    mAccessMode = aAccessMode;
  }

  AccessMode GetAccessMode() const
  {
    return mAccessMode;
  }

  virtual gfxASurface::gfxContentType GetContentType() = 0;

protected:
  DeprecatedTextureClient(CompositableForwarder* aForwarder,
                const TextureInfo& aTextureInfo);

  CompositableForwarder* mForwarder;
  
  
  SurfaceDescriptor mDescriptor;
  TextureInfo mTextureInfo;
  AccessMode mAccessMode;
};

class DeprecatedTextureClientShmem : public DeprecatedTextureClient
{
public:
  DeprecatedTextureClientShmem(CompositableForwarder* aForwarder, const TextureInfo& aTextureInfo);
  ~DeprecatedTextureClientShmem() { ReleaseResources(); }

  virtual bool SupportsType(DeprecatedTextureClientType aType) MOZ_OVERRIDE
  {
    return aType == TEXTURE_SHMEM || aType == TEXTURE_CONTENT;
  }
  virtual gfxImageSurface* LockImageSurface() MOZ_OVERRIDE;
  virtual gfxASurface* LockSurface() MOZ_OVERRIDE { return GetSurface(); }
  virtual gfx::DrawTarget* LockDrawTarget();
  virtual void Unlock() MOZ_OVERRIDE;
  virtual void EnsureAllocated(gfx::IntSize aSize, gfxASurface::gfxContentType aType) MOZ_OVERRIDE;

  virtual void ReleaseResources() MOZ_OVERRIDE;
  virtual void SetDescriptor(const SurfaceDescriptor& aDescriptor) MOZ_OVERRIDE;
  virtual gfxASurface::gfxContentType GetContentType() MOZ_OVERRIDE { return mContentType; }
private:
  gfxASurface* GetSurface();

  nsRefPtr<gfxASurface> mSurface;
  nsRefPtr<gfxImageSurface> mSurfaceAsImage;
  RefPtr<gfx::DrawTarget> mDrawTarget;

  gfxASurface::gfxContentType mContentType;
  gfx::IntSize mSize;

  friend class CompositingFactory;
};


class DeprecatedTextureClientShmemYCbCr : public DeprecatedTextureClient
{
public:
  DeprecatedTextureClientShmemYCbCr(CompositableForwarder* aForwarder, const TextureInfo& aTextureInfo)
    : DeprecatedTextureClient(aForwarder, aTextureInfo)
  { }
  ~DeprecatedTextureClientShmemYCbCr() { ReleaseResources(); }

  virtual bool SupportsType(DeprecatedTextureClientType aType) MOZ_OVERRIDE { return aType == TEXTURE_YCBCR; }
  void EnsureAllocated(gfx::IntSize aSize, gfxASurface::gfxContentType aType) MOZ_OVERRIDE;
  virtual void SetDescriptorFromReply(const SurfaceDescriptor& aDescriptor) MOZ_OVERRIDE;
  virtual void SetDescriptor(const SurfaceDescriptor& aDescriptor) MOZ_OVERRIDE;
  virtual void ReleaseResources();
  virtual gfxASurface::gfxContentType GetContentType() MOZ_OVERRIDE { return gfxASurface::CONTENT_COLOR_ALPHA; }
};

class DeprecatedTextureClientTile : public DeprecatedTextureClient
{
public:
  DeprecatedTextureClientTile(const DeprecatedTextureClientTile& aOther);
  DeprecatedTextureClientTile(CompositableForwarder* aForwarder,
                    const TextureInfo& aTextureInfo);
  ~DeprecatedTextureClientTile();

  virtual void EnsureAllocated(gfx::IntSize aSize,
                               gfxASurface::gfxContentType aType) MOZ_OVERRIDE;

  virtual gfxImageSurface* LockImageSurface() MOZ_OVERRIDE;

  gfxReusableSurfaceWrapper* GetReusableSurfaceWrapper()
  {
    return mSurface;
  }

  virtual void SetDescriptor(const SurfaceDescriptor& aDescriptor) MOZ_OVERRIDE
  {
    MOZ_ASSERT(false, "Tiled texture clients don't use SurfaceDescriptors.");
  }


  virtual gfxASurface::gfxContentType GetContentType() { return mContentType; }

private:
  gfxASurface::gfxContentType mContentType;
  nsRefPtr<gfxReusableSurfaceWrapper> mSurface;

  friend class CompositingFactory;
};






class AutoLockDeprecatedTextureClient
{
public:
  AutoLockDeprecatedTextureClient(DeprecatedTextureClient* aTexture)
  {
    mDeprecatedTextureClient = aTexture;
    mDescriptor = aTexture->LockSurfaceDescriptor();
  }

  SurfaceDescriptor* GetSurfaceDescriptor()
  {
    return mDescriptor;
  }

  virtual ~AutoLockDeprecatedTextureClient()
  {
    mDeprecatedTextureClient->Unlock();
  }
protected:
  DeprecatedTextureClient* mDeprecatedTextureClient;
  SurfaceDescriptor* mDescriptor;
};





class AutoLockYCbCrClient : public AutoLockDeprecatedTextureClient
{
public:
  AutoLockYCbCrClient(DeprecatedTextureClient* aTexture) : AutoLockDeprecatedTextureClient(aTexture) {}
  bool Update(PlanarYCbCrImage* aImage);
protected:
  bool EnsureDeprecatedTextureClient(PlanarYCbCrImage* aImage);
};





class AutoLockShmemClient : public AutoLockDeprecatedTextureClient
{
public:
  AutoLockShmemClient(DeprecatedTextureClient* aTexture) : AutoLockDeprecatedTextureClient(aTexture) {}
  bool Update(Image* aImage, uint32_t aContentFlags, gfxASurface *aSurface);
};

}
}
#endif
