




#ifndef GFX_CONTENTHOST_H
#define GFX_CONTENTHOST_H

#include <stdint.h>                     
#include <stdio.h>                      
#include "mozilla-config.h"             
#include "CompositableHost.h"           
#include "RotatedBuffer.h"              
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/BasePoint.h"      
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/ISurfaceAllocator.h"  
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/layers/TextureHost.h"  
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsTArray.h"                   
#include "nscore.h"                     

namespace mozilla {
namespace gfx {
class Matrix4x4;
}
namespace layers {
class Compositor;
class ThebesBufferData;
class TiledLayerComposer;
struct EffectChain;
class TextureImageTextureSourceOGL;

struct TexturedEffect;







class ContentHost : public CompositableHost
{
public:
  
  
  virtual TiledLayerComposer* AsTiledLayerComposer() { return nullptr; }

  virtual bool UpdateThebes(const ThebesBufferData& aData,
                            const nsIntRegion& aUpdated,
                            const nsIntRegion& aOldValidRegionBack,
                            nsIntRegion* aUpdatedRegionBack) = 0;

  virtual void SetPaintWillResample(bool aResample) { }

protected:
  ContentHost(const TextureInfo& aTextureInfo)
    : CompositableHost(aTextureInfo)
  {}
};












class ContentHostBase : public ContentHost
{
public:
  typedef RotatedContentBuffer::ContentType ContentType;
  typedef RotatedContentBuffer::PaintState PaintState;

  ContentHostBase(const TextureInfo& aTextureInfo);
  virtual ~ContentHostBase();

  virtual void Composite(EffectChain& aEffectChain,
                         float aOpacity,
                         const gfx::Matrix4x4& aTransform,
                         const gfx::Filter& aFilter,
                         const gfx::Rect& aClipRect,
                         const nsIntRegion* aVisibleRegion = nullptr,
                         TiledLayerProperties* aLayerProperties = nullptr);

  virtual void SetPaintWillResample(bool aResample) { mPaintWillResample = aResample; }

  virtual bool Lock() = 0;
  virtual void Unlock() = 0;

  virtual NewTextureSource* GetTextureSource() = 0;
  virtual NewTextureSource* GetTextureSourceOnWhite() = 0;

protected:
  virtual nsIntPoint GetOriginOffset()
  {
    return mBufferRect.TopLeft() - mBufferRotation;
  }

  bool PaintWillResample() { return mPaintWillResample; }

  nsIntRect mBufferRect;
  nsIntPoint mBufferRotation;
  bool mPaintWillResample;
  bool mInitialised;
};





class ContentHostTexture : public ContentHostBase
{
public:
  ContentHostTexture(const TextureInfo& aTextureInfo)
    : ContentHostBase(aTextureInfo)
    , mLocked(false)
  { }

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

#ifdef MOZ_DUMP_PAINTING
  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() MOZ_OVERRIDE;

  virtual void Dump(FILE* aFile=nullptr,
                    const char* aPrefix="",
                    bool aDumpHtml=false) MOZ_OVERRIDE;
#endif

  virtual void PrintInfo(nsACString& aTo, const char* aPrefix) MOZ_OVERRIDE;

  virtual void UseTextureHost(TextureHost* aTexture) MOZ_OVERRIDE;
  virtual void UseComponentAlphaTextures(TextureHost* aTextureOnBlack,
                                         TextureHost* aTextureOnWhite) MOZ_OVERRIDE;

  virtual bool Lock() {
    MOZ_ASSERT(!mLocked);
    if (!mTextureHost->Lock()) {
      return false;
    }

    if (mTextureHostOnWhite && !mTextureHostOnWhite->Lock()) {
      return false;
    }

    mLocked = true;
    return true;
  }
  virtual void Unlock() {
    MOZ_ASSERT(mLocked);
    mTextureHost->Unlock();
    if (mTextureHostOnWhite) {
      mTextureHostOnWhite->Unlock();
    }
    mLocked = false;
  }

  virtual NewTextureSource* GetTextureSource() {
    MOZ_ASSERT(mLocked);
    return mTextureHost->GetTextureSources();
  }
  virtual NewTextureSource* GetTextureSourceOnWhite() {
    MOZ_ASSERT(mLocked);
    if (mTextureHostOnWhite) {
      return mTextureHostOnWhite->GetTextureSources();
    }
    return nullptr;
  }

  LayerRenderState GetRenderState();

protected:
  RefPtr<TextureHost> mTextureHost;
  RefPtr<TextureHost> mTextureHostOnWhite;
  bool mLocked;
};

class DeprecatedContentHostBase : public ContentHost
{
public:
  typedef RotatedContentBuffer::ContentType ContentType;
  typedef RotatedContentBuffer::PaintState PaintState;

  DeprecatedContentHostBase(const TextureInfo& aTextureInfo);
  ~DeprecatedContentHostBase();

  virtual void Composite(EffectChain& aEffectChain,
                         float aOpacity,
                         const gfx::Matrix4x4& aTransform,
                         const gfx::Filter& aFilter,
                         const gfx::Rect& aClipRect,
                         const nsIntRegion* aVisibleRegion = nullptr,
                         TiledLayerProperties* aLayerProperties = nullptr);

  virtual PaintState BeginPaint(ContentType, uint32_t)
  {
    NS_RUNTIMEABORT("shouldn't BeginPaint for a shadow layer");
    return PaintState();
  }

  virtual LayerRenderState GetRenderState() MOZ_OVERRIDE;

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

#ifdef MOZ_DUMP_PAINTING
  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface();

  virtual void Dump(FILE* aFile=nullptr,
                    const char* aPrefix="",
                    bool aDumpHtml=false) MOZ_OVERRIDE;
#endif

  virtual DeprecatedTextureHost* GetDeprecatedTextureHost() MOZ_OVERRIDE;

  virtual void SetPaintWillResample(bool aResample) { mPaintWillResample = aResample; }
  
  
  
  virtual void DestroyTextures() = 0;

protected:
  virtual nsIntPoint GetOriginOffset()
  {
    return mBufferRect.TopLeft() - mBufferRotation;
  }

  bool PaintWillResample() { return mPaintWillResample; }

  
  
  void DestroyFrontHost();

  nsIntRect mBufferRect;
  nsIntPoint mBufferRotation;
  RefPtr<DeprecatedTextureHost> mDeprecatedTextureHost;
  RefPtr<DeprecatedTextureHost> mDeprecatedTextureHostOnWhite;
  
  
  
  RefPtr<DeprecatedTextureHost> mNewFrontHost;
  RefPtr<DeprecatedTextureHost> mNewFrontHostOnWhite;
  bool mPaintWillResample;
  bool mInitialised;
};






class ContentHostDoubleBuffered : public ContentHostTexture
{
public:
  ContentHostDoubleBuffered(const TextureInfo& aTextureInfo)
    : ContentHostTexture(aTextureInfo)
  {}

  virtual ~ContentHostDoubleBuffered() {}

  virtual CompositableType GetType() { return COMPOSITABLE_CONTENT_DOUBLE; }

  virtual bool UpdateThebes(const ThebesBufferData& aData,
                            const nsIntRegion& aUpdated,
                            const nsIntRegion& aOldValidRegionBack,
                            nsIntRegion* aUpdatedRegionBack);

protected:
  nsIntRegion mValidRegionForNextBackBuffer;
};





class ContentHostSingleBuffered : public ContentHostTexture
{
public:
  ContentHostSingleBuffered(const TextureInfo& aTextureInfo)
    : ContentHostTexture(aTextureInfo)
  {}
  virtual ~ContentHostSingleBuffered() {}

  virtual CompositableType GetType() { return COMPOSITABLE_CONTENT_SINGLE; }

  virtual bool UpdateThebes(const ThebesBufferData& aData,
                            const nsIntRegion& aUpdated,
                            const nsIntRegion& aOldValidRegionBack,
                            nsIntRegion* aUpdatedRegionBack);
};











class ContentHostIncremental : public ContentHostBase
{
public:
  ContentHostIncremental(const TextureInfo& aTextureInfo)
    : ContentHostBase(aTextureInfo)
    , mDeAllocator(nullptr)
    , mLocked(false)
  {}

  virtual CompositableType GetType() { return BUFFER_CONTENT_INC; }

  virtual LayerRenderState GetRenderState() MOZ_OVERRIDE { return LayerRenderState(); }

  virtual void CreatedIncrementalTexture(ISurfaceAllocator* aAllocator,
                                         const TextureInfo& aTextureInfo,
                                         const nsIntRect& aBufferRect) MOZ_OVERRIDE;

  virtual void UpdateIncremental(TextureIdentifier aTextureId,
                                 SurfaceDescriptor& aSurface,
                                 const nsIntRegion& aUpdated,
                                 const nsIntRect& aBufferRect,
                                 const nsIntPoint& aBufferRotation) MOZ_OVERRIDE;

  virtual bool UpdateThebes(const ThebesBufferData& aData,
                            const nsIntRegion& aUpdated,
                            const nsIntRegion& aOldValidRegionBack,
                            nsIntRegion* aUpdatedRegionBack)
  {
    NS_ERROR("Shouldn't call this");
    return false;
  }

  virtual void DestroyTextures()
  {
    mSource = nullptr;
    mSourceOnWhite = nullptr;
    mUpdateList.Clear();
  }

  virtual bool Lock() {
    MOZ_ASSERT(!mLocked);
    ProcessTextureUpdates();
    mLocked = true;
    return true;
  }

  virtual void Unlock() {
    MOZ_ASSERT(mLocked);
    mLocked = false;
  }

  virtual NewTextureSource* GetTextureSource();
  virtual NewTextureSource* GetTextureSourceOnWhite();

private:

  void FlushUpdateQueue();
  void ProcessTextureUpdates();

  class Request
  {
  public:
    Request()
    {
      MOZ_COUNT_CTOR(ContentHostIncremental::Request);
    }

    virtual ~Request()
    {
      MOZ_COUNT_DTOR(ContentHostIncremental::Request);
    }

    virtual void Execute(ContentHostIncremental *aHost) = 0;
  };

  class TextureCreationRequest : public Request
  {
  public:
    TextureCreationRequest(const TextureInfo& aTextureInfo,
                           const nsIntRect& aBufferRect)
      : mTextureInfo(aTextureInfo)
      , mBufferRect(aBufferRect)
    {}

    virtual void Execute(ContentHostIncremental *aHost);

  private:
    TextureInfo mTextureInfo;
    nsIntRect mBufferRect;
  };

  class TextureUpdateRequest : public Request
  {
  public:
    TextureUpdateRequest(ISurfaceAllocator* aDeAllocator,
                         TextureIdentifier aTextureId,
                         SurfaceDescriptor& aDescriptor,
                         const nsIntRegion& aUpdated,
                         const nsIntRect& aBufferRect,
                         const nsIntPoint& aBufferRotation)
      : mDeAllocator(aDeAllocator)
      , mTextureId(aTextureId)
      , mDescriptor(aDescriptor)
      , mUpdated(aUpdated)
      , mBufferRect(aBufferRect)
      , mBufferRotation(aBufferRotation)
    {}

    ~TextureUpdateRequest()
    {
      
      mDeAllocator->DestroySharedSurface(&mDescriptor);
    }

    virtual void Execute(ContentHostIncremental *aHost);

  private:
    enum XSide {
      LEFT, RIGHT
    };
    enum YSide {
      TOP, BOTTOM
    };

    nsIntRect GetQuadrantRectangle(XSide aXSide, YSide aYSide) const;

    RefPtr<ISurfaceAllocator> mDeAllocator;
    TextureIdentifier mTextureId;
    SurfaceDescriptor mDescriptor;
    nsIntRegion mUpdated;
    nsIntRect mBufferRect;
    nsIntPoint mBufferRotation;
  };

  nsTArray<nsAutoPtr<Request> > mUpdateList;

  
  
  RefPtr<TextureImageTextureSourceOGL> mSource;
  RefPtr<TextureImageTextureSourceOGL> mSourceOnWhite;

  RefPtr<ISurfaceAllocator> mDeAllocator;
  bool mLocked;
};

}
}

#endif
