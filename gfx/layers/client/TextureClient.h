




#ifndef MOZILLA_GFX_TEXTURECLIENT_H
#define MOZILLA_GFX_TEXTURECLIENT_H

#include <stddef.h>                     
#include <stdint.h>                     
#include "GLContextTypes.h"             
#include "GLTextureImage.h"             
#include "ImageTypes.h"                 
#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/2D.h"             
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/FenceUtils.h"  
#include "mozilla/ipc/Shmem.h"          
#include "mozilla/layers/AtomicRefCountedWithFinalize.h"
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/PTextureChild.h" 
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsISupportsImpl.h"            

class gfxReusableSurfaceWrapper;
class gfxASurface;
class gfxImageSurface;

namespace mozilla {
namespace layers {

class ContentClient;
class CompositableForwarder;
class ISurfaceAllocator;
class CompositableClient;
class PlanarYCbCrImage;
class PlanarYCbCrData;
class Image;
class PTextureChild;
class TextureChild;
class BufferTextureClient;









enum TextureAllocationFlags {
  ALLOC_DEFAULT = 0,
  ALLOC_CLEAR_BUFFER = 1
};




class TextureClientSurface
{
public:
  virtual bool UpdateSurface(gfxASurface* aSurface) = 0;
  virtual already_AddRefed<gfxASurface> GetAsSurface() = 0;
  






  virtual bool AllocateForSurface(gfx::IntSize aSize,
                                  TextureAllocationFlags flags = ALLOC_DEFAULT) = 0;
};




class TextureClientDrawTarget
{
public:
  
























  virtual TemporaryRef<gfx::DrawTarget> GetAsDrawTarget() = 0;
  virtual gfx::SurfaceFormat GetFormat() const = 0;
  






  virtual bool AllocateForSurface(gfx::IntSize aSize,
                                  TextureAllocationFlags flags = ALLOC_DEFAULT) = 0;
};




class TextureClientYCbCr
{
public:
  




  virtual bool UpdateYCbCr(const PlanarYCbCrData& aData) = 0;

  






  virtual bool AllocateForYCbCr(gfx::IntSize aYSize,
                                gfx::IntSize aCbCrSize,
                                StereoMode aStereoMode) = 0;
};

















class TextureClientData {
public:
  virtual void DeallocateSharedData(ISurfaceAllocator* allocator) = 0;
  virtual ~TextureClientData() {}
};
























class TextureClient
  : public AtomicRefCountedWithFinalize<TextureClient>
{
public:
  TextureClient(TextureFlags aFlags = TEXTURE_FLAGS_DEFAULT);
  virtual ~TextureClient();

  static TemporaryRef<BufferTextureClient>
  CreateBufferTextureClient(ISurfaceAllocator* aAllocator,
                            gfx::SurfaceFormat aFormat,
                            TextureFlags aTextureFlags,
                            gfx::BackendType aMoz2dBackend);

  static TemporaryRef<TextureClient>
  CreateTextureClientForDrawing(ISurfaceAllocator* aAllocator,
                                gfx::SurfaceFormat aFormat,
                                TextureFlags aTextureFlags,
                                gfx::BackendType aMoz2dBackend,
                                const gfx::IntSize& aSizeHint);

  virtual TextureClientSurface* AsTextureClientSurface() { return nullptr; }
  virtual TextureClientDrawTarget* AsTextureClientDrawTarget() { return nullptr; }
  virtual TextureClientYCbCr* AsTextureClientYCbCr() { return nullptr; }

  





  virtual bool Lock(OpenMode aMode) { return IsValid(); }

  virtual void Unlock() {}

  virtual bool IsLocked() const = 0;

  




  virtual bool CopyToTextureClient(TextureClient* aTarget,
                                   const gfx::IntRect* aRect,
                                   const gfx::IntPoint* aPoint);

  




  virtual bool ImplementsLocking() const { return false; }

  




  virtual bool HasInternalBuffer() const = 0;

  







  static PTextureChild* CreateIPDLActor();
  static bool DestroyIPDLActor(PTextureChild* actor);

  


  static TextureClient* AsTextureClient(PTextureChild* actor);

  virtual bool IsAllocated() const = 0;

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aDescriptor) = 0;

  virtual gfx::IntSize GetSize() const = 0;

  





  TextureFlags GetFlags() const { return mFlags; }

  






  void WaitForCompositorRecycle();

  




  bool IsImmutable() const { return mFlags & TEXTURE_IMMUTABLE; }

  void MarkImmutable() { AddFlags(TEXTURE_IMMUTABLE); }

  bool IsSharedWithCompositor() const { return mShared; }

  bool ShouldDeallocateInDestructor() const;

  



  bool IsValid() const { return mValid; }

  




  bool InitIPDLActor(CompositableForwarder* aForwarder);

  





  PTextureChild* GetIPDLActor();

  






  void ForceRemove();

  virtual void SetReleaseFenceHandle(FenceHandle aReleaseFenceHandle) {}

  const FenceHandle& GetReleaseFenceHandle() const
  {
    return mReleaseFenceHandle;
  }

  




  virtual void WaitReleaseFence() {}

private:
  





  void Finalize();

  friend class AtomicRefCountedWithFinalize<TextureClient>;

protected:
  



  void MarkInvalid() { mValid = false; }

  







  virtual TextureClientData* DropTextureData() = 0;

  void AddFlags(TextureFlags  aFlags)
  {
    MOZ_ASSERT(!IsSharedWithCompositor());
    mFlags |= aFlags;
  }

  RefPtr<TextureChild> mActor;
  TextureFlags mFlags;
  bool mShared;
  bool mValid;
  FenceHandle mReleaseFenceHandle;

  friend class TextureChild;
};






class BufferTextureClient : public TextureClient
                          , public TextureClientSurface
                          , public TextureClientYCbCr
                          , public TextureClientDrawTarget
{
public:
  BufferTextureClient(ISurfaceAllocator* aAllocator, gfx::SurfaceFormat aFormat,
                      gfx::BackendType aBackend, TextureFlags aFlags);

  virtual ~BufferTextureClient();

  virtual bool IsAllocated() const = 0;

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aDescriptor) = 0;

  virtual uint8_t* GetBuffer() const = 0;

  virtual gfx::IntSize GetSize() const { return mSize; }

  virtual bool Lock(OpenMode aMode) MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual bool IsLocked() const MOZ_OVERRIDE { return mLocked; }

  

  virtual TextureClientSurface* AsTextureClientSurface() MOZ_OVERRIDE { return this; }

  virtual bool UpdateSurface(gfxASurface* aSurface) MOZ_OVERRIDE;

  virtual already_AddRefed<gfxASurface> GetAsSurface() MOZ_OVERRIDE;

  virtual bool AllocateForSurface(gfx::IntSize aSize,
                                  TextureAllocationFlags aFlags = ALLOC_DEFAULT) MOZ_OVERRIDE;

  

  virtual TextureClientDrawTarget* AsTextureClientDrawTarget() MOZ_OVERRIDE { return this; }

  virtual TemporaryRef<gfx::DrawTarget> GetAsDrawTarget() MOZ_OVERRIDE;

  

  virtual TextureClientYCbCr* AsTextureClientYCbCr() MOZ_OVERRIDE { return this; }

  virtual bool UpdateYCbCr(const PlanarYCbCrData& aData) MOZ_OVERRIDE;

  virtual bool AllocateForYCbCr(gfx::IntSize aYSize,
                                gfx::IntSize aCbCrSize,
                                StereoMode aStereoMode) MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE { return mFormat; }

  
  
  
  
  virtual bool Allocate(uint32_t aSize) = 0;

  virtual size_t GetBufferSize() const = 0;

  virtual bool HasInternalBuffer() const MOZ_OVERRIDE { return true; }

  ISurfaceAllocator* GetAllocator() const;

protected:
  RefPtr<gfx::DrawTarget> mDrawTarget;
  RefPtr<ISurfaceAllocator> mAllocator;
  gfx::SurfaceFormat mFormat;
  gfx::IntSize mSize;
  gfx::BackendType mBackend;
  OpenMode mOpenMode;
  bool mUsingFallbackDrawTarget;
  bool mLocked;
};





class ShmemTextureClient : public BufferTextureClient
{
public:
  ShmemTextureClient(ISurfaceAllocator* aAllocator, gfx::SurfaceFormat aFormat,
                     gfx::BackendType aBackend, TextureFlags aFlags);

  ~ShmemTextureClient();

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aDescriptor) MOZ_OVERRIDE;

  virtual bool Allocate(uint32_t aSize) MOZ_OVERRIDE;

  virtual uint8_t* GetBuffer() const MOZ_OVERRIDE;

  virtual size_t GetBufferSize() const MOZ_OVERRIDE;

  virtual bool IsAllocated() const MOZ_OVERRIDE { return mAllocated; }

  virtual TextureClientData* DropTextureData() MOZ_OVERRIDE;

  virtual bool HasInternalBuffer() const MOZ_OVERRIDE { return true; }

  mozilla::ipc::Shmem& GetShmem() { return mShmem; }

protected:
  mozilla::ipc::Shmem mShmem;
  bool mAllocated;
};






class MemoryTextureClient : public BufferTextureClient
{
public:
  MemoryTextureClient(ISurfaceAllocator* aAllocator, gfx::SurfaceFormat aFormat,
                      gfx::BackendType aBackend, TextureFlags aFlags);

  ~MemoryTextureClient();

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aDescriptor) MOZ_OVERRIDE;

  virtual bool Allocate(uint32_t aSize) MOZ_OVERRIDE;

  virtual uint8_t* GetBuffer() const MOZ_OVERRIDE { return mBuffer; }

  virtual size_t GetBufferSize() const MOZ_OVERRIDE { return mBufSize; }

  virtual bool IsAllocated() const MOZ_OVERRIDE { return mBuffer != nullptr; }

  virtual bool HasInternalBuffer() const MOZ_OVERRIDE { return true; }

  virtual TextureClientData* DropTextureData() MOZ_OVERRIDE;

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
  MOZ_DECLARE_REFCOUNTED_TYPENAME(DeprecatedTextureClient)
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

  
  virtual gfx::BackendType BackendType()
  {
    return gfx::BackendType::NONE;
  }

  virtual void ReleaseResources() {}
  



  virtual void Unlock() {}

  




  virtual bool EnsureAllocated(gfx::IntSize aSize,
                               gfxContentType aType) = 0;

  



  virtual void SetDescriptorFromReply(const SurfaceDescriptor& aDescriptor)
  {
    
    SetDescriptor(aDescriptor);
  }
  virtual void SetDescriptor(const SurfaceDescriptor& aDescriptor)
  {
    mDescriptor = aDescriptor;
  }
  SurfaceDescriptor* GetDescriptor() { return &mDescriptor; }
  



  virtual SurfaceDescriptor* LockSurfaceDescriptor() { return GetDescriptor(); }

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

  virtual gfxContentType GetContentType() = 0;

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
  ~DeprecatedTextureClientShmem();
  virtual bool SupportsType(DeprecatedTextureClientType aType) MOZ_OVERRIDE
  {
    return aType == TEXTURE_SHMEM || aType == TEXTURE_CONTENT || aType == TEXTURE_FALLBACK;
  }
  virtual gfxImageSurface* LockImageSurface() MOZ_OVERRIDE;
  virtual gfxASurface* LockSurface() MOZ_OVERRIDE { return GetSurface(); }
  virtual gfx::DrawTarget* LockDrawTarget();
  virtual gfx::BackendType BackendType() MOZ_OVERRIDE
  {
    return gfx::BackendType::CAIRO;
  }
  virtual void Unlock() MOZ_OVERRIDE;
  virtual bool EnsureAllocated(gfx::IntSize aSize, gfxContentType aType) MOZ_OVERRIDE;

  virtual void ReleaseResources() MOZ_OVERRIDE;
  virtual void SetDescriptor(const SurfaceDescriptor& aDescriptor) MOZ_OVERRIDE;
  virtual gfxContentType GetContentType() MOZ_OVERRIDE { return mContentType; }
private:
  gfxASurface* GetSurface();

  nsRefPtr<gfxASurface> mSurface;
  nsRefPtr<gfxImageSurface> mSurfaceAsImage;
  RefPtr<gfx::DrawTarget> mDrawTarget;

  gfxContentType mContentType;
  gfx::IntSize mSize;

  friend class CompositingFactory;
};

class DeprecatedTextureClientTile : public DeprecatedTextureClient
{
public:
  DeprecatedTextureClientTile(const DeprecatedTextureClientTile& aOther);
  DeprecatedTextureClientTile(CompositableForwarder* aForwarder,
                              const TextureInfo& aTextureInfo,
                              gfxReusableSurfaceWrapper* aSurface = nullptr);
  ~DeprecatedTextureClientTile();

  virtual bool EnsureAllocated(gfx::IntSize aSize,
                               gfxContentType aType) MOZ_OVERRIDE;

  virtual gfxImageSurface* LockImageSurface() MOZ_OVERRIDE;

  gfxReusableSurfaceWrapper* GetReusableSurfaceWrapper()
  {
    return mSurface;
  }

  virtual void SetDescriptor(const SurfaceDescriptor& aDescriptor) MOZ_OVERRIDE
  {
    MOZ_ASSERT(false, "Tiled texture clients don't use SurfaceDescriptors.");
  }


  virtual gfxContentType GetContentType() { return mContentType; }

private:
  gfxContentType mContentType;
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
