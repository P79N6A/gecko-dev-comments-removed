




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
#include "nsRegion.h"                   
#include "nsTraceRefcnt.h"              
#include "nscore.h"                     

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
class CompositableQuirks;
class SurfaceDescriptor;
class ISurfaceAllocator;
class TextureSourceOGL;
class TextureSourceD3D9;
class TextureSourceD3D11;
class TextureSourceBasic;
class TextureParent;
class DataTextureSource;









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
  TextureSource();
  virtual ~TextureSource();

  



  virtual gfx::IntSize GetSize() const = 0;

  


  virtual gfx::SurfaceFormat GetFormat() const { return gfx::FORMAT_UNKNOWN; }

  


  virtual TextureSourceOGL* AsSourceOGL() { return nullptr; }
  virtual TextureSourceD3D9* AsSourceD3D9() { return nullptr; }
  virtual TextureSourceD3D11* AsSourceD3D11() { return nullptr; }
  virtual TextureSourceBasic* AsSourceBasic() { return nullptr; }

  


  virtual DataTextureSource* AsDataTextureSource() { return nullptr; }

  



  virtual TextureSource* GetSubSource(int index) { return nullptr; }

  



  virtual TileIterator* AsTileIterator() { return nullptr; }

  virtual void SetCompositableQuirks(CompositableQuirks* aQuirks);

protected:
  RefPtr<CompositableQuirks> mQuirks;
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
                      TextureFlags aFlags,
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





























class TextureHost : public RefCounted<TextureHost>
{
public:
  TextureHost(uint64_t aID,
              TextureFlags aFlags);

  virtual ~TextureHost();

  


  static TemporaryRef<TextureHost> Create(uint64_t aID,
                                          const SurfaceDescriptor& aDesc,
                                          ISurfaceAllocator* aDeallocator,
                                          TextureFlags aFlags);

  


  virtual bool Lock() { return true; }

  


  virtual void Unlock() {}

  




  virtual gfx::SurfaceFormat GetFormat() const = 0;

  






  virtual NewTextureSource* GetTextureSources() = 0;

  








  virtual void Updated(const nsIntRegion* aRegion) {}

  






  virtual void SetCompositor(Compositor* aCompositor) {}

  



  virtual void DeallocateDeviceData() {}

  



  virtual void DeallocateSharedData() {}

  







  uint64_t GetID() const { return mID; }

  virtual gfx::IntSize GetSize() const = 0;

  






  TextureHost* GetNextSibling() const { return mNextTexture; }
  void SetNextSibling(TextureHost* aNext) { mNextTexture = aNext; }

  



  virtual already_AddRefed<gfxImageSurface> GetAsSurface() = 0;

  


  void SetFlags(TextureFlags aFlags) { mFlags = aFlags; }

  


  void AddFlag(TextureFlags aFlag) { mFlags |= aFlag; }

  TextureFlags GetFlags() { return mFlags; }

  



  virtual LayerRenderState GetRenderState()
  {
    
    
    return LayerRenderState();
  }

  virtual void SetCompositableQuirks(CompositableQuirks* aQuirks);

  
  
  virtual void OnActorDestroy() {}

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char *Name() { return "TextureHost"; }
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);
#endif

protected:
  uint64_t mID;
  RefPtr<TextureHost> mNextTexture;
  TextureFlags mFlags;
  RefPtr<CompositableQuirks> mQuirks;
};














class BufferTextureHost : public TextureHost
{
public:
  BufferTextureHost(uint64_t aID,
                    gfx::SurfaceFormat aFormat,
                    TextureFlags aFlags);

  ~BufferTextureHost();

  virtual uint8_t* GetBuffer() = 0;

  virtual void Updated(const nsIntRegion* aRegion) MOZ_OVERRIDE;

  virtual bool Lock() MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual NewTextureSource* GetTextureSources() MOZ_OVERRIDE;

  virtual void DeallocateDeviceData() MOZ_OVERRIDE;

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  






  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  virtual already_AddRefed<gfxImageSurface> GetAsSurface() MOZ_OVERRIDE;

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
  ShmemTextureHost(uint64_t aID,
                   const ipc::Shmem& aShmem,
                   gfx::SurfaceFormat aFormat,
                   ISurfaceAllocator* aDeallocator,
                   TextureFlags aFlags);

  ~ShmemTextureHost();

  virtual void DeallocateSharedData() MOZ_OVERRIDE;

  virtual uint8_t* GetBuffer() MOZ_OVERRIDE;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char *Name() MOZ_OVERRIDE { return "ShmemTextureHost"; }
#endif

  virtual void OnActorDestroy() MOZ_OVERRIDE;

protected:
  ipc::Shmem* mShmem;
  ISurfaceAllocator* mDeallocator;
};







class MemoryTextureHost : public BufferTextureHost
{
public:
  MemoryTextureHost(uint64_t aID,
                    uint8_t* aBuffer,
                    gfx::SurfaceFormat aFormat,
                    TextureFlags aFlags);

  ~MemoryTextureHost();

  virtual void DeallocateSharedData() MOZ_OVERRIDE;

  virtual uint8_t* GetBuffer() MOZ_OVERRIDE;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char *Name() MOZ_OVERRIDE { return "MemoryTextureHost"; }
#endif

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

  virtual already_AddRefed<gfxImageSurface> GetAsSurface() = 0;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char *Name() = 0;
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);
#endif

  





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

  





  
  
  virtual void SetBuffer(SurfaceDescriptor* aBuffer, ISurfaceAllocator* aAllocator)
  {
    MOZ_ASSERT(!mBuffer || mBuffer == aBuffer, "Will leak the old mBuffer");
    mBuffer = aBuffer;
    mDeAllocator = aAllocator;
  }

  
  
  virtual void ForgetBuffer() {}

  void OnActorDestroy();

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
                              
                              
                              
                              
  ISurfaceAllocator* mDeAllocator;
  gfx::SurfaceFormat mFormat;
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
  virtual ~CompositingRenderTarget() {}

#ifdef MOZ_DUMP_PAINTING
  virtual already_AddRefed<gfxImageSurface> Dump(Compositor* aCompositor) { return nullptr; }
#endif
};





TemporaryRef<TextureHost>
CreateBackendIndependentTextureHost(uint64_t aID,
                                    const SurfaceDescriptor& aDesc,
                                    ISurfaceAllocator* aDeallocator,
                                    TextureFlags aFlags);

}
}

#endif
