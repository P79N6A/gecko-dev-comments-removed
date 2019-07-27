




#include "TextureClientSharedSurface.h"

#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/Logging.h"        
#include "mozilla/layers/ISurfaceAllocator.h"
#include "SharedSurface.h"
#include "GLContext.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace layers {

SharedSurfaceTextureClient::SharedSurfaceTextureClient(ISurfaceAllocator* aAllocator,
                                                       TextureFlags aFlags,
                                                       UniquePtr<gl::SharedSurface> surf,
                                                       gl::SurfaceFactory* factory)
  : TextureClient(aAllocator, aFlags | TextureFlags::RECYCLE)
  , mSurf(Move(surf))
  , mFactory(factory)
  , mRecyclingRef(this)
{
  
  
  
  
  
  mRecyclingRef->SetRecycleCallback(&gl::SurfaceFactory::RecycleCallback, nullptr);
}

SharedSurfaceTextureClient::~SharedSurfaceTextureClient()
{
  

  
  
  
  
  
  mozilla::unused << mRecyclingRef.forget().take();
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

void
SharedSurfaceTextureClient::StopRecycling()
{
  mRecyclingRef->ClearRecycleCallback();
  mRecyclingRef = nullptr;
}

} 
} 
