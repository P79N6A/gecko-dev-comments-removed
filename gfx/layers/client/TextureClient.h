




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
#include "GfxTexturesReporter.h"

class gfxReusableSurfaceWrapper;
class gfxImageSurface;

namespace mozilla {
namespace gl {
class GLContext;
class SurfaceStream;
}

namespace layers {

class AsyncTransactionTracker;
class ContentClient;
class CompositableForwarder;
class ISurfaceAllocator;
class CompositableClient;
class PlanarYCbCrImage;
struct PlanarYCbCrData;
class Image;
class PTextureChild;
class TextureChild;
class BufferTextureClient;
class TextureClient;
class KeepAlive;






enum TextureAllocationFlags {
  ALLOC_DEFAULT = 0,
  ALLOC_CLEAR_BUFFER = 1,
  ALLOC_CLEAR_BUFFER_WHITE = 2
};




class TextureClientYCbCr
{
public:
  




  virtual bool UpdateYCbCr(const PlanarYCbCrData& aData) = 0;

  






  virtual bool AllocateForYCbCr(gfx::IntSize aYSize,
                                gfx::IntSize aCbCrSize,
                                StereoMode aStereoMode) = 0;
};






class TextureReadbackSink
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TextureReadbackSink)
public:
  





  virtual void ProcessReadback(gfx::DataSourceSurface *aSourceSurface) = 0;
};
























class TextureClient
  : public AtomicRefCountedWithFinalize<TextureClient>
{
public:
  TextureClient(TextureFlags aFlags = TextureFlags::DEFAULT);
  virtual ~TextureClient();

  
  static TemporaryRef<TextureClient>
  CreateForDrawing(ISurfaceAllocator* aAllocator,
                   gfx::SurfaceFormat aFormat,
                   gfx::IntSize aSize,
                   gfx::BackendType aMoz2dBackend,
                   TextureFlags aTextureFlags,
                   TextureAllocationFlags flags = ALLOC_DEFAULT);

  
  static TemporaryRef<BufferTextureClient>
  CreateForYCbCr(ISurfaceAllocator* aAllocator,
                 gfx::IntSize aYSize,
                 gfx::IntSize aCbCrSize,
                 StereoMode aStereoMode,
                 TextureFlags aTextureFlags);

  
  
  static TemporaryRef<BufferTextureClient>
  CreateForRawBufferAccess(ISurfaceAllocator* aAllocator,
                           gfx::SurfaceFormat aFormat,
                           gfx::IntSize aSize,
                           gfx::BackendType aMoz2dBackend,
                           TextureFlags aTextureFlags,
                           TextureAllocationFlags flags = ALLOC_DEFAULT);

  
  
  
  static TemporaryRef<BufferTextureClient>
  CreateWithBufferSize(ISurfaceAllocator* aAllocator,
                       gfx::SurfaceFormat aFormat,
                       size_t aSize,
                       TextureFlags aTextureFlags);

  
  virtual TemporaryRef<TextureClient>
  CreateSimilar(TextureFlags aFlags = TextureFlags::DEFAULT,
                TextureAllocationFlags aAllocFlags = ALLOC_DEFAULT) const = 0;

  








  virtual bool AllocateForSurface(gfx::IntSize aSize,
                                  TextureAllocationFlags flags = ALLOC_DEFAULT)
  {
    return false;
  }

  virtual TextureClientYCbCr* AsTextureClientYCbCr() { return nullptr; }

  





  virtual bool Lock(OpenMode aMode) { return IsValid(); }

  virtual void Unlock() {}

  virtual bool IsLocked() const = 0;

  virtual bool CanExposeDrawTarget() const { return false; }

  
























  virtual gfx::DrawTarget* BorrowDrawTarget() { return nullptr; }

  
  virtual gfx::SurfaceFormat GetFormat() const
  {
    return gfx::SurfaceFormat::UNKNOWN;
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

  





  void KeepUntilFullDeallocation(KeepAlive* aKeep);

  




  bool InitIPDLActor(CompositableForwarder* aForwarder);

  





  PTextureChild* GetIPDLActor();

  






  void ForceRemove();

  virtual void SetReleaseFenceHandle(FenceHandle aReleaseFenceHandle)
  {
    mReleaseFenceHandle = aReleaseFenceHandle;
  }

  const FenceHandle& GetReleaseFenceHandle() const
  {
    return mReleaseFenceHandle;
  }

  virtual void SetAcquireFenceHandle(FenceHandle aAcquireFenceHandle)
  {
    mAcquireFenceHandle = aAcquireFenceHandle;
  }

  const FenceHandle& GetAcquireFenceHandle() const
  {
    return mAcquireFenceHandle;
  }

  


  virtual void SetRemoveFromCompositableTracker(AsyncTransactionTracker* aTracker) {}

  


  virtual void WaitForBufferOwnership() {}

  



   void SetWaste(int aWasteArea) {
     mWasteTracker.Update(aWasteArea, BytesPerPixel(GetFormat()));
   }

   




   virtual void SetReadbackSink(TextureReadbackSink* aReadbackSink) {
     mReadbackSink = aReadbackSink;
   }

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

  ISurfaceAllocator* GetAllocator()
  {
    return mAllocator;
  }

  RefPtr<TextureChild> mActor;
  RefPtr<ISurfaceAllocator> mAllocator;
  TextureFlags mFlags;
  FenceHandle mReleaseFenceHandle;
  FenceHandle mAcquireFenceHandle;
  gl::GfxTextureWasteTracker mWasteTracker;
  bool mShared;
  bool mValid;

  RefPtr<TextureReadbackSink> mReadbackSink;

  friend class TextureChild;
  friend class RemoveTextureFromCompositableTracker;
  friend void TestTextureClientSurface(TextureClient*, gfxImageSurface*);
  friend void TestTextureClientYCbCr(TextureClient*, PlanarYCbCrData&);
};




class TextureClientReleaseTask : public Task
{
public:
    TextureClientReleaseTask(TextureClient* aClient)
        : mTextureClient(aClient) {
    }

    virtual void Run() MOZ_OVERRIDE
    {
        mTextureClient = nullptr;
    }

private:
    mozilla::RefPtr<TextureClient> mTextureClient;
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

  virtual gfx::DrawTarget* BorrowDrawTarget() MOZ_OVERRIDE;

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

  virtual TemporaryRef<TextureClient>
  CreateSimilar(TextureFlags aFlags = TextureFlags::DEFAULT,
                TextureAllocationFlags aAllocFlags = ALLOC_DEFAULT) const MOZ_OVERRIDE;

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

protected:
  ~ShmemTextureClient();

public:
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

protected:
  ~MemoryTextureClient();

public:
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




class StreamTextureClient : public TextureClient
{
public:
  StreamTextureClient(TextureFlags aFlags);

protected:
  ~StreamTextureClient();

public:
  virtual bool IsAllocated() const MOZ_OVERRIDE;

  virtual bool Lock(OpenMode mode) MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual bool IsLocked() const MOZ_OVERRIDE { return mIsLocked; }

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) MOZ_OVERRIDE;

  virtual bool HasInternalBuffer() const MOZ_OVERRIDE { return false; }

  void InitWith(gl::SurfaceStream* aStream);

  virtual gfx::IntSize GetSize() const { return gfx::IntSize(); }

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE
  {
    return gfx::SurfaceFormat::UNKNOWN;
  }

  
  
  
  virtual TemporaryRef<TextureClient>
  CreateSimilar(TextureFlags, TextureAllocationFlags) const MOZ_OVERRIDE { return nullptr; }

  virtual bool AllocateForSurface(gfx::IntSize aSize, TextureAllocationFlags aFlags) MOZ_OVERRIDE
  {
    MOZ_CRASH("Should never hit this.");
    return false;
  }

protected:
  bool mIsLocked;
  RefPtr<gl::SurfaceStream> mStream;
  RefPtr<gl::GLContext> mGL; 
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

class KeepAlive
{
public:
  virtual ~KeepAlive() {}
};

template<typename T>
class TKeepAlive : public KeepAlive
{
public:
  TKeepAlive(T* aData) : mData(aData) {}
protected:
  RefPtr<T> mData;
};

}
}
#endif
