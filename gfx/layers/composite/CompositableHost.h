




#ifndef MOZILLA_GFX_BUFFERHOST_H
#define MOZILLA_GFX_BUFFERHOST_H

#include <stdint.h>                     
#include <stdio.h>                      
#include "gfxPoint.h"                   
#include "gfxRect.h"                    
#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"          
#include "mozilla/ipc/ProtocolUtils.h"
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/layers/PCompositableParent.h"
#include "mozilla/mozalloc.h"           
#include "nsCOMPtr.h"                   
#include "nsRegion.h"                   
#include "nscore.h"                     

class gfxImageSurface;
struct nsIntPoint;
struct nsIntRect;

namespace mozilla {
namespace gfx {
class Matrix4x4;
}

namespace layers {


struct TiledLayerProperties
{
  nsIntRegion mVisibleRegion;
  nsIntRegion mValidRegion;
  gfxRect mDisplayPort;
  gfxSize mEffectiveResolution;
  gfxRect mCompositionBounds;
  bool mRetainTiles;
};

class Layer;
class DeprecatedTextureHost;
class TextureHost;
class SurfaceDescriptor;
class Compositor;
class ISurfaceAllocator;
class ThebesBufferData;
class TiledLayerComposer;
struct EffectChain;




class CompositableBackendSpecificData : public RefCounted<CompositableBackendSpecificData>
{
public:
  CompositableBackendSpecificData()
  {
    MOZ_COUNT_CTOR(CompositableBackendSpecificData);
  }
  virtual ~CompositableBackendSpecificData()
  {
    MOZ_COUNT_DTOR(CompositableBackendSpecificData);
  }
  virtual void SetCompositor(Compositor* aCompositor) {}
  virtual void ClearData() {}
};















class CompositableHost : public RefCounted<CompositableHost>
{
public:
  CompositableHost(const TextureInfo& aTextureInfo);

  virtual ~CompositableHost();

  static TemporaryRef<CompositableHost> Create(const TextureInfo& aTextureInfo);

  virtual CompositableType GetType() = 0;

  virtual CompositableBackendSpecificData* GetCompositableBackendSpecificData()
  {
    return mBackendData;
  }

  virtual void SetCompositableBackendSpecificData(CompositableBackendSpecificData* aBackendData)
  {
    mBackendData = aBackendData;
  }

  
  virtual void SetCompositor(Compositor* aCompositor);

  
  virtual void Composite(EffectChain& aEffectChain,
                         float aOpacity,
                         const gfx::Matrix4x4& aTransform,
                         const gfx::Point& aOffset,
                         const gfx::Filter& aFilter,
                         const gfx::Rect& aClipRect,
                         const nsIntRegion* aVisibleRegion = nullptr,
                         TiledLayerProperties* aLayerProperties = nullptr) = 0;

  


  virtual bool Update(const SurfaceDescriptor& aImage,
                      SurfaceDescriptor* aResult = nullptr);

  




  virtual void UpdateThebes(const ThebesBufferData& aData,
                            const nsIntRegion& aUpdated,
                            const nsIntRegion& aOldValidRegionBack,
                            nsIntRegion* aUpdatedRegionBack)
  {
    MOZ_ASSERT(false, "should be implemented or not used");
  }

  












  virtual void UpdateIncremental(TextureIdentifier aTextureId,
                                 SurfaceDescriptor& aSurface,
                                 const nsIntRegion& aUpdated,
                                 const nsIntRect& aBufferRect,
                                 const nsIntPoint& aBufferRotation)
  {
    MOZ_ASSERT(false, "should be implemented or not used");
  }

  














  virtual void EnsureDeprecatedTextureHost(TextureIdentifier aTextureId,
                                 const SurfaceDescriptor& aSurface,
                                 ISurfaceAllocator* aAllocator,
                                 const TextureInfo& aTextureInfo)
  {
    MOZ_ASSERT(false, "should be implemented or not used");
  }

  








  virtual void EnsureDeprecatedTextureHostIncremental(ISurfaceAllocator* aAllocator,
                                            const TextureInfo& aTextureInfo,
                                            const nsIntRect& aBufferRect)
  {
    MOZ_ASSERT(false, "should be implemented or not used");
  }

  virtual DeprecatedTextureHost* GetDeprecatedTextureHost() { return nullptr; }

  


  virtual TextureHost* GetTextureHost() { return nullptr; }

  virtual LayerRenderState GetRenderState() = 0;

  virtual void SetPictureRect(const nsIntRect& aPictureRect)
  {
    MOZ_ASSERT(false, "Should have been overridden");
  }

  



  bool AddMaskEffect(EffectChain& aEffects,
                     const gfx::Matrix4x4& aTransform,
                     bool aIs3D = false);

  void RemoveMaskEffect();

  Compositor* GetCompositor() const
  {
    return mCompositor;
  }

  Layer* GetLayer() const { return mLayer; }
  void SetLayer(Layer* aLayer) { mLayer = aLayer; }

  virtual TiledLayerComposer* AsTiledLayerComposer() { return nullptr; }

  typedef uint32_t AttachFlags;
  static const AttachFlags NO_FLAGS = 0;
  static const AttachFlags ALLOW_REATTACH = 1;
  static const AttachFlags KEEP_ATTACHED = 2;

  virtual void Attach(Layer* aLayer,
                      Compositor* aCompositor,
                      AttachFlags aFlags = NO_FLAGS)
  {
    MOZ_ASSERT(aCompositor, "Compositor is required");
    NS_ASSERTION(aFlags & ALLOW_REATTACH || !mAttached,
                 "Re-attaching compositables must be explicitly authorised");
    SetCompositor(aCompositor);
    SetLayer(aLayer);
    mAttached = true;
    mKeepAttached = aFlags & KEEP_ATTACHED;
  }
  
  
  
  
  
  
  
  void Detach(Layer* aLayer = nullptr)
  {
    if (!mKeepAttached ||
        aLayer == mLayer) {
      SetLayer(nullptr);
      SetCompositor(nullptr);
      mAttached = false;
      mKeepAttached = false;
    }
  }
  bool IsAttached() { return mAttached; }

#ifdef MOZ_DUMP_PAINTING
  virtual void Dump(FILE* aFile=nullptr,
                    const char* aPrefix="",
                    bool aDumpHtml=false) { }
  static void DumpDeprecatedTextureHost(FILE* aFile, DeprecatedTextureHost* aTexture);
  static void DumpTextureHost(FILE* aFile, TextureHost* aTexture);

  virtual already_AddRefed<gfxImageSurface> GetAsSurface() { return nullptr; }
#endif

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix) { }
#endif

  void AddTextureHost(TextureHost* aTexture);
  virtual void UseTextureHost(TextureHost* aTexture) {}
  virtual void RemoveTextureHost(uint64_t aTextureID);
  TextureHost* GetTextureHost(uint64_t aTextureID);

protected:
  TextureInfo mTextureInfo;
  Compositor* mCompositor;
  Layer* mLayer;
  RefPtr<CompositableBackendSpecificData> mBackendData;
  RefPtr<TextureHost> mFirstTexture;
  bool mAttached;
  bool mKeepAttached;
};

class CompositableParentManager;









class CompositableParent : public PCompositableParent
{
public:
  CompositableParent(CompositableParentManager* aMgr,
                     const TextureInfo& aTextureInfo,
                     uint64_t aID = 0);
  ~CompositableParent();

  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

  CompositableHost* GetCompositableHost() const
  {
    return mHost;
  }

  void SetCompositableHost(CompositableHost* aHost)
  {
    mHost = aHost;
  }

  CompositableType GetType() const
  {
    return mType;
  }

  CompositableParentManager* GetCompositableManager() const
  {
    return mManager;
  }

  void SetCompositorID(uint64_t aCompositorID)
  {
    mCompositorID = aCompositorID;
  }

  uint64_t GetCompositorID() const
  {
    return mCompositorID;
  }

private:
  RefPtr<CompositableHost> mHost;
  CompositableParentManager* mManager;
  CompositableType mType;
  uint64_t mID;
  uint64_t mCompositorID;
};





























namespace CompositableMap {
  void Create();
  void Destroy();
  CompositableParent* Get(uint64_t aID);
  void Set(uint64_t aID, CompositableParent* aParent);
  void Erase(uint64_t aID);
  void Clear();
} 


} 
} 

#endif
