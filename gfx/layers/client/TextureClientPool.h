




#ifndef MOZILLA_GFX_TEXTURECLIENTPOOL_H
#define MOZILLA_GFX_TEXTURECLIENTPOOL_H

#include "mozilla/gfx/Types.h"
#include "mozilla/gfx/Point.h"
#include "mozilla/RefPtr.h"
#include "TextureClient.h"
#include "nsITimer.h"
#include <stack>

namespace mozilla {
namespace layers {

class ISurfaceAllocator;

class TextureClientPool MOZ_FINAL
{
  ~TextureClientPool();

public:
  NS_INLINE_DECL_REFCOUNTING(TextureClientPool)

  TextureClientPool(gfx::SurfaceFormat aFormat, gfx::IntSize aSize,
                    uint32_t aMaxTextureClients,
                    uint32_t aShrinkTimeoutMsec,
                    ISurfaceAllocator *aAllocator);

  








  TemporaryRef<TextureClient> GetTextureClient();

  



  void ReturnTextureClient(TextureClient *aClient);

  



  void ReturnTextureClientDeferred(TextureClient *aClient);

  



  void ShrinkToMaximumSize();

  



  void ShrinkToMinimumSize();

  



  void ReturnDeferredClients();

  



  void ReportClientLost() {
    MOZ_ASSERT(mOutstandingClients > mTextureClientsDeferred.size());
    mOutstandingClients--;
  }

  



  void Clear();

  gfx::SurfaceFormat GetFormat() { return mFormat; }

private:
  
  
  static const uint32_t sMinCacheSize = 0;

  
  gfx::SurfaceFormat mFormat;

  
  gfx::IntSize mSize;

  
  
  uint32_t mMaxTextureClients;

  
  
  uint32_t mShrinkTimeoutMsec;

  
  
  
  uint32_t mOutstandingClients;

  
  
  
  std::stack<RefPtr<TextureClient> > mTextureClients;
  std::stack<RefPtr<TextureClient> > mTextureClientsDeferred;
  nsRefPtr<nsITimer> mTimer;
  RefPtr<ISurfaceAllocator> mSurfaceAllocator;
};

}
}
#endif 
