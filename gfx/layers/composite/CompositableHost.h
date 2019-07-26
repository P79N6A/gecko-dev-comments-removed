




#ifndef MOZILLA_GFX_BUFFERHOST_H
#define MOZILLA_GFX_BUFFERHOST_H

#include "mozilla/layers/Compositor.h"
#include "mozilla/layers/PCompositableParent.h"
#include "mozilla/layers/ISurfaceAllocator.h"
#include "ThebesLayerBuffer.h"
#include "ClientTiledThebesLayer.h" 
#include "mozilla/RefPtr.h"

namespace mozilla {
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















class CompositableHost : public RefCounted<CompositableHost>
{
public:
  CompositableHost(const TextureInfo& aTextureInfo);

  virtual ~CompositableHost();

  static TemporaryRef<CompositableHost> Create(const TextureInfo& aTextureInfo);

  virtual CompositableType GetType() = 0;

  
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

  virtual void Attach(Layer* aLayer, Compositor* aCompositor)
  {
    MOZ_ASSERT(aCompositor, "Compositor is required");
    MOZ_ASSERT(!IsAttached());
    SetCompositor(aCompositor);
    SetLayer(aLayer);
    mAttached = true;
  }
  void Detach()
  {
    SetLayer(nullptr);
    SetCompositor(nullptr);
    mAttached = false;
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
  void RemoveTextureHost(uint64_t aTextureID);
  TextureHost* GetTextureHost(uint64_t aTextureID);

protected:
  TextureInfo mTextureInfo;
  Compositor* mCompositor;
  Layer* mLayer;
  RefPtr<TextureHost> mFirstTexture;
  bool mAttached;
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
