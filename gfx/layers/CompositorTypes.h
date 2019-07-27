




#ifndef MOZILLA_LAYERS_COMPOSITORTYPES_H
#define MOZILLA_LAYERS_COMPOSITORTYPES_H

#include <stdint.h>                     
#include <sys/types.h>                  
#include "LayersTypes.h"                
#include "nsXULAppAPI.h"                
#include "mozilla/gfx/Types.h"
#include "mozilla/EnumSet.h"

#include "mozilla/TypedEnum.h"
#include "mozilla/TypedEnumBits.h"

namespace mozilla {
namespace layers {

typedef int32_t SurfaceDescriptorType;
const SurfaceDescriptorType SURFACEDESCRIPTOR_UNKNOWN = 0;









MOZ_BEGIN_ENUM_CLASS(TextureFlags, uint32_t)
  NO_FLAGS           = 0,
  
  USE_NEAREST_FILTER = 1 << 0,
  
  NEEDS_Y_FLIP       = 1 << 1,
  
  
  DISALLOW_BIGIMAGE  = 1 << 2,
  
  ALLOW_REPEAT       = 1 << 3,
  
  NEW_TILE           = 1 << 4,
  
  COMPONENT_ALPHA    = 1 << 5,
  
  
  
  
  
  
  
  RB_SWAPPED         = 1 << 6,

  FRONT              = 1 << 7,
  
  ON_WHITE           = 1 << 8,
  
  ON_BLACK           = 1 << 9,
  
  TILE               = 1 << 10,
  
  RECYCLE            = 1 << 11,
  
  
  COPY_PREVIOUS      = 1 << 12,
  
  
  
  
  
  DEALLOCATE_CLIENT  = 1 << 13,
  
  
  
  IMMUTABLE          = 1 << 14,
  
  
  
  IMMEDIATE_UPLOAD   = 1 << 15,
  
  
  
  DOUBLE_BUFFERED    = 1 << 16,
  
  NON_PREMULTIPLIED  = 1 << 18,

  
  ALL_BITS           = (1 << 19) - 1,
  
  DEFAULT = FRONT
MOZ_END_ENUM_CLASS(TextureFlags)
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(TextureFlags)

static inline bool
TextureRequiresLocking(TextureFlags aFlags)
{
  
  
  
  return !(aFlags & (TextureFlags::IMMEDIATE_UPLOAD |
                     TextureFlags::DOUBLE_BUFFERED |
                     TextureFlags::IMMUTABLE));
}




MOZ_BEGIN_ENUM_CLASS(DiagnosticTypes, uint8_t)
  NO_DIAGNOSTIC    = 0,
  TILE_BORDERS     = 1 << 0,
  LAYER_BORDERS    = 1 << 1,
  BIGIMAGE_BORDERS = 1 << 2,
  FLASH_BORDERS    = 1 << 3,
  ALL_BITS         = (1 << 4) - 1
MOZ_END_ENUM_CLASS(DiagnosticTypes)
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(DiagnosticTypes)

#define DIAGNOSTIC_FLASH_COUNTER_MAX 100




MOZ_BEGIN_ENUM_CLASS(DiagnosticFlags, uint16_t)
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
MOZ_END_ENUM_CLASS(DiagnosticFlags)
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(DiagnosticFlags)




MOZ_BEGIN_ENUM_CLASS(EffectTypes, uint8_t)
  MASK,
  BLEND_MODE,
  MAX_SECONDARY, 
  RGB,
  YCBCR,
  COMPONENT_ALPHA,
  SOLID_COLOR,
  RENDER_TARGET,
  MAX  
MOZ_END_ENUM_CLASS(EffectTypes)




MOZ_BEGIN_ENUM_CLASS(CompositableType, uint8_t)
  BUFFER_UNKNOWN,
  
  BUFFER_IMAGE_SINGLE,    
  BUFFER_IMAGE_BUFFERED,  
  BUFFER_BRIDGE,          
  BUFFER_CONTENT_INC,     
                          
  
  BUFFER_TILED,           
  BUFFER_SIMPLE_TILED,
  
  IMAGE,     
  CONTENT_SINGLE,  
  CONTENT_DOUBLE,  
  BUFFER_COUNT
MOZ_END_ENUM_CLASS(CompositableType)




MOZ_BEGIN_ENUM_CLASS(DeprecatedTextureHostFlags, uint8_t)
  DEFAULT = 0,       
  TILED = 1 << 0,    
  COPY_PREVIOUS = 1 << 1, 
                                      
  ALL_BITS = (1 << 2) - 1
MOZ_END_ENUM_CLASS(DeprecatedTextureHostFlags)
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(DeprecatedTextureHostFlags)






struct TextureFactoryIdentifier
{
  LayersBackend mParentBackend;
  GeckoProcessType mParentProcessId;
  EnumSet<gfx::CompositionOp> mSupportedBlendModes;
  int32_t mMaxTextureSize;
  bool mSupportsTextureBlitting;
  bool mSupportsPartialUploads;

  explicit TextureFactoryIdentifier(LayersBackend aLayersBackend = LayersBackend::LAYERS_NONE,
                                    GeckoProcessType aParentProcessId = GeckoProcessType_Default,
                                    int32_t aMaxTextureSize = 4096,
                                    bool aSupportsTextureBlitting = false,
                                    bool aSupportsPartialUploads = false)
    : mParentBackend(aLayersBackend)
    , mParentProcessId(aParentProcessId)
    , mSupportedBlendModes(gfx::CompositionOp::OP_OVER)
    , mMaxTextureSize(aMaxTextureSize)
    , mSupportsTextureBlitting(aSupportsTextureBlitting)
    , mSupportsPartialUploads(aSupportsPartialUploads)
  {}
};







MOZ_BEGIN_ENUM_CLASS(TextureIdentifier, uint8_t)
  Front = 1,
  Back = 2,
  OnWhiteFront = 3,
  OnWhiteBack = 4,
  HighBound
MOZ_END_ENUM_CLASS(TextureIdentifier)








struct TextureInfo
{
  CompositableType mCompositableType;
  DeprecatedTextureHostFlags mDeprecatedTextureHostFlags;
  TextureFlags mTextureFlags;

  TextureInfo()
    : mCompositableType(CompositableType::BUFFER_UNKNOWN)
    , mDeprecatedTextureHostFlags(DeprecatedTextureHostFlags::DEFAULT)
    , mTextureFlags(TextureFlags::NO_FLAGS)
  {}

  explicit TextureInfo(CompositableType aType)
    : mCompositableType(aType)
    , mDeprecatedTextureHostFlags(DeprecatedTextureHostFlags::DEFAULT)
    , mTextureFlags(TextureFlags::NO_FLAGS)
  {}

  bool operator==(const TextureInfo& aOther) const
  {
    return mCompositableType == aOther.mCompositableType &&
           mDeprecatedTextureHostFlags == aOther.mDeprecatedTextureHostFlags &&
           mTextureFlags == aOther.mTextureFlags;
  }
};






MOZ_BEGIN_ENUM_CLASS(OpenMode, uint8_t)
  OPEN_NONE        = 0,
  OPEN_READ        = 0x1,
  OPEN_WRITE       = 0x2,
  OPEN_READ_WRITE  = OPEN_READ|OPEN_WRITE,
  OPEN_READ_ONLY   = OPEN_READ,
  OPEN_WRITE_ONLY  = OPEN_WRITE
MOZ_END_ENUM_CLASS(OpenMode)
MOZ_MAKE_ENUM_CLASS_BITWISE_OPERATORS(OpenMode)



MOZ_BEGIN_ENUM_CLASS(MaskType, uint8_t)
  MaskNone = 0,   
  Mask2d,         
  Mask3d,         
  NumMaskTypes
MOZ_END_ENUM_CLASS(MaskType)

} 
} 

#endif
