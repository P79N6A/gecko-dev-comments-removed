




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
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/Effects.h"     
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/layers/LayersMessages.h"
#include "mozilla/layers/TextureHost.h" 
#include "mozilla/mozalloc.h"           
#include "nsCOMPtr.h"                   
#include "nsRegion.h"                   
#include "nscore.h"                     
#include "Units.h"                      

namespace mozilla {
namespace gfx {
class Matrix4x4;
class DataSourceSurface;
}

namespace layers {

class Layer;
class Compositor;
class ThebesBufferData;
class TiledLayerComposer;
class CompositableParentManager;
class PCompositableParent;
struct EffectChain;















class CompositableHost
{
protected:
  virtual ~CompositableHost();

public:
  NS_INLINE_DECL_REFCOUNTING(CompositableHost)
  explicit CompositableHost(const TextureInfo& aTextureInfo);

  static TemporaryRef<CompositableHost> Create(const TextureInfo& aTextureInfo);

  virtual CompositableType GetType() = 0;

  
  virtual void SetCompositor(Compositor* aCompositor);

  
  virtual void Composite(EffectChain& aEffectChain,
                         float aOpacity,
                         const gfx::Matrix4x4& aTransform,
                         const gfx::Filter& aFilter,
                         const gfx::Rect& aClipRect,
                         const nsIntRegion* aVisibleRegion = nullptr) = 0;

  




  virtual bool UpdateThebes(const ThebesBufferData& aData,
                            const nsIntRegion& aUpdated,
                            const nsIntRegion& aOldValidRegionBack,
                            nsIntRegion* aUpdatedRegionBack)
  {
    NS_ERROR("should be implemented or not used");
    return false;
  }

  


  virtual TextureHost* GetAsTextureHost() { return nullptr; }

  virtual LayerRenderState GetRenderState() = 0;

  virtual void SetPictureRect(const gfx::IntRect& aPictureRect)
  {
    MOZ_ASSERT(false, "Should have been overridden");
  }

  virtual gfx::IntSize GetImageSize() const
  {
    MOZ_ASSERT(false, "Should have been overridden");
    return gfx::IntSize();
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

    
    RefPtr<TextureHost> frontBuffer = GetAsTextureHost();
    if (frontBuffer) {
      UseTextureHost(frontBuffer);
    }
  }
  
  
  
  
  
  
  
  
  virtual void Detach(Layer* aLayer = nullptr, AttachFlags aFlags = NO_FLAGS)
  {
    if (!mKeepAttached ||
        aLayer == mLayer ||
        aFlags & FORCE_DETACH) {
      SetLayer(nullptr);
      mAttached = false;
      mKeepAttached = false;
    }
  }
  bool IsAttached() { return mAttached; }

  virtual void Dump(std::stringstream& aStream,
                    const char* aPrefix="",
                    bool aDumpHtml=false) { }
  static void DumpTextureHost(std::stringstream& aStream, TextureHost* aTexture);

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() { return nullptr; }

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix) = 0;

  virtual void UseTextureHost(TextureHost* aTexture);
  virtual void UseComponentAlphaTextures(TextureHost* aTextureOnBlack,
                                         TextureHost* aTextureOnWhite);
  virtual void UseOverlaySource(OverlaySource aOverlay) { }

  virtual void RemoveTextureHost(TextureHost* aTexture);

  
  void BumpFlashCounter() {
    mFlashCounter = mFlashCounter >= DIAGNOSTIC_FLASH_COUNTER_MAX
                  ? DIAGNOSTIC_FLASH_COUNTER_MAX : mFlashCounter + 1;
  }

  static PCompositableParent*
  CreateIPDLActor(CompositableParentManager* mgr,
                  const TextureInfo& textureInfo,
                  uint64_t asyncID);

  static bool DestroyIPDLActor(PCompositableParent* actor);

  static CompositableHost* FromIPDLActor(PCompositableParent* actor);

  uint64_t GetCompositorID() const { return mCompositorID; }

  uint64_t GetAsyncID() const { return mAsyncID; }

  void SetCompositorID(uint64_t aID) { mCompositorID = aID; }

  void SetAsyncID(uint64_t aID) { mAsyncID = aID; }

  virtual bool Lock() { return false; }

  virtual void Unlock() { }

  virtual TemporaryRef<TexturedEffect> GenEffect(const gfx::Filter& aFilter) {
    return nullptr;
  }

protected:
  TextureInfo mTextureInfo;
  uint64_t mAsyncID;
  uint64_t mCompositorID;
  RefPtr<Compositor> mCompositor;
  Layer* mLayer;
  uint32_t mFlashCounter; 
  bool mAttached;
  bool mKeepAttached;
};

class AutoLockCompositableHost final
{
public:
  explicit AutoLockCompositableHost(CompositableHost* aHost)
    : mHost(aHost)
  {
    mSucceeded = (mHost && mHost->Lock());
  }

  ~AutoLockCompositableHost()
  {
    if (mSucceeded && mHost) {
      mHost->Unlock();
    }
  }

  bool Failed() const { return !mSucceeded; }

private:
  RefPtr<CompositableHost> mHost;
  bool mSucceeded;
};




























namespace CompositableMap {
  void Create();
  void Destroy();
  PCompositableParent* Get(uint64_t aID);
  void Set(uint64_t aID, PCompositableParent* aParent);
  void Erase(uint64_t aID);
  void Clear();
} 


} 
} 

#endif
