




#ifndef MOZILLA_GFX_BUFFERHOST_H
#define MOZILLA_GFX_BUFFERHOST_H

#include <stdint.h>                     
#include <stdio.h>                      
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
#include "mozilla/layers/TextureHost.h" 
#include "mozilla/mozalloc.h"           
#include "nsCOMPtr.h"                   
#include "nsRegion.h"                   
#include "nscore.h"                     
#include "Units.h"                      

struct nsIntPoint;
struct nsIntRect;

namespace mozilla {
namespace gfx {
class Matrix4x4;
class DataSourceSurface;
}

namespace layers {


struct TiledLayerProperties
{
  nsIntRegion mVisibleRegion;
  nsIntRegion mValidRegion;
  CSSToScreenScale mEffectiveResolution;
};

class Layer;
class SurfaceDescriptor;
class Compositor;
class ISurfaceAllocator;
class ThebesBufferData;
class TiledLayerComposer;
struct EffectChain;




class CompositableBackendSpecificData
{
protected:
  virtual ~CompositableBackendSpecificData() { }

public:
  NS_INLINE_DECL_REFCOUNTING(CompositableBackendSpecificData)

  CompositableBackendSpecificData()
  {
  }

  virtual void SetCompositor(Compositor* aCompositor) {}
  virtual void ClearData()
  {
    mCurrentReleaseFenceTexture = nullptr;
    ClearPendingReleaseFenceTextureList();
  }

  




  void SetCurrentReleaseFenceTexture(TextureHost* aTexture)
  {
    if (mCurrentReleaseFenceTexture) {
      mPendingReleaseFenceTextures.push_back(mCurrentReleaseFenceTexture);
    }
    mCurrentReleaseFenceTexture = aTexture;
  }

  virtual std::vector< RefPtr<TextureHost> >& GetPendingReleaseFenceTextureList()
  {
    return mPendingReleaseFenceTextures;
  }

  virtual void ClearPendingReleaseFenceTextureList()
  {
    return mPendingReleaseFenceTextures.clear();
  }
protected:
  



  RefPtr<TextureHost> mCurrentReleaseFenceTexture;
  



  std::vector< RefPtr<TextureHost> > mPendingReleaseFenceTextures;
};















class CompositableHost
{
protected:
  virtual ~CompositableHost();

public:
  NS_INLINE_DECL_REFCOUNTING(CompositableHost)
  CompositableHost(const TextureInfo& aTextureInfo);

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
                         const gfx::Filter& aFilter,
                         const gfx::Rect& aClipRect,
                         const nsIntRegion* aVisibleRegion = nullptr,
                         TiledLayerProperties* aLayerProperties = nullptr) = 0;

  




  virtual bool UpdateThebes(const ThebesBufferData& aData,
                            const nsIntRegion& aUpdated,
                            const nsIntRegion& aOldValidRegionBack,
                            nsIntRegion* aUpdatedRegionBack)
  {
    NS_ERROR("should be implemented or not used");
    return false;
  }

  












  virtual void UpdateIncremental(TextureIdentifier aTextureId,
                                 SurfaceDescriptor& aSurface,
                                 const nsIntRegion& aUpdated,
                                 const nsIntRect& aBufferRect,
                                 const nsIntPoint& aBufferRotation)
  {
    MOZ_ASSERT(false, "should be implemented or not used");
  }

  








  virtual bool CreatedIncrementalTexture(ISurfaceAllocator* aAllocator,
                                         const TextureInfo& aTextureInfo,
                                         const nsIntRect& aBufferRect)
  {
    NS_ERROR("should be implemented or not used");
    return false;
  }

  


  virtual TextureHost* GetAsTextureHost() { return nullptr; }

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
  static const AttachFlags FORCE_DETACH = 2;

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
  
  
  
  
  
  
  
  
  void Detach(Layer* aLayer = nullptr, AttachFlags aFlags = NO_FLAGS)
  {
    if (!mKeepAttached ||
        aLayer == mLayer ||
        aFlags & FORCE_DETACH) {
      SetLayer(nullptr);
      mAttached = false;
      mKeepAttached = false;
      if (mBackendData) {
        mBackendData->ClearData();
      }
    }
  }
  bool IsAttached() { return mAttached; }

#ifdef MOZ_DUMP_PAINTING
  virtual void Dump(FILE* aFile=nullptr,
                    const char* aPrefix="",
                    bool aDumpHtml=false) { }
  static void DumpTextureHost(FILE* aFile, TextureHost* aTexture);

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() { return nullptr; }
#endif

  virtual void PrintInfo(nsACString& aTo, const char* aPrefix) { }

  virtual void UseTextureHost(TextureHost* aTexture);
  virtual void UseComponentAlphaTextures(TextureHost* aTextureOnBlack,
                                         TextureHost* aTextureOnWhite);

  virtual void RemoveTextureHost(TextureHost* aTexture);

  
  void BumpFlashCounter() {
    mFlashCounter = mFlashCounter >= DIAGNOSTIC_FLASH_COUNTER_MAX
                  ? DIAGNOSTIC_FLASH_COUNTER_MAX : mFlashCounter + 1;
  }
protected:
  TextureInfo mTextureInfo;
  Compositor* mCompositor;
  Layer* mLayer;
  RefPtr<CompositableBackendSpecificData> mBackendData;
  uint32_t mFlashCounter; 
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
