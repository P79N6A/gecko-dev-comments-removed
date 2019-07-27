




#include "TextureClientPool.h"
#include "CompositableClient.h"
#include "mozilla/layers/ISurfaceAllocator.h"

#include "gfxPrefs.h"

#include "nsComponentManagerUtils.h"

#define TCP_LOG(...)


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
  TCP_LOG("TexturePool %p created with max size %u and timeout %u\n",
      this, mMaxTextureClients, aShrinkTimeoutMsec);
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
  if (!aClient || !aPool) {
    return false;
  }

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

already_AddRefed<TextureClient>
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
    TCP_LOG("TexturePool %p giving %p from pool; size %u outstanding %u\n",
        this, textureClient.get(), mTextureClients.size(), mOutstandingClients);
    return textureClient.forget();
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
  if (textureClient) {
    textureClient->mPoolTracker = this;
  }
#endif
  TCP_LOG("TexturePool %p giving new %p; size %u outstanding %u\n",
      this, textureClient.get(), mTextureClients.size(), mOutstandingClients);
  return textureClient.forget();
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
  TCP_LOG("TexturePool %p had client %p returned; size %u outstanding %u\n",
      this, aClient, mTextureClients.size(), mOutstandingClients);

  
  ShrinkToMaximumSize();

  
  
  if (mTextureClients.size() > sMinCacheSize) {
    TCP_LOG("TexturePool %p scheduling a shrink-to-min-size\n", this);
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
  TCP_LOG("TexturePool %p had client %p defer-returned, size %u outstanding %u\n",
      this, aClient, mTextureClientsDeferred.size(), mOutstandingClients);
  ShrinkToMaximumSize();
}

void
TextureClientPool::ShrinkToMaximumSize()
{
  uint32_t totalClientsOutstanding = mTextureClients.size() + mOutstandingClients;
  TCP_LOG("TexturePool %p shrinking to max size %u; total outstanding %u\n",
      this, mMaxTextureClients, totalClientsOutstanding);

  
  
  
  
  while (totalClientsOutstanding > mMaxTextureClients) {
    if (mTextureClientsDeferred.size()) {
      MOZ_ASSERT(mOutstandingClients > 0);
      mOutstandingClients--;
      TCP_LOG("TexturePool %p dropped deferred client %p; %u remaining\n",
          this, mTextureClientsDeferred.top().get(),
          mTextureClientsDeferred.size() - 1);
      mTextureClientsDeferred.pop();
    } else {
      if (!mTextureClients.size()) {
        
        
        
        
        TCP_LOG("TexturePool %p encountering pathological case!\n", this);
        break;
      }
      TCP_LOG("TexturePool %p dropped non-deferred client %p; %u remaining\n",
          this, mTextureClients.top().get(), mTextureClients.size() - 1);
      mTextureClients.pop();
    }
    totalClientsOutstanding--;
  }
}

void
TextureClientPool::ShrinkToMinimumSize()
{
  TCP_LOG("TexturePool %p shrinking to minimum size %u\n", this, sMinCacheSize);
  while (mTextureClients.size() > sMinCacheSize) {
    TCP_LOG("TexturePool %p popped %p; shrunk to %u\n",
        this, mTextureClients.top().get(), mTextureClients.size() - 1);
    mTextureClients.pop();
  }
}

void
TextureClientPool::ReturnDeferredClients()
{
  TCP_LOG("TexturePool %p returning %u deferred clients to pool\n",
      this, mTextureClientsDeferred.size());
  while (!mTextureClientsDeferred.empty()) {
    mTextureClients.push(mTextureClientsDeferred.top());
    mTextureClientsDeferred.pop();

    MOZ_ASSERT(mOutstandingClients > 0);
    mOutstandingClients--;
  }
  ShrinkToMaximumSize();

  
  
  if (mTextureClients.size() > sMinCacheSize) {
    TCP_LOG("TexturePool %p kicking off shrink-to-min timer\n", this);
    mTimer->InitWithFuncCallback(ShrinkCallback, this, mShrinkTimeoutMsec,
                                 nsITimer::TYPE_ONE_SHOT);
  }
}

void
TextureClientPool::ReportClientLost()
{
  MOZ_ASSERT(mOutstandingClients > mTextureClientsDeferred.size());
  mOutstandingClients--;
  TCP_LOG("TexturePool %p getting report client lost; down to %u outstanding\n",
      this, mOutstandingClients);
}

void
TextureClientPool::Clear()
{
  TCP_LOG("TexturePool %p getting cleared\n", this);
  while (!mTextureClients.empty()) {
    TCP_LOG("TexturePool %p releasing client %p\n",
        this, mTextureClients.top().get());
    mTextureClients.pop();
  }
  while (!mTextureClientsDeferred.empty()) {
    MOZ_ASSERT(mOutstandingClients > 0);
    mOutstandingClients--;
    TCP_LOG("TexturePool %p releasing deferred client %p\n",
        this, mTextureClientsDeferred.top().get());
    mTextureClientsDeferred.pop();
  }
}

}
}
