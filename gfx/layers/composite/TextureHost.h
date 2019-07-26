




#ifndef MOZILLA_GFX_TEXTUREHOST_H
#define MOZILLA_GFX_TEXTUREHOST_H

#include "mozilla/layers/LayersTypes.h"
#include "nsRect.h"
#include "nsRegion.h"
#include "mozilla/gfx/Rect.h"
#include "mozilla/layers/CompositorTypes.h"
#include "nsAutoPtr.h"
#include "mozilla/RefPtr.h"
#include "mozilla/layers/ISurfaceAllocator.h"

class gfxReusableSurfaceWrapper;
class gfxImageSurface;

namespace mozilla {
namespace layers {

class Compositor;
class SurfaceDescriptor;
class ISurfaceAllocator;
class TextureSourceOGL;
class TextureSourceD3D11;
class TextureSourceBasic;
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
  TextureSource()
  {
    MOZ_COUNT_CTOR(TextureSource);
  }
  virtual ~TextureSource()
  {
    MOZ_COUNT_DTOR(TextureSource);
  }

  




  virtual gfx::IntSize GetSize() const = 0;

  virtual gfx::SurfaceFormat GetFormat() const { return gfx::FORMAT_UNKNOWN; }

  


  virtual TextureSourceOGL* AsSourceOGL() { return nullptr; }

  


  virtual TextureSourceD3D11* AsSourceD3D11() { return nullptr; }

  virtual TextureSourceBasic* AsSourceBasic() { return nullptr; }

  



  virtual TextureSource* GetSubSource(int index) { return nullptr; }
  



  virtual TileIterator* AsTileIterator() { return nullptr; }

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);
#endif
};



















































class DeprecatedTextureHost : public TextureSource
{
public:
  







  static TemporaryRef<DeprecatedTextureHost> CreateDeprecatedTextureHost(SurfaceDescriptorType aDescriptorType,
                                                     uint32_t aDeprecatedTextureHostFlags,
                                                     uint32_t aTextureFlags);

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

  





  virtual void EnsureBuffer(const nsIntSize& aSize, gfxASurface::gfxContentType aType)
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

  





  
  
  virtual void SetBuffer(SurfaceDescriptor* aBuffer, ISurfaceAllocator* aAllocator)
  {
    MOZ_ASSERT(!mBuffer, "Will leak the old mBuffer");
    mBuffer = aBuffer;
    mDeAllocator = aAllocator;
  }

  
  
  virtual void ForgetBuffer() {}

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
                              
                              
                              
                              
  gfx::SurfaceFormat mFormat;

  ISurfaceAllocator* mDeAllocator;
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

}
}
#endif
