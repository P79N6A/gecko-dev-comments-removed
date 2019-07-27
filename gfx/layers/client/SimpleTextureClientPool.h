




#ifndef MOZILLA_GFX_SIMPLETEXTURECLIENTPOOL_H
#define MOZILLA_GFX_SIMPLETEXTURECLIENTPOOL_H

#include "mozilla/gfx/Types.h"
#include "mozilla/gfx/Point.h"
#include "mozilla/RefPtr.h"
#include "TextureClient.h"
#include "nsITimer.h"
#include <stack>
#include <list>

namespace mozilla {
namespace layers {

class ISurfaceAllocator;

class SimpleTextureClientPool
{
  ~SimpleTextureClientPool()
  {
    for (auto it = mOutstandingTextureClients.begin(); it != mOutstandingTextureClients.end(); ++it) {
      (*it)->ClearRecycleCallback();
    }
  }

public:
  NS_INLINE_DECL_REFCOUNTING(SimpleTextureClientPool)

  SimpleTextureClientPool(gfx::SurfaceFormat aFormat, gfx::IntSize aSize,
                          uint32_t aMaxTextureClients,
                          uint32_t aShrinkTimeoutMsec,
                          ISurfaceAllocator *aAllocator);

  




  TemporaryRef<TextureClient> GetTextureClient(bool aAutoRecycle = false);
  TemporaryRef<TextureClient> GetTextureClientWithAutoRecycle() { return GetTextureClient(true); }

  void ReturnTextureClient(TextureClient *aClient);

  void ShrinkToMinimumSize();

  void Clear();

private:
  
  
  static const uint32_t sMinCacheSize = 16;

  static void ShrinkCallback(nsITimer *aTimer, void *aClosure);
  static void RecycleCallback(TextureClient* aClient, void* aClosure);
  static void WaitForCompositorRecycleCallback(TextureClient* aClient, void* aClosure);

  gfx::SurfaceFormat mFormat;
  gfx::IntSize mSize;

  
  
  uint32_t mMaxTextureClients;

  
  
  uint32_t mShrinkTimeoutMsec;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  std::stack<RefPtr<TextureClient> > mAvailableTextureClients;
  std::list<RefPtr<TextureClient> > mOutstandingTextureClients;

  nsRefPtr<nsITimer> mTimer;
  RefPtr<ISurfaceAllocator> mSurfaceAllocator;
};

}
}
#endif 
