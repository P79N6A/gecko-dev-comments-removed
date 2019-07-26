




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

class TextureClientPool : public RefCounted<TextureClientPool>
{
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(TextureClientPool)
  TextureClientPool(gfx::SurfaceFormat aFormat, gfx::IntSize aSize,
                    ISurfaceAllocator *aAllocator);

  








  TemporaryRef<TextureClient> GetTextureClient();

  



  void ReturnTextureClient(TextureClient *aClient);

  



  void ReturnTextureClientDeferred(TextureClient *aClient);

  



  void ShrinkToMaximumSize();

  



  void ShrinkToMinimumSize();

  



  void ReturnDeferredClients();

  



  void ReportClientLost() { mOutstandingClients--; }

  



  void Clear();

private:
  
  
  static const uint32_t sShrinkTimeout = 1000;

  
  
  static const uint32_t sMinCacheSize = 0;

  
  
  static const uint32_t sMaxTextureClients = 50;

  gfx::SurfaceFormat mFormat;
  gfx::IntSize mSize;

  uint32_t mOutstandingClients;

  std::stack<RefPtr<TextureClient> > mTextureClients;
  std::stack<RefPtr<TextureClient> > mTextureClientsDeferred;
  nsRefPtr<nsITimer> mTimer;
  RefPtr<ISurfaceAllocator> mSurfaceAllocator;
};

}
}
#endif 
