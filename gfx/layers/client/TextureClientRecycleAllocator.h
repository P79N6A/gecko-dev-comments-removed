




#ifndef MOZILLA_GFX_TEXTURECLIENT_RECYCLE_ALLOCATOR_H
#define MOZILLA_GFX_TEXTURECLIENT_RECYCLE_ALLOCATOR_H

#include "mozilla/gfx/Types.h"
#include "mozilla/RefPtr.h"
#include "TextureClient.h"

namespace mozilla {
namespace layers {

class ISurfaceAllocator;
class TextureClientRecycleAllocatorImp;








class TextureClientRecycleAllocator
{
  ~TextureClientRecycleAllocator();

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TextureClientRecycleAllocator)

  explicit TextureClientRecycleAllocator(ISurfaceAllocator* aAllocator);

  void SetMaxPoolSize(uint32_t aMax);

  
  already_AddRefed<TextureClient>
  CreateOrRecycleForDrawing(gfx::SurfaceFormat aFormat,
                            gfx::IntSize aSize,
                            gfx::BackendType aMoz2dBackend,
                            TextureFlags aTextureFlags,
                            TextureAllocationFlags flags = ALLOC_DEFAULT);

private:
  RefPtr<TextureClientRecycleAllocatorImp> mAllocator;
};

}
}
#endif 
