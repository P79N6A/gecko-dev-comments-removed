




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
  


  virtual TextureSourceOGL* AsSourceOGL() { return nullptr; }
  



  virtual TextureSource* GetSubSource(int index) { return nullptr; }
  



  virtual TileIterator* AsTileIterator() { return nullptr; }

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);
#endif
};

















































class TextureHost : public TextureSource
{
public:
  







  static TemporaryRef<TextureHost> CreateTextureHost(SurfaceDescriptorType aDescriptorType,
                                                     uint32_t aTextureHostFlags,
                                                     uint32_t aTextureFlags);

  TextureHost();
  virtual ~TextureHost();

  virtual gfx::SurfaceFormat GetFormat() const { return mFormat; }

  virtual bool IsValid() const { return true; }

  


  void Update(const SurfaceDescriptor& aImage,
              nsIntRegion *aRegion = nullptr);

  



  void SwapTextures(const SurfaceDescriptor& aImage,
                    SurfaceDescriptor* aResult = nullptr,
                    nsIntRegion *aRegion = nullptr);

  



  virtual void Update(gfxReusableSurfaceWrapper* aReusableSurface,
  	                  TextureFlags aFlags,
  	                  const gfx::IntSize& aSize) {}

  



  virtual bool Lock() { return true; }

  


  virtual void Unlock() {}

  void SetFlags(TextureFlags aFlags) { mFlags = aFlags; }
  void AddFlag(TextureFlags aFlag) { mFlags |= aFlag; }
  TextureFlags GetFlags() { return mFlags; }

  






  virtual void SetCompositor(Compositor* aCompositor) {}

  ISurfaceAllocator* GetDeAllocator()
  {
    return mDeAllocator;
  }

#ifdef MOZ_DUMP_PAINTING
  virtual already_AddRefed<gfxImageSurface> Dump() { return nullptr; }
#endif

  bool operator== (const TextureHost& o) const
  {
    return GetIdentifier() == o.GetIdentifier();
  }
  bool operator!= (const TextureHost& o) const
  {
    return GetIdentifier() != o.GetIdentifier();
  }

  LayerRenderState GetRenderState()
  {
    return LayerRenderState(mBuffer,
                            mFlags & NeedsYFlip ? LAYER_RENDER_STATE_Y_FLIPPED : 0);
  }

  

  void SetTextureParent(TextureParent* aParent)
  {
    MOZ_ASSERT(!mTextureParent || mTextureParent == aParent);
    mTextureParent = aParent;
  }

  TextureParent* GetIPDLActor() const
  {
    return mTextureParent;
  }

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char *Name() = 0;
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);
#endif


  SurfaceDescriptor* GetBuffer() const { return mBuffer; }
  





  void SetBuffer(SurfaceDescriptor* aBuffer, ISurfaceAllocator* aAllocator)
  {
    MOZ_ASSERT(!mBuffer, "Will leak the old mBuffer");
    mBuffer = aBuffer;
    mDeAllocator = aAllocator;
  }

protected:
  





  virtual void UpdateImpl(const SurfaceDescriptor& aImage,
                          nsIntRegion *aRegion)
  {
    NS_RUNTIMEABORT("Should not be reached");
  }

  








  virtual void SwapTexturesImpl(const SurfaceDescriptor& aImage,
                                nsIntRegion *aRegion)
  {
    UpdateImpl(aImage, aRegion);
  }

  
  
  
  virtual uint64_t GetIdentifier() const
  {
    return reinterpret_cast<uint64_t>(this);
  }

  
  TextureFlags mFlags;
  SurfaceDescriptor* mBuffer;
  gfx::SurfaceFormat mFormat;

  TextureParent* mTextureParent;
  ISurfaceAllocator* mDeAllocator;
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
