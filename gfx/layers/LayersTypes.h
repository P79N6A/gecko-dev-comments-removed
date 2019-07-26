




#ifndef GFX_LAYERSTYPES_H
#define GFX_LAYERSTYPES_H

#include <stdint.h>                     
#include "nsPoint.h"                    





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


typedef uint32_t TextureFlags;

enum LayersBackend {
  LAYERS_NONE = 0,
  LAYERS_BASIC,
  LAYERS_OPENGL,
  LAYERS_D3D9,
  LAYERS_D3D10,
  LAYERS_D3D11,
  LAYERS_CLIENT,
  LAYERS_LAST
};

enum BufferMode {
  BUFFER_NONE,
  BUFFER_BUFFERED
};




enum LayerRenderStateFlags {
  LAYER_RENDER_STATE_Y_FLIPPED = 1 << 0,
  LAYER_RENDER_STATE_BUFFER_ROTATION = 1 << 1,
  
  LAYER_RENDER_STATE_FORMAT_RB_SWAP = 1 << 2
};



struct LayerRenderState {
  LayerRenderState()
#ifdef MOZ_WIDGET_GONK
    : mSurface(nullptr), mFlags(0), mHasOwnOffset(false)
#endif
  {}

#ifdef MOZ_WIDGET_GONK
  LayerRenderState(android::GraphicBuffer* aSurface,
                   const nsIntSize& aSize,
                   uint32_t aFlags)
    : mSurface(aSurface)
    , mSize(aSize)
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
#endif
  
  uint32_t mFlags;
  
  nsIntPoint mOffset;
  
  bool mHasOwnOffset;
};

enum ScaleMode {
  SCALE_NONE,
  SCALE_STRETCH,
  SCALE_SENTINEL

};

} 
} 

#endif 
