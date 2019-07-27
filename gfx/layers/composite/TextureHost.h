




#ifndef MOZILLA_GFX_TEXTUREHOST_H
#define MOZILLA_GFX_TEXTUREHOST_H

#include <stddef.h>                     
#include <stdint.h>                     
#include "gfxTypes.h"
#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/2D.h"             
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/FenceUtils.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/mozalloc.h"           
#include "mozilla/UniquePtr.h"          
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRegion.h"                   
#include "nsTraceRefcnt.h"              
#include "nscore.h"                     
#include "mozilla/layers/AtomicRefCountedWithFinalize.h"
#include "mozilla/gfx/Rect.h"

class gfxReusableSurfaceWrapper;

namespace mozilla {
namespace gl {
class SharedSurface;
}
namespace ipc {
class Shmem;
}

namespace layers {

class Compositor;
class CompositableHost;
class CompositableParentManager;
class SurfaceDescriptor;
class SharedSurfaceDescriptor;
class ISurfaceAllocator;
class TextureHostOGL;
class TextureSourceOGL;
class TextureSourceD3D9;
class TextureSourceD3D11;
class TextureSourceBasic;
class DataTextureSource;
class PTextureParent;
class TextureParent;









class BigImageIterator
{
public:
  virtual void BeginBigImageIteration() = 0;
  virtual void EndBigImageIteration() {};
  virtual gfx::IntRect GetTileRect() = 0;
  virtual size_t GetTileCount() = 0;
  virtual bool NextTile() = 0;
};










class TextureSource: public RefCounted<TextureSource>
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(TextureSource)

  TextureSource();

  virtual ~TextureSource();

  



  virtual void DeallocateDeviceData() {}


  



  virtual gfx::IntSize GetSize() const = 0;

  


  virtual gfx::SurfaceFormat GetFormat() const { return gfx::SurfaceFormat::UNKNOWN; }

  


  virtual TextureSourceOGL* AsSourceOGL() { return nullptr; }
  virtual TextureSourceD3D9* AsSourceD3D9() { return nullptr; }
  virtual TextureSourceD3D11* AsSourceD3D11() { return nullptr; }
  virtual TextureSourceBasic* AsSourceBasic() { return nullptr; }
  


  virtual DataTextureSource* AsDataTextureSource() { return nullptr; }

  



  virtual BigImageIterator* AsBigImageIterator() { return nullptr; }

  virtual void SetCompositor(Compositor* aCompositor) {}

  void SetNextSibling(TextureSource* aTexture) { mNextSibling = aTexture; }

  TextureSource* GetNextSibling() const { return mNextSibling; }

  



  TextureSource* GetSubSource(int index)
  {
    switch (index) {
      case 0: return this;
      case 1: return GetNextSibling();
      case 2: return GetNextSibling() ? GetNextSibling()->GetNextSibling() : nullptr;
    }
    return nullptr;
  }

  void AddCompositableRef() { ++mCompositableCount; }

  void ReleaseCompositableRef() {
    --mCompositableCount;
    MOZ_ASSERT(mCompositableCount >= 0);
  }

  int NumCompositableRefs() const { return mCompositableCount; }

protected:

  RefPtr<TextureSource> mNextSibling;
  int mCompositableCount;
};





template<typename T>
class CompositableTextureRef {
public:
  CompositableTextureRef() {}

  explicit CompositableTextureRef(const CompositableTextureRef& aOther)
  {
    *this = aOther;
  }

  explicit CompositableTextureRef(T* aOther)
  {
    *this = aOther;
  }

  ~CompositableTextureRef()
  {
    if (mRef) {
      mRef->ReleaseCompositableRef();
    }
  }

  CompositableTextureRef& operator=(const CompositableTextureRef& aOther)
  {
    if (aOther.get()) {
      aOther->AddCompositableRef();
    }
    if (mRef) {
      mRef->ReleaseCompositableRef();
    }
    mRef = aOther.get();
    return *this;
  }

  CompositableTextureRef& operator=(T* aOther)
  {
    if (aOther) {
      aOther->AddCompositableRef();
    }
    if (mRef) {
      mRef->ReleaseCompositableRef();
    }
    mRef = aOther;
    return *this;
  }

  T* get() const { return mRef; }
  operator T*() const { return mRef; }
  T* operator->() const { return mRef; }
  T& operator*() const { return *mRef; }

private:
  RefPtr<T> mRef;
};

typedef CompositableTextureRef<TextureSource> CompositableTextureSourceRef;
typedef CompositableTextureRef<TextureHost> CompositableTextureHostRef;






class DataTextureSource : public TextureSource
{
public:
  DataTextureSource()
    : mUpdateSerial(0)
  {}

  virtual DataTextureSource* AsDataTextureSource() override { return this; }

  





  virtual bool Update(gfx::DataSourceSurface* aSurface,
                      nsIntRegion* aDestRegion = nullptr,
                      gfx::IntPoint* aSrcOffset = nullptr) = 0;

  







  uint32_t GetUpdateSerial() const { return mUpdateSerial; }
  void SetUpdateSerial(uint32_t aValue) { mUpdateSerial = aValue; }

  
  
  virtual void DeallocateDeviceData() override
  {
    SetUpdateSerial(0);
  }

#ifdef DEBUG
  





  virtual TemporaryRef<gfx::DataSourceSurface> ReadBack() { return nullptr; };
#endif

private:
  uint32_t mUpdateSerial;
};





























class TextureHost
  : public AtomicRefCountedWithFinalize<TextureHost>
{
  





  void Finalize();

  friend class AtomicRefCountedWithFinalize<TextureHost>;
public:
  explicit TextureHost(TextureFlags aFlags);

protected:
  virtual ~TextureHost();

public:
  


  static TemporaryRef<TextureHost> Create(const SurfaceDescriptor& aDesc,
                                          ISurfaceAllocator* aDeallocator,
                                          TextureFlags aFlags);

  







  void CompositorRecycle();

  


  virtual bool Lock() { return true; }

  


  virtual void Unlock() {}

  




  virtual gfx::SurfaceFormat GetFormat() const = 0;

  




  virtual void PrepareTextureSource(CompositableTextureSourceRef& aTexture) {}

  




  virtual bool BindTextureSource(CompositableTextureSourceRef& aTexture) = 0;

  


  virtual void UnbindTextureSource() {}

  








  virtual void Updated(const nsIntRegion* aRegion = nullptr) {}

  






  virtual void SetCompositor(Compositor* aCompositor) {}

  



  virtual void DeallocateDeviceData() {}

  



  virtual void DeallocateSharedData() {}

  





  virtual void ForgetSharedData() {}

  virtual gfx::IntSize GetSize() const = 0;

  



  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() = 0;

  


  void SetFlags(TextureFlags aFlags) { mFlags = aFlags; }

  


  void AddFlag(TextureFlags aFlag) { mFlags |= aFlag; }

  TextureFlags GetFlags() { return mFlags; }

  







  static PTextureParent* CreateIPDLActor(CompositableParentManager* aManager,
                                         const SurfaceDescriptor& aSharedData,
                                         TextureFlags aFlags);
  static bool DestroyIPDLActor(PTextureParent* actor);

  


  static bool SendDeleteIPDLActor(PTextureParent* actor);

  


  static TextureHost* AsTextureHost(PTextureParent* actor);

  





  PTextureParent* GetIPDLActor();

  virtual FenceHandle GetAndResetReleaseFenceHandle();

  



  virtual LayerRenderState GetRenderState()
  {
    
    
    return LayerRenderState();
  }

  
  
  virtual void OnShutdown() {}

  
  virtual void ForgetBufferActor() {}

  virtual const char *Name() { return "TextureHost"; }
  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix);

  




  virtual bool HasInternalBuffer() const { return false; }

  


  virtual TextureHostOGL* AsHostOGL() { return nullptr; }

  void AddCompositableRef() { ++mCompositableCount; }

  void ReleaseCompositableRef()
  {
    --mCompositableCount;
    MOZ_ASSERT(mCompositableCount >= 0);
    if (mCompositableCount == 0) {
      UnbindTextureSource();
    }
  }

  int NumCompositableRefs() const { return mCompositableCount; }

protected:
  void RecycleTexture(TextureFlags aFlags);

  PTextureParent* mActor;
  TextureFlags mFlags;
  int mCompositableCount;

  friend class TextureParent;
};














class BufferTextureHost : public TextureHost
{
public:
  BufferTextureHost(gfx::SurfaceFormat aFormat,
                    TextureFlags aFlags);

  ~BufferTextureHost();

  virtual uint8_t* GetBuffer() = 0;

  virtual size_t GetBufferSize() = 0;

  virtual void Updated(const nsIntRegion* aRegion = nullptr) override;

  virtual bool Lock() override;

  virtual void Unlock() override;

  virtual bool BindTextureSource(CompositableTextureSourceRef& aTexture) override;

  virtual void DeallocateDeviceData() override;

  virtual void SetCompositor(Compositor* aCompositor) override;

  






  virtual gfx::SurfaceFormat GetFormat() const override;

  virtual gfx::IntSize GetSize() const override { return mSize; }

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() override;

  virtual bool HasInternalBuffer() const override { return true; }

protected:
  bool Upload(nsIntRegion *aRegion = nullptr);
  bool MaybeUpload(nsIntRegion *aRegion = nullptr);

  void InitSize();

  RefPtr<Compositor> mCompositor;
  RefPtr<DataTextureSource> mFirstSource;
  nsIntRegion mMaybeUpdatedRegion;
  gfx::IntSize mSize;
  
  gfx::SurfaceFormat mFormat;
  uint32_t mUpdateSerial;
  bool mLocked;
  bool mNeedsFullUpdate;
};






class ShmemTextureHost : public BufferTextureHost
{
public:
  ShmemTextureHost(const mozilla::ipc::Shmem& aShmem,
                   gfx::SurfaceFormat aFormat,
                   ISurfaceAllocator* aDeallocator,
                   TextureFlags aFlags);

protected:
  ~ShmemTextureHost();

public:
  virtual void DeallocateSharedData() override;

  virtual void ForgetSharedData() override;

  virtual uint8_t* GetBuffer() override;

  virtual size_t GetBufferSize() override;

  virtual const char *Name() override { return "ShmemTextureHost"; }

  virtual void OnShutdown() override;

protected:
  UniquePtr<mozilla::ipc::Shmem> mShmem;
  RefPtr<ISurfaceAllocator> mDeallocator;
};







class MemoryTextureHost : public BufferTextureHost
{
public:
  MemoryTextureHost(uint8_t* aBuffer,
                    gfx::SurfaceFormat aFormat,
                    TextureFlags aFlags);

protected:
  ~MemoryTextureHost();

public:
  virtual void DeallocateSharedData() override;

  virtual void ForgetSharedData() override;

  virtual uint8_t* GetBuffer() override;

  virtual size_t GetBufferSize() override;

  virtual const char *Name() override { return "MemoryTextureHost"; }

protected:
  uint8_t* mBuffer;
};




class SharedSurfaceTextureHost : public TextureHost
{
public:
  SharedSurfaceTextureHost(TextureFlags aFlags,
                           const SharedSurfaceDescriptor& aDesc);

  virtual ~SharedSurfaceTextureHost() {
    MOZ_ASSERT(!mIsLocked);
  }

  virtual void DeallocateDeviceData() override {};

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() override {
    return nullptr; 
  }

  virtual void SetCompositor(Compositor* aCompositor) override {
    MOZ_ASSERT(!mIsLocked);

    if (aCompositor == mCompositor)
      return;

    mTexSource = nullptr;
    mCompositor = aCompositor;
  }

public:

  virtual bool Lock() override;
  virtual void Unlock() override;

  virtual bool BindTextureSource(CompositableTextureSourceRef& aTexture) override {
    MOZ_ASSERT(mIsLocked);
    MOZ_ASSERT(mTexSource);
    aTexture = mTexSource;
    return !!aTexture;
  }

  virtual gfx::SurfaceFormat GetFormat() const override;

  virtual gfx::IntSize GetSize() const override;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() override { return "SharedSurfaceTextureHost"; }
#endif

protected:
  void EnsureTexSource();

  bool mIsLocked;
  gl::SharedSurface* const mSurf;
  RefPtr<Compositor> mCompositor;
  RefPtr<TextureSource> mTexSource;
};

class MOZ_STACK_CLASS AutoLockTextureHost
{
public:
  explicit AutoLockTextureHost(TextureHost* aTexture)
    : mTexture(aTexture)
  {
    mLocked = mTexture ? mTexture->Lock() : false;
  }

  ~AutoLockTextureHost()
  {
    if (mTexture && mLocked) {
      mTexture->Unlock();
    }
  }

  bool Failed() { return mTexture && !mLocked; }

private:
  RefPtr<TextureHost> mTexture;
  bool mLocked;
};





class CompositingRenderTarget: public TextureSource
{
public:

  explicit CompositingRenderTarget(const gfx::IntPoint& aOrigin)
    : mClearOnBind(false)
    , mOrigin(aOrigin)
  {}
  virtual ~CompositingRenderTarget() {}

#ifdef MOZ_DUMP_PAINTING
  virtual TemporaryRef<gfx::DataSourceSurface> Dump(Compositor* aCompositor) { return nullptr; }
#endif

  



  void ClearOnBind() {
    mClearOnBind = true;
  }

  const gfx::IntPoint& GetOrigin() { return mOrigin; }
  gfx::IntRect GetRect() { return gfx::IntRect(GetOrigin(), GetSize()); }

protected:
  bool mClearOnBind;

private:
  gfx::IntPoint mOrigin;
};





TemporaryRef<TextureHost>
CreateBackendIndependentTextureHost(const SurfaceDescriptor& aDesc,
                                    ISurfaceAllocator* aDeallocator,
                                    TextureFlags aFlags);

}
}

#endif
