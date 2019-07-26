




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
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsISupportsImpl.h"            

class gfxReusableSurfaceWrapper;
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
class TextureClient;






enum TextureAllocationFlags {
  ALLOC_DEFAULT = 0,
  ALLOC_CLEAR_BUFFER = 1
};




class TextureClientYCbCr
{
public:
  




  virtual bool UpdateYCbCr(const PlanarYCbCrData& aData) = 0;

  






  virtual bool AllocateForYCbCr(gfx::IntSize aYSize,
                                gfx::IntSize aCbCrSize,
                                StereoMode aStereoMode) = 0;
};
























class TextureClient
  : public AtomicRefCountedWithFinalize<TextureClient>
{
public:
  TextureClient(TextureFlags aFlags = TextureFlags::DEFAULT);
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

  virtual TextureClientYCbCr* AsTextureClientYCbCr() { return nullptr; }

  





  virtual bool Lock(OpenMode aMode) { return IsValid(); }

  virtual void Unlock() {}

  virtual bool IsLocked() const = 0;

  virtual bool CanExposeDrawTarget() const { return false; }

  
























  virtual TemporaryRef<gfx::DrawTarget> GetAsDrawTarget() { return nullptr; }

  
  virtual gfx::SurfaceFormat GetFormat() const
  {
    return gfx::SurfaceFormat::UNKNOWN;
  }

  








  virtual bool AllocateForSurface(gfx::IntSize aSize,
                                  TextureAllocationFlags flags = ALLOC_DEFAULT)
  {
    return false;
  }

  




  virtual bool CopyToTextureClient(TextureClient* aTarget,
                                   const gfx::IntRect* aRect,
                                   const gfx::IntPoint* aPoint);

  




  virtual bool ImplementsLocking() const { return false; }

  




  virtual bool HasInternalBuffer() const = 0;

  







  static PTextureChild* CreateIPDLActor();
  static bool DestroyIPDLActor(PTextureChild* actor);

  


  static TextureClient* AsTextureClient(PTextureChild* actor);

  virtual bool IsAllocated() const = 0;

  virtual gfx::IntSize GetSize() const = 0;

  





  TextureFlags GetFlags() const { return mFlags; }

  






  void WaitForCompositorRecycle();

  




  bool IsImmutable() const { return !!(mFlags & TextureFlags::IMMUTABLE); }

  void MarkImmutable() { AddFlags(TextureFlags::IMMUTABLE); }

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

  







  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aDescriptor) = 0;

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
  friend void TestTextureClientSurface(TextureClient*, gfxImageSurface*);
  friend void TestTextureClientYCbCr(TextureClient*, PlanarYCbCrData&);
};






class BufferTextureClient : public TextureClient
                          , public TextureClientYCbCr
{
public:
  BufferTextureClient(ISurfaceAllocator* aAllocator, gfx::SurfaceFormat aFormat,
                      gfx::BackendType aBackend, TextureFlags aFlags);

  virtual ~BufferTextureClient();

  virtual bool IsAllocated() const = 0;

  virtual uint8_t* GetBuffer() const = 0;

  virtual gfx::IntSize GetSize() const { return mSize; }

  virtual bool Lock(OpenMode aMode) MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual bool IsLocked() const MOZ_OVERRIDE { return mLocked; }

  virtual bool CanExposeDrawTarget() const MOZ_OVERRIDE { return true; }

  virtual TemporaryRef<gfx::DrawTarget> GetAsDrawTarget() MOZ_OVERRIDE;

  virtual bool AllocateForSurface(gfx::IntSize aSize,
                                  TextureAllocationFlags aFlags = ALLOC_DEFAULT) MOZ_OVERRIDE;

  

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

}
}
#endif
