




#include "TextureClientPool.h"
#include "CompositableClient.h"
#include "mozilla/layers/ISurfaceAllocator.h"

#include "gfxPrefs.h"

#include "nsComponentManagerUtils.h"

namespace mozilla {
namespace layers {

static void
ShrinkCallback(nsITimer *aTimer, void *aClosure)
{
  static_cast<TextureClientPool*>(aClosure)->ShrinkToMinimumSize();
}

TextureClientPool::TextureClientPool(gfx::SurfaceFormat aFormat,
                                     gfx::IntSize aSize,
                                     uint32_t aMaxTextureClients,
                                     uint32_t aShrinkTimeoutMsec,
                                     ISurfaceAllocator *aAllocator)
  : mFormat(aFormat)
  , mSize(aSize)
  , mMaxTextureClients(aMaxTextureClients)
  , mShrinkTimeoutMsec(aShrinkTimeoutMsec)
  , mOutstandingClients(0)
  , mSurfaceAllocator(aAllocator)
{
  mTimer = do_CreateInstance("@mozilla.org/timer;1");
  if (aFormat == gfx::SurfaceFormat::UNKNOWN) {
    gfxWarning() << "Creating texture pool for SurfaceFormat::UNKNOWN format";
  }
}

TextureClientPool::~TextureClientPool()
{
  mTimer->Cancel();
}

#ifdef GFX_DEBUG_TRACK_CLIENTS_IN_POOL
static bool TestClientPool(const char* what,
                           TextureClient* aClient,
                           TextureClientPool* aPool)
{
  TextureClientPool* actual = aClient->mPoolTracker;
  bool ok = (actual == aPool);
  if (ok) {
    ok = (aClient->GetFormat() == aPool->GetFormat());
  }

  if (!ok) {
    if (actual) {
      gfxCriticalError() << "Pool error(" << what << "): "
                   << aPool << "-" << aPool->GetFormat() << ", "
                   << actual << "-" << actual->GetFormat() << ", "
                   << aClient->GetFormat();
      MOZ_CRASH("Crashing with actual");
    } else {
      gfxCriticalError() << "Pool error(" << what << "): "
                   << aPool << "-" << aPool->GetFormat() << ", nullptr, "
                   << aClient->GetFormat();
      MOZ_CRASH("Crashing without actual");
    }
  }
  return ok;
}
#endif

TemporaryRef<TextureClient>
TextureClientPool::GetTextureClient()
{
  
  RefPtr<TextureClient> textureClient;
  if (mTextureClients.size()) {
    mOutstandingClients++;
    textureClient = mTextureClients.top();
    mTextureClients.pop();
#ifdef GFX_DEBUG_TRACK_CLIENTS_IN_POOL
    DebugOnly<bool> ok = TestClientPool("fetch", textureClient, this);
    MOZ_ASSERT(ok);
#endif
    return textureClient;
  }

  
  
  ShrinkToMaximumSize();

  
  if (gfxPrefs::ForceShmemTiles()) {
    
    textureClient = TextureClient::CreateForRawBufferAccess(mSurfaceAllocator,
      mFormat, mSize, gfx::BackendType::NONE,
      TextureFlags::IMMEDIATE_UPLOAD, ALLOC_DEFAULT);
  } else {
    textureClient = TextureClient::CreateForDrawing(mSurfaceAllocator,
      mFormat, mSize, gfx::BackendType::NONE, TextureFlags::IMMEDIATE_UPLOAD);
  }

  mOutstandingClients++;
#ifdef GFX_DEBUG_TRACK_CLIENTS_IN_POOL
  textureClient->mPoolTracker = this;
#endif
  return textureClient;
}

void
TextureClientPool::ReturnTextureClient(TextureClient *aClient)
{
  if (!aClient) {
    return;
  }
#ifdef GFX_DEBUG_TRACK_CLIENTS_IN_POOL
  DebugOnly<bool> ok = TestClientPool("return", aClient, this);
  MOZ_ASSERT(ok);
#endif
  
  MOZ_ASSERT(mOutstandingClients > mTextureClientsDeferred.size());
  mOutstandingClients--;
  mTextureClients.push(aClient);

  
  ShrinkToMaximumSize();

  
  
  if (mTextureClients.size() > sMinCacheSize) {
    mTimer->InitWithFuncCallback(ShrinkCallback, this, mShrinkTimeoutMsec,
                                 nsITimer::TYPE_ONE_SHOT);
  }
}

void
TextureClientPool::ReturnTextureClientDeferred(TextureClient *aClient)
{
  if (!aClient) {
    return;
  }
#ifdef GFX_DEBUG_TRACK_CLIENTS_IN_POOL
  DebugOnly<bool> ok = TestClientPool("defer", aClient, this);
  MOZ_ASSERT(ok);
#endif
  mTextureClientsDeferred.push(aClient);
  ShrinkToMaximumSize();
}

void
TextureClientPool::ShrinkToMaximumSize()
{
  uint32_t totalClientsOutstanding = mTextureClients.size() + mOutstandingClients;

  
  
  
  
  while (totalClientsOutstanding > mMaxTextureClients) {
    if (mTextureClientsDeferred.size()) {
      MOZ_ASSERT(mOutstandingClients > 0);
      mOutstandingClients--;
      mTextureClientsDeferred.pop();
    } else {
      if (!mTextureClients.size()) {
        
        
        
        
        break;
      }
      mTextureClients.pop();
    }
    totalClientsOutstanding--;
  }
}

void
TextureClientPool::ShrinkToMinimumSize()
{
  while (mTextureClients.size() > sMinCacheSize) {
    mTextureClients.pop();
  }
}

void
TextureClientPool::ReturnDeferredClients()
{
  while (!mTextureClientsDeferred.empty()) {
    mTextureClients.push(mTextureClientsDeferred.top());
    mTextureClientsDeferred.pop();

    MOZ_ASSERT(mOutstandingClients > 0);
    mOutstandingClients--;
  }
  ShrinkToMaximumSize();

  
  
  if (mTextureClients.size() > sMinCacheSize) {
    mTimer->InitWithFuncCallback(ShrinkCallback, this, mShrinkTimeoutMsec,
                                 nsITimer::TYPE_ONE_SHOT);
  }
}

void
TextureClientPool::Clear()
{
  while (!mTextureClients.empty()) {
    mTextureClients.pop();
  }
  while (!mTextureClientsDeferred.empty()) {
    MOZ_ASSERT(mOutstandingClients > 0);
    mOutstandingClients--;
    mTextureClientsDeferred.pop();
  }
}

}
}
