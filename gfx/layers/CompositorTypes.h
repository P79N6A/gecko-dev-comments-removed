




#ifndef MOZILLA_LAYERS_COMPOSITORTYPES_H
#define MOZILLA_LAYERS_COMPOSITORTYPES_H

#include <stdint.h>                     
#include <sys/types.h>                  
#include "LayersTypes.h"                
#include "nsXULAppAPI.h"                
#include "mozilla/gfx/Types.h"
#include "mozilla/EnumSet.h"

#include "mozilla/TypedEnumBits.h"

namespace mozilla {
namespace layers {







enum class TextureFlags : uint32_t {
  NO_FLAGS           = 0,
  
  USE_NEAREST_FILTER = 1 << 0,
  
  ORIGIN_BOTTOM_LEFT = 1 << 1,
  
  
  DISALLOW_BIGIMAGE  = 1 << 2,
  
  
  
  
  
  
  
  RB_SWAPPED         = 1 << 3,
  
  
  NON_PREMULTIPLIED  = 1 << 4,
  
  RECYCLE            = 1 << 5,
  
  
  
  
  DEALLOCATE_CLIENT  = 1 << 6,
  
  
  
  IMMUTABLE          = 1 << 7,
  
  
  
  IMMEDIATE_UPLOAD   = 1 << 8,
  
  COMPONENT_ALPHA    = 1 << 9,

  
  ALL_BITS           = (1 << 10) - 1,
  
  DEFAULT = NO_FLAGS
};
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(TextureFlags)

static inline bool
TextureRequiresLocking(TextureFlags aFlags)
{
  
  
  
  return !(aFlags & (TextureFlags::IMMEDIATE_UPLOAD |
                     TextureFlags::IMMUTABLE));
}




enum class DiagnosticTypes : uint8_t {
  NO_DIAGNOSTIC    = 0,
  TILE_BORDERS     = 1 << 0,
  LAYER_BORDERS    = 1 << 1,
  BIGIMAGE_BORDERS = 1 << 2,
  FLASH_BORDERS    = 1 << 3,
  ALL_BITS         = (1 << 4) - 1
};
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(DiagnosticTypes)

#define DIAGNOSTIC_FLASH_COUNTER_MAX 100




enum class DiagnosticFlags : uint16_t {
  NO_DIAGNOSTIC   = 0,
  IMAGE           = 1 << 0,
  CONTENT         = 1 << 1,
  CANVAS          = 1 << 2,
  COLOR           = 1 << 3,
  CONTAINER       = 1 << 4,
  TILE            = 1 << 5,
  BIGIMAGE        = 1 << 6,
  COMPONENT_ALPHA = 1 << 7,
  REGION_RECT     = 1 << 8
};
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(DiagnosticFlags)




enum class EffectTypes : uint8_t {
  MASK,
  BLEND_MODE,
  COLOR_MATRIX,
  MAX_SECONDARY, 
  RGB,
  YCBCR,
  COMPONENT_ALPHA,
  SOLID_COLOR,
  RENDER_TARGET,
  VR_DISTORTION,
  MAX  
};




enum class CompositableType : uint8_t {
  UNKNOWN,
  CONTENT_TILED,   
  IMAGE,           
  IMAGE_OVERLAY,   
  IMAGE_BRIDGE,    
  CONTENT_SINGLE,  
  CONTENT_DOUBLE,  
  COUNT
};

#ifdef XP_WIN
typedef void* SyncHandle;
#else
typedef uintptr_t SyncHandle;
#endif 






struct TextureFactoryIdentifier
{
  LayersBackend mParentBackend;
  GeckoProcessType mParentProcessId;
  EnumSet<gfx::CompositionOp> mSupportedBlendModes;
  int32_t mMaxTextureSize;
  bool mSupportsTextureBlitting;
  bool mSupportsPartialUploads;
  SyncHandle mSyncHandle;

  explicit TextureFactoryIdentifier(LayersBackend aLayersBackend = LayersBackend::LAYERS_NONE,
                                    GeckoProcessType aParentProcessId = GeckoProcessType_Default,
                                    int32_t aMaxTextureSize = 4096,
                                    bool aSupportsTextureBlitting = false,
                                    bool aSupportsPartialUploads = false,
                                    SyncHandle aSyncHandle = 0)
    : mParentBackend(aLayersBackend)
    , mParentProcessId(aParentProcessId)
    , mSupportedBlendModes(gfx::CompositionOp::OP_OVER)
    , mMaxTextureSize(aMaxTextureSize)
    , mSupportsTextureBlitting(aSupportsTextureBlitting)
    , mSupportsPartialUploads(aSupportsPartialUploads)
    , mSyncHandle(aSyncHandle)
  {}
};








struct TextureInfo
{
  CompositableType mCompositableType;
  TextureFlags mTextureFlags;

  TextureInfo()
    : mCompositableType(CompositableType::UNKNOWN)
    , mTextureFlags(TextureFlags::NO_FLAGS)
  {}

  explicit TextureInfo(CompositableType aType,
                       TextureFlags aTextureFlags = TextureFlags::DEFAULT)
    : mCompositableType(aType)
    , mTextureFlags(aTextureFlags)
  {}

  bool operator==(const TextureInfo& aOther) const
  {
    return mCompositableType == aOther.mCompositableType &&
           mTextureFlags == aOther.mTextureFlags;
  }
};






enum class OpenMode : uint8_t {
  OPEN_NONE        = 0,
  OPEN_READ        = 0x1,
  OPEN_WRITE       = 0x2,
  OPEN_READ_WRITE  = OPEN_READ|OPEN_WRITE,
  OPEN_READ_ONLY   = OPEN_READ,
  OPEN_WRITE_ONLY  = OPEN_WRITE
};
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(OpenMode)



enum class MaskType : uint8_t {
  MaskNone = 0,   
  Mask2d,         
  Mask3d,         
  NumMaskTypes
};

} 
} 

#endif
