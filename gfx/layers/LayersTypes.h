




#ifndef GFX_LAYERSTYPES_H
#define GFX_LAYERSTYPES_H

#include <stdint.h>                     
#include "nsPoint.h"                    
#include "nsRegion.h"

#include "mozilla/TypedEnum.h"

#ifdef MOZ_WIDGET_GONK
#include <ui/GraphicBuffer.h>
#endif
#if defined(DEBUG) || defined(PR_LOGGING)
#  include <stdio.h>            
#  include "prlog.h"            
#  ifndef MOZ_LAYERS_HAVE_LOG
#    define MOZ_LAYERS_HAVE_LOG
#  endif
#  define MOZ_LAYERS_LOG(_args)                             \
  PR_LOG(LayerManager::GetLog(), PR_LOG_DEBUG, _args)
#  define MOZ_LAYERS_LOG_IF_SHADOWABLE(layer, _args)         \
  do { if (layer->AsShadowableLayer()) { PR_LOG(LayerManager::GetLog(), PR_LOG_DEBUG, _args); } } while (0)
#else
struct PRLogModuleInfo;
#  define MOZ_LAYERS_LOG(_args)
#  define MOZ_LAYERS_LOG_IF_SHADOWABLE(layer, _args)
#endif  

namespace android {
class GraphicBuffer;
}

namespace mozilla {
namespace layers {

class TextureHost;

#undef NONE
#undef OPAQUE

MOZ_BEGIN_ENUM_CLASS(LayersBackend, int8_t)
  LAYERS_NONE = 0,
  LAYERS_BASIC,
  LAYERS_OPENGL,
  LAYERS_D3D9,
  LAYERS_D3D10,
  LAYERS_D3D11,
  LAYERS_CLIENT,
  LAYERS_LAST
MOZ_END_ENUM_CLASS(LayersBackend)

MOZ_BEGIN_ENUM_CLASS(BufferMode, int8_t)
  BUFFER_NONE,
  BUFFERED
MOZ_END_ENUM_CLASS(BufferMode)

MOZ_BEGIN_ENUM_CLASS(DrawRegionClip, int8_t)
  DRAW,
  DRAW_SNAPPED,
  CLIP_NONE
MOZ_END_ENUM_CLASS(DrawRegionClip)

MOZ_BEGIN_ENUM_CLASS(SurfaceMode, int8_t)
  SURFACE_NONE = 0,
  SURFACE_OPAQUE,
  SURFACE_SINGLE_CHANNEL_ALPHA,
  SURFACE_COMPONENT_ALPHA
MOZ_END_ENUM_CLASS(SurfaceMode)




enum LayerRenderStateFlags {
  LAYER_RENDER_STATE_Y_FLIPPED = 1 << 0,
  LAYER_RENDER_STATE_BUFFER_ROTATION = 1 << 1,
  
  LAYER_RENDER_STATE_FORMAT_RB_SWAP = 1 << 2
};



struct LayerRenderState {
  LayerRenderState()
#ifdef MOZ_WIDGET_GONK
    : mSurface(nullptr), mTexture(nullptr), mFlags(0), mHasOwnOffset(false)
#endif
  {}

#ifdef MOZ_WIDGET_GONK
  LayerRenderState(android::GraphicBuffer* aSurface,
                   const nsIntSize& aSize,
                   uint32_t aFlags,
                   TextureHost* aTexture)
    : mSurface(aSurface)
    , mSize(aSize)
    , mTexture(aTexture)
    , mFlags(aFlags)
    , mHasOwnOffset(false)
  {}

  bool YFlipped() const
  { return mFlags & LAYER_RENDER_STATE_Y_FLIPPED; }

  bool BufferRotated() const
  { return mFlags & LAYER_RENDER_STATE_BUFFER_ROTATION; }

  bool FormatRBSwapped() const
  { return mFlags & LAYER_RENDER_STATE_FORMAT_RB_SWAP; }
#endif

  void SetOffset(const nsIntPoint& aOffset)
  {
    mOffset = aOffset;
    mHasOwnOffset = true;
  }

#ifdef MOZ_WIDGET_GONK
  
  android::sp<android::GraphicBuffer> mSurface;
  
  nsIntSize mSize;
  TextureHost* mTexture;
#endif
  
  uint32_t mFlags;
  
  nsIntPoint mOffset;
  
  bool mHasOwnOffset;
};

MOZ_BEGIN_ENUM_CLASS(ScaleMode, int8_t)
  SCALE_NONE,
  STRETCH,
  SENTINEL

MOZ_END_ENUM_CLASS(ScaleMode)

struct EventRegions {
  nsIntRegion mHitRegion;
  nsIntRegion mDispatchToContentHitRegion;

  bool operator==(const EventRegions& aRegions) const
  {
    return mHitRegion == aRegions.mHitRegion &&
           mDispatchToContentHitRegion == aRegions.mDispatchToContentHitRegion;
  }
  bool operator!=(const EventRegions& aRegions) const
  {
    return !(*this == aRegions);
  }

  nsCString ToString() const
  {
    nsCString result = mHitRegion.ToString();
    result.AppendLiteral(";dispatchToContent=");
    result.Append(mDispatchToContentHitRegion.ToString());
    return result;
  }
};

} 
} 

#endif 
