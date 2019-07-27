




#ifndef GFX_LAYERSTYPES_H
#define GFX_LAYERSTYPES_H

#include <stdint.h>                     
#include "nsPoint.h"                    
#include "nsRegion.h"

#include "mozilla/TypedEnum.h"
#include "mozilla/TypedEnumBits.h"

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

#define INVALID_OVERLAY -1

namespace android {
class GraphicBuffer;
}

namespace mozilla {
namespace layers {

class TextureHost;

#undef NONE
#undef OPAQUE

enum class LayersBackend : int8_t {
  LAYERS_NONE = 0,
  LAYERS_BASIC,
  LAYERS_OPENGL,
  LAYERS_D3D9,
  LAYERS_D3D10,
  LAYERS_D3D11,
  LAYERS_CLIENT,
  LAYERS_LAST
};

enum class BufferMode : int8_t {
  BUFFER_NONE,
  BUFFERED
};

enum class DrawRegionClip : int8_t {
  DRAW,
  NONE
};

enum class SurfaceMode : int8_t {
  SURFACE_NONE = 0,
  SURFACE_OPAQUE,
  SURFACE_SINGLE_CHANNEL_ALPHA,
  SURFACE_COMPONENT_ALPHA
};




enum class LayerRenderStateFlags : int8_t {
  LAYER_RENDER_STATE_DEFAULT = 0,
  ORIGIN_BOTTOM_LEFT = 1 << 0,
  BUFFER_ROTATION = 1 << 1,
  
  FORMAT_RB_SWAP = 1 << 2,
  
  
  
  OPAQUE = 1 << 3
};
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(LayerRenderStateFlags)



struct LayerRenderState {
  LayerRenderState()
#ifdef MOZ_WIDGET_GONK
    : mFlags(LayerRenderStateFlags::LAYER_RENDER_STATE_DEFAULT)
    , mHasOwnOffset(false)
    , mSurface(nullptr)
    , mOverlayId(INVALID_OVERLAY)
    , mTexture(nullptr)
#endif
  {}

#ifdef MOZ_WIDGET_GONK
  LayerRenderState(android::GraphicBuffer* aSurface,
                   const nsIntSize& aSize,
                   LayerRenderStateFlags aFlags,
                   TextureHost* aTexture)
    : mFlags(aFlags)
    , mHasOwnOffset(false)
    , mSurface(aSurface)
    , mOverlayId(INVALID_OVERLAY)
    , mSize(aSize)
    , mTexture(aTexture)
  {}

  bool OriginBottomLeft() const
  { return bool(mFlags & LayerRenderStateFlags::ORIGIN_BOTTOM_LEFT); }

  bool BufferRotated() const
  { return bool(mFlags & LayerRenderStateFlags::BUFFER_ROTATION); }

  bool FormatRBSwapped() const
  { return bool(mFlags & LayerRenderStateFlags::FORMAT_RB_SWAP); }

  void SetOverlayId(const int32_t& aId)
  { mOverlayId = aId; }
#endif

  void SetOffset(const nsIntPoint& aOffset)
  {
    mOffset = aOffset;
    mHasOwnOffset = true;
  }

  
  LayerRenderStateFlags mFlags;
  
  bool mHasOwnOffset;
  
  nsIntPoint mOffset;
#ifdef MOZ_WIDGET_GONK
  
  android::sp<android::GraphicBuffer> mSurface;
  int32_t mOverlayId;
  
  nsIntSize mSize;
  TextureHost* mTexture;
#endif
};

enum class ScaleMode : int8_t {
  SCALE_NONE,
  STRETCH,
  SENTINEL

};

struct EventRegions {
  nsIntRegion mHitRegion;
  nsIntRegion mDispatchToContentHitRegion;

  EventRegions()
  {
  }

  explicit EventRegions(nsIntRegion aHitRegion)
    : mHitRegion(aHitRegion)
  {
  }

  bool operator==(const EventRegions& aRegions) const
  {
    return mHitRegion == aRegions.mHitRegion &&
           mDispatchToContentHitRegion == aRegions.mDispatchToContentHitRegion;
  }
  bool operator!=(const EventRegions& aRegions) const
  {
    return !(*this == aRegions);
  }

  void OrWith(const EventRegions& aOther)
  {
    mHitRegion.OrWith(aOther.mHitRegion);
    mDispatchToContentHitRegion.OrWith(aOther.mDispatchToContentHitRegion);
  }

  void AndWith(const nsIntRegion& aRegion)
  {
    mHitRegion.AndWith(aRegion);
    mDispatchToContentHitRegion.AndWith(aRegion);
  }

  void Sub(const EventRegions& aMinuend, const nsIntRegion& aSubtrahend)
  {
    mHitRegion.Sub(aMinuend.mHitRegion, aSubtrahend);
    mDispatchToContentHitRegion.Sub(aMinuend.mDispatchToContentHitRegion, aSubtrahend);
  }

  void Transform(const gfx3DMatrix& aTransform)
  {
    mHitRegion.Transform(aTransform);
    mDispatchToContentHitRegion.Transform(aTransform);
  }

  bool IsEmpty() const
  {
    return mHitRegion.IsEmpty()
        && mDispatchToContentHitRegion.IsEmpty();
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
