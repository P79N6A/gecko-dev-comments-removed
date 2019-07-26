




#ifndef MOZILLA_LAYERS_COMPOSITORTYPES_H
#define MOZILLA_LAYERS_COMPOSITORTYPES_H

#include "LayersTypes.h"

namespace mozilla {
namespace layers {

typedef int32_t SurfaceDescriptorType;
const SurfaceDescriptorType SURFACEDESCRIPTOR_UNKNOWN = 0;







typedef uint32_t TextureFlags;

const TextureFlags UseNearestFilter   = 0x1;

const TextureFlags NeedsYFlip         = 0x2;


const TextureFlags ForceSingleTile    = 0x4;

const TextureFlags AllowRepeat        = 0x8;

const TextureFlags NewTile            = 0x10;

const TextureFlags HostRelease        = 0x20;

const TextureFlags ComponentAlpha     = 0x40;






enum TextureClientType
{
  TEXTURE_CONTENT,            
  TEXTURE_SHMEM,              
  TEXTURE_YCBCR,              
  TEXTURE_SHARED_GL,          
  TEXTURE_SHARED_GL_EXTERNAL, 
                              
                              
  TEXTURE_STREAM_GL           
};




enum CompositableType
{
  BUFFER_UNKNOWN,
  BUFFER_IMAGE_SINGLE,    
  BUFFER_IMAGE_BUFFERED,  
  BUFFER_BRIDGE,          
  BUFFER_CONTENT,         
  BUFFER_CONTENT_DIRECT,  
  BUFFER_TILED,           
  BUFFER_COUNT
};




enum TextureHostFlags
{
  TEXTURE_HOST_DEFAULT = 0,       
                                  
  TEXTURE_HOST_TILED = 1 << 0     
};






struct TextureFactoryIdentifier
{
  LayersBackend mParentBackend;
  int32_t mMaxTextureSize;

  TextureFactoryIdentifier(LayersBackend aLayersBackend = LAYERS_NONE,
                           int32_t aMaxTextureSize = 0)
    : mParentBackend(aLayersBackend)
    , mMaxTextureSize(aMaxTextureSize)
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
  uint32_t mTextureHostFlags;
  uint32_t mTextureFlags;

  TextureInfo()
    : mCompositableType(BUFFER_UNKNOWN)
    , mTextureHostFlags(0)
    , mTextureFlags(0)
  {}

  TextureInfo(CompositableType aType)
    : mCompositableType(aType)
    , mTextureHostFlags(0)
    , mTextureFlags(0)
  {}

  bool operator==(const TextureInfo& aOther) const
  {
    return mCompositableType == aOther.mCompositableType &&
           mTextureHostFlags == aOther.mTextureHostFlags &&
           mTextureFlags == aOther.mTextureFlags;
  }
};


} 
} 

#endif
