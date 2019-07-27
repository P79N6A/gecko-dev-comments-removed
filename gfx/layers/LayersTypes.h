




#ifndef GFX_LAYERSTYPES_H
#define GFX_LAYERSTYPES_H

#include <stdint.h>                     
#include "mozilla/gfx/Point.h"          
#include "nsRegion.h"

#include "mozilla/TypedEnumBits.h"

#ifdef MOZ_WIDGET_GONK
#include <utils/RefBase.h>
#endif
#include <stdio.h>            
#include "mozilla/Logging.h"            
#ifndef MOZ_LAYERS_HAVE_LOG
#  define MOZ_LAYERS_HAVE_LOG
#endif
#define MOZ_LAYERS_LOG(_args)                             \
  MOZ_LOG(LayerManager::GetLog(), LogLevel::Debug, _args)
#define MOZ_LAYERS_LOG_IF_SHADOWABLE(layer, _args)         \
  do { if (layer->AsShadowableLayer()) { MOZ_LOG(LayerManager::GetLog(), LogLevel::Debug, _args); } } while (0)

#define INVALID_OVERLAY -1

namespace android {
class MOZ_EXPORT GraphicBuffer;
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
  
  
  
  
  LayerRenderState();
  LayerRenderState(const LayerRenderState& aOther);
  ~LayerRenderState();

#ifdef MOZ_WIDGET_GONK
  LayerRenderState(android::GraphicBuffer* aSurface,
                   const gfx::IntSize& aSize,
                   LayerRenderStateFlags aFlags,
                   TextureHost* aTexture);

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
  
  gfx::IntSize mSize;
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

  
  
  
  
  nsIntRegion mNoActionRegion;
  nsIntRegion mHorizontalPanRegion;
  nsIntRegion mVerticalPanRegion;

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

  void ApplyTranslationAndScale(float aXTrans, float aYTrans, float aXScale, float aYScale)
  {
    mHitRegion.ScaleRoundOut(aXScale, aYScale);
    mDispatchToContentHitRegion.ScaleRoundOut(aXScale, aYScale);
    mNoActionRegion.ScaleRoundOut(aXScale, aYScale);
    mHorizontalPanRegion.ScaleRoundOut(aXScale, aYScale);
    mVerticalPanRegion.ScaleRoundOut(aXScale, aYScale);

    mHitRegion.MoveBy(aXTrans, aYTrans);
    mDispatchToContentHitRegion.MoveBy(aXTrans, aYTrans);
    mNoActionRegion.MoveBy(aXTrans, aYTrans);
    mHorizontalPanRegion.MoveBy(aXTrans, aYTrans);
    mVerticalPanRegion.MoveBy(aXTrans, aYTrans);
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





enum EventRegionsOverride {
  
  NoOverride             = 0,
  
  ForceDispatchToContent = (1 << 0),
  
  ForceEmptyHitRegion    = (1 << 1),
  
  ALL_BITS               = (1 << 2) - 1
};

MOZ_ALWAYS_INLINE EventRegionsOverride
operator|(EventRegionsOverride a, EventRegionsOverride b)
{
  return (EventRegionsOverride)((int)a | (int)b);
}

MOZ_ALWAYS_INLINE EventRegionsOverride&
operator|=(EventRegionsOverride& a, EventRegionsOverride b)
{
  a = a | b;
  return a;
}

} 
} 

#endif
