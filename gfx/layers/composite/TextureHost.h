




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

class gfxReusableSurfaceWrapper;
struct nsIntPoint;
struct nsIntSize;
struct nsIntRect;

namespace mozilla {
namespace gl {
class SurfaceStream;
}
namespace ipc {
class Shmem;
}

namespace layers {

class Compositor;
class CompositableHost;
class CompositableBackendSpecificData;
class CompositableParentManager;
class SurfaceDescriptor;
class SurfaceStreamDescriptor;
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
  virtual nsIntRect GetTileRect() = 0;
  virtual size_t GetTileCount() = 0;
  virtual bool NextTile() = 0;
};










class TextureSource
{
protected:
  virtual ~TextureSource();

public:
  NS_INLINE_DECL_REFCOUNTING(TextureSource)

  TextureSource();

  



  virtual gfx::IntSize GetSize() const = 0;

  


  virtual gfx::SurfaceFormat GetFormat() const { return gfx::SurfaceFormat::UNKNOWN; }

  


  virtual TextureSourceOGL* AsSourceOGL() { return nullptr; }
  virtual TextureSourceD3D9* AsSourceD3D9() { return nullptr; }
  virtual TextureSourceD3D11* AsSourceD3D11() { return nullptr; }
  virtual TextureSourceBasic* AsSourceBasic() { return nullptr; }

  


  virtual DataTextureSource* AsDataTextureSource() { return nullptr; }

  



  virtual TextureSource* GetSubSource(int index) { return nullptr; }

  



  virtual BigImageIterator* AsBigImageIterator() { return nullptr; }

  virtual void SetCompositableBackendSpecificData(CompositableBackendSpecificData* aBackendData);

protected:
  RefPtr<CompositableBackendSpecificData> mCompositableBackendData;
};





class NewTextureSource : public TextureSource
{
public:
  NewTextureSource()
  {
    MOZ_COUNT_CTOR(NewTextureSource);
  }
protected:
  virtual ~NewTextureSource()
  {
    MOZ_COUNT_DTOR(NewTextureSource);
  }

public:
  



  virtual void DeallocateDeviceData() = 0;

  virtual void SetCompositor(Compositor* aCompositor) {}

  void SetNextSibling(NewTextureSource* aTexture)
  {
    mNextSibling = aTexture;
  }

  NewTextureSource* GetNextSibling() const
  {
    return mNextSibling;
  }

  
  virtual TextureSource* GetSubSource(int index) MOZ_OVERRIDE
  {
    switch (index) {
      case 0: return this;
      case 1: return GetNextSibling();
      case 2: return GetNextSibling() ? GetNextSibling()->GetNextSibling() : nullptr;
    }
    return nullptr;
  }

protected:
  RefPtr<NewTextureSource> mNextSibling;
};






class DataTextureSource : public NewTextureSource
{
public:
  DataTextureSource()
    : mUpdateSerial(0)
  {}

  virtual DataTextureSource* AsDataTextureSource() MOZ_OVERRIDE { return this; }

  





  virtual bool Update(gfx::DataSourceSurface* aSurface,
                      nsIntRegion* aDestRegion = nullptr,
                      gfx::IntPoint* aSrcOffset = nullptr) = 0;

  







  uint32_t GetUpdateSerial() const { return mUpdateSerial; }
  void SetUpdateSerial(uint32_t aValue) { mUpdateSerial = aValue; }

  
  
  virtual void DeallocateDeviceData() MOZ_OVERRIDE
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

  






  virtual NewTextureSource* GetTextureSources() = 0;

  








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

  static void SendFenceHandleIfPresent(PTextureParent* actor);

  FenceHandle GetAndResetReleaseFenceHandle();

  



  virtual LayerRenderState GetRenderState()
  {
    
    
    return LayerRenderState();
  }

  virtual void SetCompositableBackendSpecificData(CompositableBackendSpecificData* aBackendData);

  
  
  virtual void OnShutdown() {}

  
  virtual void ForgetBufferActor() {}

  virtual const char *Name() { return "TextureHost"; }
  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix);

  




  virtual bool HasInternalBuffer() const { return false; }

  


  virtual TextureHostOGL* AsHostOGL() { return nullptr; }

protected:
  PTextureParent* mActor;
  TextureFlags mFlags;
  RefPtr<CompositableBackendSpecificData> mCompositableBackendData;

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

  virtual void Updated(const nsIntRegion* aRegion = nullptr) MOZ_OVERRIDE;

  virtual bool Lock() MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual NewTextureSource* GetTextureSources() MOZ_OVERRIDE;

  virtual void DeallocateDeviceData() MOZ_OVERRIDE;

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  






  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() MOZ_OVERRIDE;

  virtual bool HasInternalBuffer() const MOZ_OVERRIDE { return true; }

protected:
  bool Upload(nsIntRegion *aRegion = nullptr);
  bool MaybeUpload(nsIntRegion *aRegion = nullptr);

  Compositor* mCompositor;
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
  virtual void DeallocateSharedData() MOZ_OVERRIDE;

  virtual void ForgetSharedData() MOZ_OVERRIDE;

  virtual uint8_t* GetBuffer() MOZ_OVERRIDE;

  virtual size_t GetBufferSize() MOZ_OVERRIDE;

  virtual const char *Name() MOZ_OVERRIDE { return "ShmemTextureHost"; }

  virtual void OnShutdown() MOZ_OVERRIDE;

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
  virtual void DeallocateSharedData() MOZ_OVERRIDE;

  virtual void ForgetSharedData() MOZ_OVERRIDE;

  virtual uint8_t* GetBuffer() MOZ_OVERRIDE;

  virtual size_t GetBufferSize() MOZ_OVERRIDE;

  virtual const char *Name() MOZ_OVERRIDE { return "MemoryTextureHost"; }

protected:
  uint8_t* mBuffer;
};




class StreamTextureHost : public TextureHost
{
public:
  StreamTextureHost(TextureFlags aFlags,
                    const SurfaceStreamDescriptor& aDesc);

  virtual ~StreamTextureHost();

  virtual void DeallocateDeviceData() MOZ_OVERRIDE {};

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual bool Lock() MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE;

  virtual NewTextureSource* GetTextureSources() MOZ_OVERRIDE
  {
    return mTextureSource;
  }

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() MOZ_OVERRIDE
  {
    return nullptr; 
  }

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "StreamTextureHost"; }
#endif

protected:
  Compositor* mCompositor;
  gl::SurfaceStream* mStream;
  RefPtr<NewTextureSource> mTextureSource;
  RefPtr<DataTextureSource> mDataTextureSource;
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





class CompositingRenderTarget : public TextureSource
{
public:
  explicit CompositingRenderTarget(const gfx::IntPoint& aOrigin)
    : mOrigin(aOrigin)
  {}
  virtual ~CompositingRenderTarget() {}

#ifdef MOZ_DUMP_PAINTING
  virtual TemporaryRef<gfx::DataSourceSurface> Dump(Compositor* aCompositor) { return nullptr; }
#endif

  const gfx::IntPoint& GetOrigin() { return mOrigin; }

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
