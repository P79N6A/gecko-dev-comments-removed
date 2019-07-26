




#ifndef MOZILLA_LAYERS_COMPOSITORTYPES_H
#define MOZILLA_LAYERS_COMPOSITORTYPES_H

#include <stdint.h>                     
#include <sys/types.h>                  
#include "LayersTypes.h"                
#include "nsXULAppAPI.h"                

namespace mozilla {
namespace layers {

typedef int32_t SurfaceDescriptorType;
const SurfaceDescriptorType SURFACEDESCRIPTOR_UNKNOWN = 0;









typedef uint32_t TextureFlags;

const TextureFlags TEXTURE_USE_NEAREST_FILTER = 1 << 0;

const TextureFlags TEXTURE_NEEDS_Y_FLIP       = 1 << 1;


const TextureFlags TEXTURE_DISALLOW_BIGIMAGE  = 1 << 2;

const TextureFlags TEXTURE_ALLOW_REPEAT       = 1 << 3;

const TextureFlags TEXTURE_NEW_TILE           = 1 << 4;

const TextureFlags TEXTURE_COMPONENT_ALPHA    = 1 << 5;







const TextureFlags TEXTURE_RB_SWAPPED         = 1 << 6;


const TextureFlags TEXTURE_FRONT              = 1 << 12;

const TextureFlags TEXTURE_ON_WHITE           = 1 << 13;
 
const TextureFlags TEXTURE_ON_BLACK           = 1 << 14;

const TextureFlags TEXTURE_TILE               = 1 << 15;


const TextureFlags TEXTURE_COPY_PREVIOUS      = 1 << 24;





const TextureFlags TEXTURE_DEALLOCATE_CLIENT  = 1 << 25;
const TextureFlags TEXTURE_DEALLOCATE_HOST    = 1 << 26;



const TextureFlags TEXTURE_IMMUTABLE          = 1 << 27;



const TextureFlags TEXTURE_IMMEDIATE_UPLOAD   = 1 << 28;



const TextureFlags TEXTURE_DOUBLE_BUFFERED    = 1 << 29;


const TextureFlags TEXTURE_FLAGS_DEFAULT = TEXTURE_DEALLOCATE_HOST
                                         | TEXTURE_FRONT;

static inline bool
TextureRequiresLocking(TextureFlags aFlags)
{
  
  
  
  return !(aFlags & (TEXTURE_IMMEDIATE_UPLOAD |
                     TEXTURE_DOUBLE_BUFFERED |
                     TEXTURE_IMMUTABLE));
}




typedef uint32_t DiagnosticTypes;
const DiagnosticTypes DIAGNOSTIC_NONE             = 0;
const DiagnosticTypes DIAGNOSTIC_TILE_BORDERS     = 1 << 0;
const DiagnosticTypes DIAGNOSTIC_LAYER_BORDERS    = 1 << 1;
const DiagnosticTypes DIAGNOSTIC_BIGIMAGE_BORDERS = 1 << 2;




typedef uint32_t DiagnosticFlags;
const DiagnosticFlags DIAGNOSTIC_IMAGE      = 1 << 0;
const DiagnosticFlags DIAGNOSTIC_CONTENT    = 1 << 1;
const DiagnosticFlags DIAGNOSTIC_CANVAS     = 1 << 2;
const DiagnosticFlags DIAGNOSTIC_COLOR      = 1 << 3;
const DiagnosticFlags DIAGNOSTIC_CONTAINER  = 1 << 4;
const DiagnosticFlags DIAGNOSTIC_TILE       = 1 << 5;
const DiagnosticFlags DIAGNOSTIC_BIGIMAGE   = 1 << 6;
const DiagnosticFlags DIAGNOSTIC_COMPONENT_ALPHA = 1 << 7;




enum EffectTypes
{
  EFFECT_MASK,
  EFFECT_MAX_SECONDARY, 
  EFFECT_BGRX,
  EFFECT_RGBX,
  EFFECT_BGRA,
  EFFECT_RGBA,
  EFFECT_YCBCR,
  EFFECT_COMPONENT_ALPHA,
  EFFECT_SOLID_COLOR,
  EFFECT_RENDER_TARGET,
  EFFECT_MAX  
};






enum DeprecatedTextureClientType
{
  TEXTURE_CONTENT,            
  TEXTURE_SHMEM,              
  TEXTURE_YCBCR,              
  TEXTURE_SHARED_GL,          
  TEXTURE_SHARED_GL_EXTERNAL, 
                              
                              
  TEXTURE_STREAM_GL,          
  TEXTURE_FALLBACK            
};




enum CompositableType
{
  BUFFER_UNKNOWN,
  
  BUFFER_IMAGE_SINGLE,    
  BUFFER_IMAGE_BUFFERED,  
  BUFFER_BRIDGE,          
  BUFFER_CONTENT,         
  BUFFER_CONTENT_DIRECT,  
  BUFFER_CONTENT_INC,     
                          
  BUFFER_TILED,           
  
  COMPOSITABLE_IMAGE,     
  BUFFER_COUNT
};




enum DeprecatedTextureHostFlags
{
  TEXTURE_HOST_DEFAULT = 0,       
                                  
  TEXTURE_HOST_TILED = 1 << 0,    
  TEXTURE_HOST_COPY_PREVIOUS = 1 << 1 
                                      
};






struct TextureFactoryIdentifier
{
  LayersBackend mParentBackend;
  GeckoProcessType mParentProcessId;
  int32_t mMaxTextureSize;
  bool mSupportsTextureBlitting;
  bool mSupportsPartialUploads;

  TextureFactoryIdentifier(LayersBackend aLayersBackend = LAYERS_NONE,
                           GeckoProcessType aParentProcessId = GeckoProcessType_Default,
                           int32_t aMaxTextureSize = 0,
                           bool aSupportsTextureBlitting = false,
                           bool aSupportsPartialUploads = false)
    : mParentBackend(aLayersBackend)
    , mParentProcessId(aParentProcessId)
    , mMaxTextureSize(aMaxTextureSize)
    , mSupportsTextureBlitting(aSupportsTextureBlitting)
    , mSupportsPartialUploads(aSupportsPartialUploads)
  {}
};







typedef uint32_t TextureIdentifier;
const TextureIdentifier TextureFront = 1;
const TextureIdentifier TextureBack = 2;
const TextureIdentifier TextureOnWhiteFront = 3;
const TextureIdentifier TextureOnWhiteBack = 4;








struct TextureInfo
{
  CompositableType mCompositableType;
  uint32_t mDeprecatedTextureHostFlags;
  uint32_t mTextureFlags;

  TextureInfo()
    : mCompositableType(BUFFER_UNKNOWN)
    , mDeprecatedTextureHostFlags(0)
    , mTextureFlags(0)
  {}

  TextureInfo(CompositableType aType)
    : mCompositableType(aType)
    , mDeprecatedTextureHostFlags(0)
    , mTextureFlags(0)
  {}

  bool operator==(const TextureInfo& aOther) const
  {
    return mCompositableType == aOther.mCompositableType &&
           mDeprecatedTextureHostFlags == aOther.mDeprecatedTextureHostFlags &&
           mTextureFlags == aOther.mTextureFlags;
  }
};






enum OpenMode {
  OPEN_READ_ONLY  = 0x1,
  OPEN_READ_WRITE = 0x2
};



enum MaskType {
  MaskNone = 0,   
  Mask2d,         
  Mask3d,         
  NumMaskTypes
};

} 
} 

#endif
