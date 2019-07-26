




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
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsTArray.h"                   
#include "nsTraceRefcnt.h"              
#include "nscore.h"                     

class gfxImageSurface;

namespace mozilla {
namespace gfx {
class Matrix4x4;
}
namespace layers {
class Compositor;
class ThebesBufferData;
class TiledLayerComposer;
struct EffectChain;

struct TexturedEffect;







class ContentHost : public CompositableHost
{
public:
  
  
  virtual TiledLayerComposer* AsTiledLayerComposer() { return nullptr; }

  virtual void UpdateThebes(const ThebesBufferData& aData,
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

  virtual LayerRenderState GetRenderState() MOZ_OVERRIDE;

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

#ifdef MOZ_DUMP_PAINTING
  virtual already_AddRefed<gfxImageSurface> GetAsSurface() MOZ_OVERRIDE;

  virtual void Dump(FILE* aFile=nullptr,
                    const char* aPrefix="",
                    bool aDumpHtml=false) MOZ_OVERRIDE;
#endif

  virtual void PrintInfo(nsACString& aTo, const char* aPrefix) MOZ_OVERRIDE;

  virtual TextureHost* GetAsTextureHost() MOZ_OVERRIDE;

  virtual void UseTextureHost(TextureHost* aTexture) MOZ_OVERRIDE;

  virtual void RemoveTextureHost(TextureHost* aTexture) MOZ_OVERRIDE;

  virtual void SetPaintWillResample(bool aResample) { mPaintWillResample = aResample; }

  virtual void OnActorDestroy() MOZ_OVERRIDE;

protected:
  virtual nsIntPoint GetOriginOffset()
  {
    return mBufferRect.TopLeft() - mBufferRotation;
  }

  bool PaintWillResample() { return mPaintWillResample; }

  
  void DestroyTextureHost();
  void DestroyTextureHostOnWhite();

  nsIntRect mBufferRect;
  nsIntPoint mBufferRotation;
  RefPtr<TextureHost> mTextureHost;
  RefPtr<TextureHost> mTextureHostOnWhite;
  bool mPaintWillResample;
  bool mInitialised;
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
  virtual already_AddRefed<gfxImageSurface> GetAsSurface();

  virtual void Dump(FILE* aFile=nullptr,
                    const char* aPrefix="",
                    bool aDumpHtml=false) MOZ_OVERRIDE;
#endif

  virtual DeprecatedTextureHost* GetDeprecatedTextureHost() MOZ_OVERRIDE;

  virtual void SetPaintWillResample(bool aResample) { mPaintWillResample = aResample; }
  
  
  
  virtual void DestroyTextures() = 0;

  virtual void OnActorDestroy() MOZ_OVERRIDE;

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






class ContentHostDoubleBuffered : public ContentHostBase
{
public:
  ContentHostDoubleBuffered(const TextureInfo& aTextureInfo)
    : ContentHostBase(aTextureInfo)
  {}

  virtual ~ContentHostDoubleBuffered() {}

  virtual CompositableType GetType() { return COMPOSITABLE_CONTENT_DOUBLE; }

  virtual void UpdateThebes(const ThebesBufferData& aData,
                            const nsIntRegion& aUpdated,
                            const nsIntRegion& aOldValidRegionBack,
                            nsIntRegion* aUpdatedRegionBack);

protected:
  nsIntRegion mValidRegionForNextBackBuffer;
};

class DeprecatedContentHostDoubleBuffered : public DeprecatedContentHostBase
{
public:
  DeprecatedContentHostDoubleBuffered(const TextureInfo& aTextureInfo)
    : DeprecatedContentHostBase(aTextureInfo)
  {}

  ~DeprecatedContentHostDoubleBuffered();

  virtual CompositableType GetType() { return BUFFER_CONTENT_DIRECT; }

  virtual void UpdateThebes(const ThebesBufferData& aData,
                            const nsIntRegion& aUpdated,
                            const nsIntRegion& aOldValidRegionBack,
                            nsIntRegion* aUpdatedRegionBack);

  virtual void EnsureDeprecatedTextureHost(TextureIdentifier aTextureId,
                                 const SurfaceDescriptor& aSurface,
                                 ISurfaceAllocator* aAllocator,
                                 const TextureInfo& aTextureInfo) MOZ_OVERRIDE;
  virtual void DestroyTextures() MOZ_OVERRIDE;

  virtual void OnActorDestroy() MOZ_OVERRIDE;

#ifdef MOZ_DUMP_PAINTING
  virtual void Dump(FILE* aFile=nullptr,
                    const char* aPrefix="",
                    bool aDumpHtml=false) MOZ_OVERRIDE;
#endif

  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);
protected:
  nsIntRegion mValidRegionForNextBackBuffer;
  
  
  
  RefPtr<DeprecatedTextureHost> mBackHost;
  RefPtr<DeprecatedTextureHost> mBackHostOnWhite;
};





class ContentHostSingleBuffered : public ContentHostBase
{
public:
  ContentHostSingleBuffered(const TextureInfo& aTextureInfo)
    : ContentHostBase(aTextureInfo)
  {}
  virtual ~ContentHostSingleBuffered() {}

  virtual CompositableType GetType() { return COMPOSITABLE_CONTENT_SINGLE; }

  virtual void UpdateThebes(const ThebesBufferData& aData,
                            const nsIntRegion& aUpdated,
                            const nsIntRegion& aOldValidRegionBack,
                            nsIntRegion* aUpdatedRegionBack);
};

class DeprecatedContentHostSingleBuffered : public DeprecatedContentHostBase
{
public:
  DeprecatedContentHostSingleBuffered(const TextureInfo& aTextureInfo)
    : DeprecatedContentHostBase(aTextureInfo)
  {}
  virtual ~DeprecatedContentHostSingleBuffered();

  virtual CompositableType GetType() { return BUFFER_CONTENT; }

  virtual void UpdateThebes(const ThebesBufferData& aData,
                            const nsIntRegion& aUpdated,
                            const nsIntRegion& aOldValidRegionBack,
                            nsIntRegion* aUpdatedRegionBack);

  virtual void EnsureDeprecatedTextureHost(TextureIdentifier aTextureId,
                                 const SurfaceDescriptor& aSurface,
                                 ISurfaceAllocator* aAllocator,
                                 const TextureInfo& aTextureInfo) MOZ_OVERRIDE;
  virtual void DestroyTextures() MOZ_OVERRIDE;

  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);
};











class ContentHostIncremental : public DeprecatedContentHostBase
{
public:
  ContentHostIncremental(const TextureInfo& aTextureInfo)
    : DeprecatedContentHostBase(aTextureInfo)
    , mDeAllocator(nullptr)
  {}

  virtual CompositableType GetType() { return BUFFER_CONTENT; }

  virtual void EnsureDeprecatedTextureHostIncremental(ISurfaceAllocator* aAllocator,
                                            const TextureInfo& aTextureInfo,
                                            const nsIntRect& aBufferRect) MOZ_OVERRIDE;

  virtual void EnsureDeprecatedTextureHost(TextureIdentifier aTextureId,
                                 const SurfaceDescriptor& aSurface,
                                 ISurfaceAllocator* aAllocator,
                                 const TextureInfo& aTextureInfo)
  {
    NS_RUNTIMEABORT("Shouldn't call this");
  }

  virtual void UpdateIncremental(TextureIdentifier aTextureId,
                                 SurfaceDescriptor& aSurface,
                                 const nsIntRegion& aUpdated,
                                 const nsIntRect& aBufferRect,
                                 const nsIntPoint& aBufferRotation) MOZ_OVERRIDE;

  virtual void UpdateThebes(const ThebesBufferData& aData,
                            const nsIntRegion& aUpdated,
                            const nsIntRegion& aOldValidRegionBack,
                            nsIntRegion* aUpdatedRegionBack)
  {
    NS_RUNTIMEABORT("Shouldn't call this");
  }

  virtual void Composite(EffectChain& aEffectChain,
                         float aOpacity,
                         const gfx::Matrix4x4& aTransform,
                         const gfx::Filter& aFilter,
                         const gfx::Rect& aClipRect,
                         const nsIntRegion* aVisibleRegion = nullptr,
                         TiledLayerProperties* aLayerProperties = nullptr)
  {
    ProcessTextureUpdates();

    DeprecatedContentHostBase::Composite(aEffectChain, aOpacity,
                               aTransform, aFilter,
                               aClipRect, aVisibleRegion,
                               aLayerProperties);
  }

  virtual void DestroyTextures()
  {
    mDeprecatedTextureHost = nullptr;
    mDeprecatedTextureHostOnWhite = nullptr;
    mUpdateList.Clear();
  }

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

    ISurfaceAllocator* mDeAllocator;
    TextureIdentifier mTextureId;
    SurfaceDescriptor mDescriptor;
    nsIntRegion mUpdated;
    nsIntRect mBufferRect;
    nsIntPoint mBufferRotation;
  };

  nsTArray<nsAutoPtr<Request> > mUpdateList;

  ISurfaceAllocator* mDeAllocator;
};

}
}

#endif
