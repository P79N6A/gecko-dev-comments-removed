




#include "TextureClientSharedSurface.h"

#include "GLContext.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/Logging.h"        
#include "mozilla/layers/ISurfaceAllocator.h"
#include "mozilla/unused.h"
#include "nsThreadUtils.h"
#include "SharedSurface.h"

namespace mozilla {
namespace layers {

SharedSurfaceTextureClient::SharedSurfaceTextureClient(ISurfaceAllocator* aAllocator,
                                                       TextureFlags aFlags,
                                                       UniquePtr<gl::SharedSurface> surf,
                                                       gl::SurfaceFactory* factory)
  : TextureClient(aAllocator, aFlags | TextureFlags::RECYCLE)
  , mSurf(Move(surf))
{ }

SharedSurfaceTextureClient::~SharedSurfaceTextureClient()
{
  
}

gfx::IntSize
SharedSurfaceTextureClient::GetSize() const
{
  return mSurf->mSize;
}

bool
SharedSurfaceTextureClient::ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor)
{
  return mSurf->ToSurfaceDescriptor(&aOutDescriptor);
}

} 
} 
