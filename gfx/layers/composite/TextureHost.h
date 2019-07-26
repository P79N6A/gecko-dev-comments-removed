




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
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/mozalloc.h"           
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRegion.h"                   
#include "nsTraceRefcnt.h"              
#include "nscore.h"                     
#include "mozilla/layers/AtomicRefCountedWithFinalize.h"

class gfxImageSurface;
class gfxReusableSurfaceWrapper;
struct nsIntPoint;
struct nsIntSize;
struct nsIntRect;

namespace mozilla {
namespace ipc {
class Shmem;
}

namespace layers {

class Compositor;
class CompositableHost;
class CompositableBackendSpecificData;
class SurfaceDescriptor;
class ISurfaceAllocator;
class TextureHostOGL;
class TextureSourceOGL;
class TextureSourceD3D9;
class TextureSourceD3D11;
class TextureSourceBasic;
class DataTextureSource;
class PTextureParent;
class TextureParent;









class TileIterator
{
public:
  virtual void BeginTileIteration() = 0;
  virtual void EndTileIteration() {};
  virtual nsIntRect GetTileRect() = 0;
  virtual size_t GetTileCount() = 0;
  virtual bool NextTile() = 0;
};










class TextureSource : public RefCounted<TextureSource>
{
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(TextureSource)
  TextureSource();
  virtual ~TextureSource();

  



  virtual gfx::IntSize GetSize() const = 0;

  


  virtual gfx::SurfaceFormat GetFormat() const { return gfx::SurfaceFormat::UNKNOWN; }

  


  virtual TextureSourceOGL* AsSourceOGL() { return nullptr; }
  virtual TextureSourceD3D9* AsSourceD3D9() { return nullptr; }
  virtual TextureSourceD3D11* AsSourceD3D11() { return nullptr; }
  virtual TextureSourceBasic* AsSourceBasic() { return nullptr; }

  


  virtual DataTextureSource* AsDataTextureSource() { return nullptr; }

  



  virtual TextureSource* GetSubSource(int index) { return nullptr; }

  



  virtual TileIterator* AsTileIterator() { return nullptr; }

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
  virtual ~NewTextureSource()
  {
    MOZ_COUNT_DTOR(NewTextureSource);
  }

  



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
  TextureHost(TextureFlags aFlags);

  virtual ~TextureHost();

  


  static TemporaryRef<TextureHost> Create(const SurfaceDescriptor& aDesc,
                                          ISurfaceAllocator* aDeallocator,
                                          TextureFlags aFlags);

  


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

  







  static PTextureParent* CreateIPDLActor(ISurfaceAllocator* aAllocator,
                                         const SurfaceDescriptor& aSharedData,
                                         TextureFlags aFlags);
  static bool DestroyIPDLActor(PTextureParent* actor);

  


  static bool SendDeleteIPDLActor(PTextureParent* actor);

  


  static TextureHost* AsTextureHost(PTextureParent* actor);

  





  PTextureParent* GetIPDLActor();

  



  virtual LayerRenderState GetRenderState()
  {
    
    
    return LayerRenderState();
  }

  virtual void SetCompositableBackendSpecificData(CompositableBackendSpecificData* aBackendData);

  
  
  virtual void OnShutdown() {}

  
  virtual void ForgetBufferActor() {}

  virtual const char *Name() { return "TextureHost"; }
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);

  




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
  bool mPartialUpdate;
};






class ShmemTextureHost : public BufferTextureHost
{
public:
  ShmemTextureHost(const mozilla::ipc::Shmem& aShmem,
                   gfx::SurfaceFormat aFormat,
                   ISurfaceAllocator* aDeallocator,
                   TextureFlags aFlags);

  ~ShmemTextureHost();

  virtual void DeallocateSharedData() MOZ_OVERRIDE;

  virtual void ForgetSharedData() MOZ_OVERRIDE;

  virtual uint8_t* GetBuffer() MOZ_OVERRIDE;

  virtual size_t GetBufferSize() MOZ_OVERRIDE;

  virtual const char *Name() MOZ_OVERRIDE { return "ShmemTextureHost"; }

  virtual void OnShutdown() MOZ_OVERRIDE;

protected:
  mozilla::ipc::Shmem* mShmem;
  RefPtr<ISurfaceAllocator> mDeallocator;
};







class MemoryTextureHost : public BufferTextureHost
{
public:
  MemoryTextureHost(uint8_t* aBuffer,
                    gfx::SurfaceFormat aFormat,
                    TextureFlags aFlags);

  ~MemoryTextureHost();

  virtual void DeallocateSharedData() MOZ_OVERRIDE;

  virtual void ForgetSharedData() MOZ_OVERRIDE;

  virtual uint8_t* GetBuffer() MOZ_OVERRIDE;

  virtual size_t GetBufferSize() MOZ_OVERRIDE;

  virtual const char *Name() MOZ_OVERRIDE { return "MemoryTextureHost"; }

protected:
  uint8_t* mBuffer;
};




















































class DeprecatedTextureHost : public TextureSource
{
public:
  







  static TemporaryRef<DeprecatedTextureHost> CreateDeprecatedTextureHost(SurfaceDescriptorType aDescriptorType,
                                                     uint32_t aDeprecatedTextureHostFlags,
                                                     uint32_t aTextureFlags,
                                                     CompositableHost* aCompositableHost);

  DeprecatedTextureHost();
  virtual ~DeprecatedTextureHost();

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE { return mFormat; }

  virtual bool IsValid() const { return true; }

  






  void Update(const SurfaceDescriptor& aImage,
              nsIntRegion *aRegion = nullptr,
              nsIntPoint* aOffset = nullptr);

  



  void SwapTextures(const SurfaceDescriptor& aImage,
                    SurfaceDescriptor* aResult = nullptr,
                    nsIntRegion *aRegion = nullptr);

  



  virtual void Update(gfxReusableSurfaceWrapper* aReusableSurface,
  	                  TextureFlags aFlags,
  	                  const gfx::IntSize& aSize) {}

  



  virtual bool Lock() { return IsValid(); }

  



  virtual void Unlock() {}

  void SetFlags(TextureFlags aFlags) { mFlags = aFlags; }
  void AddFlag(TextureFlags aFlag) { mFlags |= aFlag; }
  TextureFlags GetFlags() { return mFlags; }

  






  virtual void SetCompositor(Compositor* aCompositor) {}

  ISurfaceAllocator* GetDeAllocator()
  {
    return mDeAllocator;
  }

  bool operator== (const DeprecatedTextureHost& o) const
  {
    return GetIdentifier() == o.GetIdentifier();
  }
  bool operator!= (const DeprecatedTextureHost& o) const
  {
    return GetIdentifier() != o.GetIdentifier();
  }

  virtual LayerRenderState GetRenderState()
  {
    return LayerRenderState();
  }

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() = 0;

  virtual const char *Name() = 0;
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);

  





  virtual void EnsureBuffer(const nsIntSize& aSize, gfxContentType aType)
  {
    NS_RUNTIMEABORT("DeprecatedTextureHost doesn't support EnsureBuffer");
  }

  







  virtual void CopyTo(const nsIntRect& aSourceRect,
                      DeprecatedTextureHost *aDest,
                      const nsIntRect& aDestRect)
  {
    NS_RUNTIMEABORT("DeprecatedTextureHost doesn't support CopyTo");
  }


  SurfaceDescriptor* GetBuffer() const { return mBuffer; }
  virtual SurfaceDescriptor* LockSurfaceDescriptor() const { return GetBuffer(); }

  





  
  
  virtual void SetBuffer(SurfaceDescriptor* aBuffer, ISurfaceAllocator* aAllocator);

  
  
  virtual void ForgetBuffer() {}

  void OnShutdown();

protected:
  





  virtual void UpdateImpl(const SurfaceDescriptor& aImage,
                          nsIntRegion *aRegion,
                          nsIntPoint *aOffset = nullptr)
  {
    NS_RUNTIMEABORT("Should not be reached");
  }

  








  virtual void SwapTexturesImpl(const SurfaceDescriptor& aImage,
                                nsIntRegion *aRegion)
  {
    UpdateImpl(aImage, aRegion, nullptr);
  }

  
  
  
  virtual uint64_t GetIdentifier() const
  {
    return reinterpret_cast<uint64_t>(this);
  }

  
  TextureFlags mFlags;
  SurfaceDescriptor* mBuffer; 
                              
                              
                              
                              
  RefPtr<ISurfaceAllocator> mDeAllocator;
  gfx::SurfaceFormat mFormat;
};

class MOZ_STACK_CLASS AutoLockTextureHost
{
public:
  AutoLockTextureHost(TextureHost* aTexture)
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

class AutoLockDeprecatedTextureHost
{
public:
  AutoLockDeprecatedTextureHost(DeprecatedTextureHost* aHost)
    : mDeprecatedTextureHost(aHost)
    , mIsValid(true)
  {
    if (mDeprecatedTextureHost) {
      mIsValid = mDeprecatedTextureHost->Lock();
    }
  }

  ~AutoLockDeprecatedTextureHost()
  {
    if (mDeprecatedTextureHost && mIsValid) {
      mDeprecatedTextureHost->Unlock();
    }
  }

  bool IsValid() { return mIsValid; }

private:
  DeprecatedTextureHost *mDeprecatedTextureHost;
  bool mIsValid;
};





class CompositingRenderTarget : public TextureSource
{
public:
  CompositingRenderTarget(const gfx::IntPoint& aOrigin)
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
