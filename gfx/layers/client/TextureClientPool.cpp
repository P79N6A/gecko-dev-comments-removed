




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
                                     ISurfaceAllocator *aAllocator)
  : mFormat(aFormat)
  , mSize(aSize)
  , mOutstandingClients(0)
  , mSurfaceAllocator(aAllocator)
{
  mTimer = do_CreateInstance("@mozilla.org/timer;1");
}

TemporaryRef<TextureClient>
TextureClientPool::GetTextureClient()
{
  mOutstandingClients++;

  
  RefPtr<TextureClient> textureClient;
  if (mTextureClients.size()) {
    textureClient = mTextureClients.top();
    mTextureClients.pop();
    return textureClient;
  }

  
  
  ShrinkToMaximumSize();

  
  if (gfxPrefs::ForceShmemTiles()) {
    textureClient = TextureClient::CreateBufferTextureClient(mSurfaceAllocator, mFormat, TEXTURE_IMMEDIATE_UPLOAD);
  } else {
    textureClient = TextureClient::CreateTextureClientForDrawing(mSurfaceAllocator, mFormat, TEXTURE_IMMEDIATE_UPLOAD, mSize);
  }
  textureClient->AsTextureClientDrawTarget()->AllocateForSurface(mSize, ALLOC_DEFAULT);

  return textureClient;
}

void
TextureClientPool::ReturnTextureClient(TextureClient *aClient)
{
  if (!aClient) {
    return;
  }
  MOZ_ASSERT(mOutstandingClients);
  mOutstandingClients--;

  
  mTextureClients.push(aClient);
  ShrinkToMaximumSize();

  
  
  if (mTextureClients.size() > sMinCacheSize) {
    mTimer->InitWithFuncCallback(ShrinkCallback, this, sShrinkTimeout,
                                 nsITimer::TYPE_ONE_SHOT);
  }
}

void
TextureClientPool::ReturnTextureClientDeferred(TextureClient *aClient)
{
  mTextureClientsDeferred.push(aClient);
  ShrinkToMaximumSize();
}

void
TextureClientPool::ShrinkToMaximumSize()
{
  uint32_t totalClientsOutstanding = mTextureClients.size() + mOutstandingClients;

  
  
  
  
  while (totalClientsOutstanding > sMaxTextureClients) {
    if (mTextureClientsDeferred.size()) {
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
    ReturnTextureClient(mTextureClientsDeferred.top());
    mTextureClientsDeferred.pop();
  }
}

void
TextureClientPool::Clear()
{
  while (!mTextureClients.empty()) {
    mTextureClients.pop();
  }
  while (!mTextureClientsDeferred.empty()) {
    mOutstandingClients--;
    mTextureClientsDeferred.pop();
  }
}

}
}
