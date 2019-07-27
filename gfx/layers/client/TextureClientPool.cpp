




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

TextureClientPool::TextureClientPool(gfx::SurfaceFormat aFormat, gfx::IntSize aSize,
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
}

TextureClientPool::~TextureClientPool()
{
  mTimer->Cancel();
}

TemporaryRef<TextureClient>
TextureClientPool::GetTextureClient()
{
  
  RefPtr<TextureClient> textureClient;
  if (mTextureClients.size()) {
    mOutstandingClients++;
    textureClient = mTextureClients.top();
    mTextureClients.pop();
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
  return textureClient;
}

void
TextureClientPool::ReturnTextureClient(TextureClient *aClient)
{
  if (!aClient) {
    return;
  }
  
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
