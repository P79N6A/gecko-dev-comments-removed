




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
#include "mozilla/layers/LayersTypes.h"
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsISupportsImpl.h"            
#include "GfxTexturesReporter.h"

class gfxImageSurface;

namespace mozilla {
namespace gl {
class GLContext;
class SharedSurface;
}



#ifdef DEBUG
#define GFX_DEBUG_TRACK_CLIENTS_IN_POOL 1
#endif

namespace layers {

class AsyncTransactionTracker;
class CompositableForwarder;
class ISurfaceAllocator;
class CompositableClient;
struct PlanarYCbCrData;
class Image;
class PTextureChild;
class TextureChild;
class BufferTextureClient;
class TextureClient;
#ifdef GFX_DEBUG_TRACK_CLIENTS_IN_POOL
class TextureClientPool;
#endif
class KeepAlive;






enum TextureAllocationFlags {
  ALLOC_DEFAULT = 0,
  ALLOC_CLEAR_BUFFER = 1,
  ALLOC_CLEAR_BUFFER_WHITE = 2,
  ALLOC_DISALLOW_BUFFERTEXTURECLIENT = 4
};

#ifdef XP_WIN
typedef void* SyncHandle;
#else
typedef uintptr_t SyncHandle;
#endif 

class SyncObject : public RefCounted<SyncObject>
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(SyncObject)
  virtual ~SyncObject() { }

  static TemporaryRef<SyncObject> CreateSyncObject(SyncHandle aHandle);

  enum class SyncType {
    D3D11,
  };

  virtual SyncType GetSyncType() = 0;
  virtual void FinalizeFrame() = 0;

protected:
  SyncObject() { }
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

protected:
  virtual ~TextureReadbackSink() {}
};
























class TextureClient
  : public AtomicRefCountedWithFinalize<TextureClient>
{
public:
  explicit TextureClient(ISurfaceAllocator* aAllocator,
                         TextureFlags aFlags = TextureFlags::DEFAULT);
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

  



  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() {
    Lock(OpenMode::OPEN_READ);
    RefPtr<gfx::SourceSurface> surf = BorrowDrawTarget()->Snapshot();
    RefPtr<gfx::DataSourceSurface> data = surf->GetDataSurface();
    Unlock();
    return data;
  }

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix);

  




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

  bool HasFlags(TextureFlags aFlags) const
  {
    return (mFlags & aFlags) == aFlags;
  }

  void AddFlags(TextureFlags aFlags);

  void RemoveFlags(TextureFlags aFlags);

  void RecycleTexture(TextureFlags aFlags);

  






  void WaitForCompositorRecycle();

  




  bool IsImmutable() const { return !!(mFlags & TextureFlags::IMMUTABLE); }

  void MarkImmutable() { AddFlags(TextureFlags::IMMUTABLE); }

  bool IsSharedWithCompositor() const { return mShared; }

  bool ShouldDeallocateInDestructor() const;

  



  bool IsValid() const { return mValid; }

  


  void SetAddedToCompositableClient();

  



  bool IsAddedToCompositableClient() const { return mAddedToCompositableClient; }

  





  void KeepUntilFullDeallocation(UniquePtr<KeepAlive> aKeep, bool aMainThreadOnly = false);

  




  bool InitIPDLActor(CompositableForwarder* aForwarder);

  





  PTextureChild* GetIPDLActor();

  








  void ForceRemove(bool sync = false);

  virtual void SetReleaseFenceHandle(FenceHandle aReleaseFenceHandle)
  {
    mReleaseFenceHandle.Merge(aReleaseFenceHandle);
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

  


  virtual void WaitForBufferOwnership(bool aWaitReleaseFence = true) {}

  



   void SetWaste(int aWasteArea) {
     mWasteTracker.Update(aWasteArea, BytesPerPixel(GetFormat()));
   }

   




   virtual void SetReadbackSink(TextureReadbackSink* aReadbackSink) {
     mReadbackSink = aReadbackSink;
   }
   
   virtual void SyncWithObject(SyncObject* aSyncObject) { }

private:
  





  void Finalize();

  friend class AtomicRefCountedWithFinalize<TextureClient>;

protected:
  



  void MarkInvalid() { mValid = false; }

  







  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aDescriptor) = 0;

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
  bool mAddedToCompositableClient;

  RefPtr<TextureReadbackSink> mReadbackSink;

  friend class TextureChild;
  friend class RemoveTextureFromCompositableTracker;
  friend void TestTextureClientSurface(TextureClient*, gfxImageSurface*);
  friend void TestTextureClientYCbCr(TextureClient*, PlanarYCbCrData&);

#ifdef GFX_DEBUG_TRACK_CLIENTS_IN_POOL
public:
  
  TextureClientPool* mPoolTracker;
#endif
};




class TextureClientReleaseTask : public Task
{
public:
    explicit TextureClientReleaseTask(TextureClient* aClient)
        : mTextureClient(aClient) {
    }

    virtual void Run() override
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

  virtual bool IsAllocated() const override = 0;

  virtual uint8_t* GetBuffer() const = 0;

  virtual gfx::IntSize GetSize() const override { return mSize; }

  virtual bool Lock(OpenMode aMode) override;

  virtual void Unlock() override;

  virtual bool IsLocked() const override { return mLocked; }

  uint8_t* GetLockedData() const;

  virtual bool CanExposeDrawTarget() const override { return true; }

  virtual gfx::DrawTarget* BorrowDrawTarget() override;

  virtual bool AllocateForSurface(gfx::IntSize aSize,
                                  TextureAllocationFlags aFlags = ALLOC_DEFAULT) override;

  

  virtual TextureClientYCbCr* AsTextureClientYCbCr() override { return this; }

  virtual bool UpdateYCbCr(const PlanarYCbCrData& aData) override;

  virtual bool AllocateForYCbCr(gfx::IntSize aYSize,
                                gfx::IntSize aCbCrSize,
                                StereoMode aStereoMode) override;

  virtual gfx::SurfaceFormat GetFormat() const override { return mFormat; }

  
  
  
  
  virtual bool Allocate(uint32_t aSize) = 0;

  virtual size_t GetBufferSize() const = 0;

  virtual bool HasInternalBuffer() const override { return true; }

  virtual TemporaryRef<TextureClient>
  CreateSimilar(TextureFlags aFlags = TextureFlags::DEFAULT,
                TextureAllocationFlags aAllocFlags = ALLOC_DEFAULT) const override;

protected:
  RefPtr<gfx::DrawTarget> mDrawTarget;
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
  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aDescriptor) override;

  virtual bool Allocate(uint32_t aSize) override;

  virtual uint8_t* GetBuffer() const override;

  virtual size_t GetBufferSize() const override;

  virtual bool IsAllocated() const override { return mAllocated; }

  virtual bool HasInternalBuffer() const override { return true; }

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
  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aDescriptor) override;

  virtual bool Allocate(uint32_t aSize) override;

  virtual uint8_t* GetBuffer() const override { return mBuffer; }

  virtual size_t GetBufferSize() const override { return mBufSize; }

  virtual bool IsAllocated() const override { return mBuffer != nullptr; }

  virtual bool HasInternalBuffer() const override { return true; }

protected:
  uint8_t* mBuffer;
  size_t mBufSize;
};




class SharedSurfaceTextureClient : public TextureClient
{
public:
  SharedSurfaceTextureClient(ISurfaceAllocator* aAllocator, TextureFlags aFlags,
                             gl::SharedSurface* surf);

protected:
  ~SharedSurfaceTextureClient();

public:
  
  virtual bool IsAllocated() const override { return true; }

  virtual bool Lock(OpenMode) override {
    MOZ_ASSERT(!mIsLocked);
    mIsLocked = true;
    return true;
  }

  virtual void Unlock() override {
    MOZ_ASSERT(mIsLocked);
    mIsLocked = false;
  }

  virtual bool IsLocked() const override { return mIsLocked; }

  virtual bool HasInternalBuffer() const override { return false; }

  virtual gfx::SurfaceFormat GetFormat() const override {
    return gfx::SurfaceFormat::UNKNOWN;
  }

  virtual gfx::IntSize GetSize() const override { return gfx::IntSize(); }

  
  
  
  virtual TemporaryRef<TextureClient>
  CreateSimilar(TextureFlags, TextureAllocationFlags) const override {
    return nullptr;
  }

  virtual bool AllocateForSurface(gfx::IntSize,
                                  TextureAllocationFlags) override {
    MOZ_CRASH("Should never hit this.");
    return false;
  }
  

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) override;

protected:
  bool mIsLocked;
  gl::SharedSurface* const mSurf;
  RefPtr<gl::GLContext> mGL; 
};

struct TextureClientAutoUnlock
{
  TextureClient* mTexture;

  explicit TextureClientAutoUnlock(TextureClient* aTexture)
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
  explicit TKeepAlive(T* aData) : mData(aData) {}
protected:
  RefPtr<T> mData;
};

}
}
#endif
