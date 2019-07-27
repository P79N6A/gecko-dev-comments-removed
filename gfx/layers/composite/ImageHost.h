




#ifndef MOZILLA_GFX_IMAGEHOST_H
#define MOZILLA_GFX_IMAGEHOST_H

#include <stdio.h>                      
#include "CompositableHost.h"           
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/layers/TextureHost.h"  
#include "mozilla/mozalloc.h"           
#include "nsCOMPtr.h"                   
#include "nsRect.h"                     
#include "nscore.h"                     

class nsIntRegion;

namespace mozilla {
namespace gfx {
class Matrix4x4;
}
namespace layers {

class Compositor;
class ISurfaceAllocator;
struct EffectChain;




class ImageHost : public CompositableHost
{
public:
  explicit ImageHost(const TextureInfo& aTextureInfo);
  ~ImageHost();

  virtual CompositableType GetType() { return mTextureInfo.mCompositableType; }

  virtual void Composite(EffectChain& aEffectChain,
                         float aOpacity,
                         const gfx::Matrix4x4& aTransform,
                         const gfx::Filter& aFilter,
                         const gfx::Rect& aClipRect,
                         const nsIntRegion* aVisibleRegion = nullptr) MOZ_OVERRIDE;

  virtual void UseTextureHost(TextureHost* aTexture) MOZ_OVERRIDE;

  virtual void RemoveTextureHost(TextureHost* aTexture) MOZ_OVERRIDE;

  virtual TextureHost* GetAsTextureHost() MOZ_OVERRIDE;

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual void SetPictureRect(const nsIntRect& aPictureRect) MOZ_OVERRIDE
  {
    mPictureRect = aPictureRect;
    mHasPictureRect = true;
  }

  virtual LayerRenderState GetRenderState() MOZ_OVERRIDE;

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix);

#ifdef MOZ_DUMP_PAINTING
  virtual void Dump(std::stringstream& aStream,
                    const char* aPrefix = "",
                    bool aDumpHtml = false) MOZ_OVERRIDE;

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() MOZ_OVERRIDE;
#endif

  virtual bool Lock() MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual TemporaryRef<NewTextureSource> GetTextureSource();

  virtual TemporaryRef<TexturedEffect> GenEffect(const gfx::Filter& aFilter) MOZ_OVERRIDE;

protected:

  RefPtr<TextureHost> mFrontBuffer;
  nsIntRect mPictureRect;
  bool mHasPictureRect;
  bool mLocked;
};

#ifdef MOZ_WIDGET_GONK




class ImageHostOverlay : public CompositableHost {
public:
  ImageHostOverlay(const TextureInfo& aTextureInfo);
  ~ImageHostOverlay();

  virtual CompositableType GetType() { return mTextureInfo.mCompositableType; }

  virtual void Composite(EffectChain& aEffectChain,
                         float aOpacity,
                         const gfx::Matrix4x4& aTransform,
                         const gfx::Filter& aFilter,
                         const gfx::Rect& aClipRect,
                         const nsIntRegion* aVisibleRegion = nullptr) MOZ_OVERRIDE;
  virtual LayerRenderState GetRenderState() MOZ_OVERRIDE;
  virtual void UseOverlaySource(OverlaySource aOverlay) MOZ_OVERRIDE;
  virtual void SetPictureRect(const nsIntRect& aPictureRect) MOZ_OVERRIDE
  {
    mPictureRect = aPictureRect;
    mHasPictureRect = true;
  }
  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix);
protected:
  nsIntRect mPictureRect;
  bool mHasPictureRect;
  OverlaySource mOverlay;
};

#endif

}
}

#endif
