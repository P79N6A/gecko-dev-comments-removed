




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
struct EffectChain;




class ImageHost : public CompositableHost
{
public:
  explicit ImageHost(const TextureInfo& aTextureInfo);
  ~ImageHost();

  virtual CompositableType GetType() override { return mTextureInfo.mCompositableType; }

  virtual void Composite(EffectChain& aEffectChain,
                         float aOpacity,
                         const gfx::Matrix4x4& aTransform,
                         const gfx::Filter& aFilter,
                         const gfx::Rect& aClipRect,
                         const nsIntRegion* aVisibleRegion = nullptr) override;

  virtual void UseTextureHost(const nsTArray<TimedTexture>& aTextures) override;

  virtual void RemoveTextureHost(TextureHost* aTexture) override;

  virtual TextureHost* GetAsTextureHost(gfx::IntRect* aPictureRect = nullptr) override;

  virtual void Attach(Layer* aLayer,
                      Compositor* aCompositor,
                      AttachFlags aFlags = NO_FLAGS) override;

  virtual void SetCompositor(Compositor* aCompositor) override;

  gfx::IntSize GetImageSize() const override;

  virtual LayerRenderState GetRenderState() override;

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix) override;

  virtual void Dump(std::stringstream& aStream,
                    const char* aPrefix = "",
                    bool aDumpHtml = false) override;

  virtual already_AddRefed<gfx::DataSourceSurface> GetAsSurface() override;

  virtual bool Lock() override;

  virtual void Unlock() override;

  virtual already_AddRefed<TexturedEffect> GenEffect(const gfx::Filter& aFilter) override;

protected:
  struct TimedImage {
    CompositableTextureHostRef mFrontBuffer;
    CompositableTextureSourceRef mTextureSource;
    TimeStamp mTimeStamp;
    gfx::IntRect mPictureRect;
  };

  




  const TimedImage* ChooseImage() const;
  TimedImage* ChooseImage();
  int ChooseImageIndex() const;

  nsTArray<TimedImage> mImages;

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
                         const nsIntRegion* aVisibleRegion = nullptr) override;
  virtual LayerRenderState GetRenderState() override;
  virtual void UseOverlaySource(OverlaySource aOverlay,
                                const gfx::IntRect& aPictureRect) override;
  virtual gfx::IntSize GetImageSize() const override;
  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix);
protected:
  gfx::IntRect mPictureRect;
  OverlaySource mOverlay;
};

#endif

}
}

#endif
