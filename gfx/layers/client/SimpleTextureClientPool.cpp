




#include "SimpleTextureClientPool.h"
#include "CompositableClient.h"
#include "mozilla/layers/ISurfaceAllocator.h"

#include "gfxPrefs.h"

#include "nsComponentManagerUtils.h"

#if 0
#define RECYCLE_LOG(...) printf_stderr(__VA_ARGS__)
#else
#define RECYCLE_LOG(...) do { } while (0)
#endif

namespace mozilla {
namespace layers {

using gfx::SurfaceFormat;

 void
SimpleTextureClientPool::ShrinkCallback(nsITimer *aTimer, void *aClosure)
{
  static_cast<SimpleTextureClientPool*>(aClosure)->ShrinkToMinimumSize();
}

 void
SimpleTextureClientPool::RecycleCallback(TextureClient* aClient, void* aClosure)
{
  SimpleTextureClientPool* pool =
    static_cast<SimpleTextureClientPool*>(aClosure);

  aClient->ClearRecycleCallback();
  pool->ReturnTextureClient(aClient);
}

 void
SimpleTextureClientPool::WaitForCompositorRecycleCallback(TextureClient* aClient, void* aClosure)
{
  
  
  
  aClient->WaitForCompositorRecycle();
  aClient->SetRecycleCallback(SimpleTextureClientPool::RecycleCallback, aClosure);
}

SimpleTextureClientPool::SimpleTextureClientPool(gfx::SurfaceFormat aFormat, gfx::IntSize aSize,
                                                 ISurfaceAllocator *aAllocator)
  : mFormat(aFormat)
  , mSize(aSize)
  , mSurfaceAllocator(aAllocator)
{
  mTimer = do_CreateInstance("@mozilla.org/timer;1");
}

TemporaryRef<TextureClient>
SimpleTextureClientPool::GetTextureClient(bool aAutoRecycle)
{
  
  RefPtr<TextureClient> textureClient;
  if (mAvailableTextureClients.size()) {
    textureClient = mAvailableTextureClients.top();
    textureClient->WaitReleaseFence();
    mAvailableTextureClients.pop();
    RECYCLE_LOG("%s Skip allocate (%i left), returning %p\n", (mFormat == SurfaceFormat::B8G8R8A8?"poolA":"poolX"), mAvailableTextureClients.size(), textureClient.get());

  } else {
    
    if (gfxPrefs::ForceShmemTiles()) {
      textureClient = TextureClient::CreateBufferTextureClient(mSurfaceAllocator,
        mFormat, TextureFlags::IMMEDIATE_UPLOAD | TextureFlags::RECYCLE, gfx::BackendType::NONE);
    } else {
      textureClient = TextureClient::CreateTextureClientForDrawing(mSurfaceAllocator,
        mFormat, TextureFlags::DEFAULT | TextureFlags::RECYCLE, gfx::BackendType::NONE, mSize);
    }
    if (!textureClient->AllocateForSurface(mSize, ALLOC_DEFAULT)) {
      NS_WARNING("TextureClient::AllocateForSurface failed!");
    }
    RECYCLE_LOG("%s Must allocate (0 left), returning %p\n", (mFormat == SurfaceFormat::B8G8R8A8?"poolA":"poolX"), textureClient.get());
  }

  if (aAutoRecycle) {
    mOutstandingTextureClients.push_back(textureClient);
    textureClient->SetRecycleCallback(SimpleTextureClientPool::WaitForCompositorRecycleCallback, this);
  }

  return textureClient;
}

void
SimpleTextureClientPool::ReturnTextureClient(TextureClient *aClient)
{
  if (!aClient) {
    return;
  }

  
  if (mAvailableTextureClients.size() < sMaxTextureClients) {
    mAvailableTextureClients.push(aClient);
    RECYCLE_LOG("%s recycled %p (have %d)\n", (mFormat == SurfaceFormat::B8G8R8A8?"poolA":"poolX"), aClient, mAvailableTextureClients.size());
  } else {
    RECYCLE_LOG("%s did not recycle %p (have %d)\n", (mFormat == SurfaceFormat::B8G8R8A8?"poolA":"poolX"), aClient, mAvailableTextureClients.size());
  }

  
  
  if (mAvailableTextureClients.size() > sMinCacheSize) {
    mTimer->InitWithFuncCallback(SimpleTextureClientPool::ShrinkCallback, this, sShrinkTimeout,
                                 nsITimer::TYPE_ONE_SHOT);
  }

  mOutstandingTextureClients.remove(aClient);
}

void
SimpleTextureClientPool::ShrinkToMinimumSize()
{
  RECYCLE_LOG("%s ShrinkToMinimumSize, removing %d clients", (mFormat == SurfaceFormat::B8G8R8A8?"poolA":"poolX"), mAvailableTextureClients.size() > sMinCacheSize ? mAvailableTextureClients.size() - sMinCacheSize : 0);

  mTimer->Cancel();

  while (mAvailableTextureClients.size() > sMinCacheSize) {
    mAvailableTextureClients.pop();
  }
}

void
SimpleTextureClientPool::Clear()
{
  while (!mAvailableTextureClients.empty()) {
    mAvailableTextureClients.pop();
  }
}

}
}
