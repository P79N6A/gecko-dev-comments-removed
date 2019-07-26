




#ifndef GFX_LAYERSTYPES_H
#define GFX_LAYERSTYPES_H

#include "nsPoint.h"

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

namespace mozilla {
namespace layers {

class SurfaceDescriptor;

typedef uint32_t TextureFlags;

enum LayersBackend {
  LAYERS_NONE = 0,
  LAYERS_BASIC,
  LAYERS_OPENGL,
  LAYERS_D3D9,
  LAYERS_D3D10,
  LAYERS_CLIENT,
  LAYERS_LAST
};

enum BufferMode {
  BUFFER_NONE,
  BUFFER_BUFFERED
};



enum MaskType {
  MaskNone = 0,   
  Mask2d,         
  Mask3d,         
  NumMaskTypes
};


enum LayerRenderStateFlags {
  LAYER_RENDER_STATE_Y_FLIPPED = 1 << 0,
  LAYER_RENDER_STATE_BUFFER_ROTATION = 1 << 1
};

struct LayerRenderState {
  LayerRenderState() : mSurface(nullptr), mFlags(0), mHasOwnOffset(false)
  {}

  LayerRenderState(SurfaceDescriptor* aSurface, uint32_t aFlags = 0)
    : mSurface(aSurface)
    , mFlags(aFlags)
    , mHasOwnOffset(false)
  {}

  LayerRenderState(SurfaceDescriptor* aSurface, nsIntPoint aOffset, uint32_t aFlags = 0)
    : mSurface(aSurface)
    , mFlags(aFlags)
    , mOffset(aOffset)
    , mHasOwnOffset(true)
  {}

  bool YFlipped() const
  { return mFlags & LAYER_RENDER_STATE_Y_FLIPPED; }

  bool BufferRotated() const
  { return mFlags & LAYER_RENDER_STATE_BUFFER_ROTATION; }

  SurfaceDescriptor* mSurface;
  uint32_t mFlags;
  nsIntPoint mOffset;
  bool mHasOwnOffset;
};

} 
} 

#endif 
