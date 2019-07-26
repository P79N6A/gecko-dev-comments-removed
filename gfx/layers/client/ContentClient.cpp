




#include "mozilla/layers/ContentClient.h"
#include "mozilla/gfx/2D.h"
#include "BasicThebesLayer.h"
#include "nsIWidget.h"
#include "gfxUtils.h"
#include "gfxPlatform.h"

namespace mozilla {

using namespace gfx;

namespace layers {

 TemporaryRef<ContentClient>
ContentClient::CreateContentClient(LayersBackend aParentBackend,
                                   CompositableForwarder* aForwarder)
{
  if (aParentBackend != LAYERS_OPENGL) {
    return nullptr;
  }
  if (ShadowLayerManager::SupportsDirectTexturing() ||
      PR_GetEnv("MOZ_FORCE_DOUBLE_BUFFERING")) {
    return new ContentClientDoubleBuffered(aForwarder);
  }
  return new ContentClientSingleBuffered(aForwarder);
}

ContentClientBasic::ContentClientBasic(CompositableForwarder* aForwarder,
                                       BasicLayerManager* aManager)
: ContentClient(aForwarder), mManager(aManager)
{}

already_AddRefed<gfxASurface>
ContentClientBasic::CreateBuffer(ContentType aType,
                                 const nsIntRect& aRect,
                                 uint32_t aFlags)
{
  nsRefPtr<gfxASurface> referenceSurface = GetBuffer();
  if (!referenceSurface) {
    gfxContext* defaultTarget = mManager->GetDefaultTarget();
    if (defaultTarget) {
      referenceSurface = defaultTarget->CurrentSurface();
    } else {
      nsIWidget* widget = mManager->GetRetainerWidget();
      if (!widget || !(referenceSurface = widget->GetThebesSurface())) {
        referenceSurface = mManager->GetTarget()->CurrentSurface();
      }
    }
  }
  return referenceSurface->CreateSimilarSurface(
    aType, gfxIntSize(aRect.width, aRect.height));
}

TemporaryRef<DrawTarget>
ContentClientBasic::CreateDTBuffer(ContentType aType,
                                 const nsIntRect& aRect,
                                 uint32_t aFlags)
{
  NS_RUNTIMEABORT("ContentClientBasic does not support Moz2D drawing yet!");
  
  return nullptr;
}

void
ContentClientRemote::DestroyBuffers()
{
  if (!mTextureClient) {
    return;
  }

  MOZ_ASSERT(mTextureClient->GetAccessMode() == TextureClient::ACCESS_READ_WRITE);
  mTextureClient = nullptr;

  DestroyFrontBuffer();

  mForwarder->DestroyThebesBuffer(this);
}

void
ContentClientRemote::BeginPaint()
{
  
  
  if (mTextureClient) {
    SetTextureClientForBuffer(mTextureClient);
  }
}

void
ContentClientRemote::EndPaint()
{
  
  
  SetTextureClientForBuffer(nullptr);
  mOldTextures.Clear();

  if (mTextureClient) {
    mTextureClient->Unlock();
  }
}

TemporaryRef<DrawTarget>
ContentClientRemote::CreateDTBuffer(ContentType aType,
                                    const nsIntRect& aRect,
                                    uint32_t aFlags)
{
  NS_ABORT_IF_FALSE(!mIsNewBuffer,
                    "Bad! Did we create a buffer twice without painting?");

  mIsNewBuffer = true;

  if (mTextureClient) {
    mOldTextures.AppendElement(mTextureClient);
    DestroyBuffers();
  }
  mTextureInfo.mTextureFlags = aFlags | HostRelease;
  mTextureClient = CreateTextureClient(TEXTURE_CONTENT);

  mContentType = aType;
  mSize = gfx::IntSize(aRect.width, aRect.height);
  mTextureClient->EnsureAllocated(mSize, mContentType);
  MOZ_ASSERT(IsSurfaceDescriptorValid(*mTextureClient->GetDescriptor()));

  CreateFrontBufferAndNotify(aRect);

  RefPtr<DrawTarget> ret = mTextureClient->LockDrawTarget();
  return ret.forget();
}

already_AddRefed<gfxASurface>
ContentClientRemote::CreateBuffer(ContentType aType,
                                  const nsIntRect& aRect,
                                  uint32_t aFlags)
{
  NS_ABORT_IF_FALSE(!mIsNewBuffer,
                    "Bad! Did we create a buffer twice without painting?");

  mIsNewBuffer = true;

  if (mTextureClient) {
    mOldTextures.AppendElement(mTextureClient);
    DestroyBuffers();
  }
  mTextureInfo.mTextureFlags = aFlags | HostRelease;
  mTextureClient = CreateTextureClient(TEXTURE_CONTENT);

  mContentType = aType;
  mSize = gfx::IntSize(aRect.width, aRect.height);
  mTextureClient->EnsureAllocated(mSize, mContentType);
  MOZ_ASSERT(IsSurfaceDescriptorValid(*mTextureClient->GetDescriptor()));

  CreateFrontBufferAndNotify(aRect);

  nsRefPtr<gfxASurface> ret = mTextureClient->LockSurface();
  return ret.forget();
}

nsIntRegion
ContentClientRemote::GetUpdatedRegion(const nsIntRegion& aRegionToDraw,
                                      const nsIntRegion& aVisibleRegion,
                                      bool aDidSelfCopy)
{
  nsIntRegion updatedRegion;
  if (mIsNewBuffer || aDidSelfCopy) {
    
    
    
    
    
    
    
    updatedRegion = aVisibleRegion;
    mIsNewBuffer = false;
  } else {
    updatedRegion = aRegionToDraw;
  }

  NS_ASSERTION(BufferRect().Contains(aRegionToDraw.GetBounds()),
               "Update outside of buffer rect!");
  NS_ABORT_IF_FALSE(mTextureClient, "should have a back buffer by now");

  return updatedRegion;
}

void
ContentClientRemote::Updated(const nsIntRegion& aRegionToDraw,
                             const nsIntRegion& aVisibleRegion,
                             bool aDidSelfCopy)
{
  nsIntRegion updatedRegion = GetUpdatedRegion(aRegionToDraw,
                                               aVisibleRegion,
                                               aDidSelfCopy);

  MOZ_ASSERT(mTextureClient);
  mTextureClient->SetAccessMode(TextureClient::ACCESS_NONE);
  LockFrontBuffer();
  mForwarder->UpdateTextureRegion(this,
                                  ThebesBufferData(BufferRect(),
                                                   BufferRotation()),
                                  updatedRegion);
}

void
ContentClientRemote::SwapBuffers(const nsIntRegion& aFrontUpdatedRegion)
{
  MOZ_ASSERT(mTextureClient->GetAccessMode() == TextureClient::ACCESS_NONE);
  MOZ_ASSERT(mTextureClient);

  mFrontAndBackBufferDiffer = true;
  mTextureClient->SetAccessMode(TextureClient::ACCESS_READ_WRITE);
}

void
ContentClientRemote::SetBackingBuffer(gfxASurface* aBuffer,
                                      const nsIntRect& aRect,
                                      const nsIntPoint& aRotation)
{
#ifdef DEBUG
  gfxIntSize prevSize = gfxIntSize(BufferRect().width, BufferRect().height);
  gfxIntSize newSize = aBuffer->GetSize();
  NS_ABORT_IF_FALSE(newSize == prevSize,
                    "Swapped-in buffer size doesn't match old buffer's!");
#endif
  nsRefPtr<gfxASurface> oldBuffer;
  oldBuffer = SetBuffer(aBuffer, aRect, aRotation);
}

ContentClientDoubleBuffered::~ContentClientDoubleBuffered()
{
  if (mTextureClient) {
    MOZ_ASSERT(mFrontClient);
    mTextureClient->SetDescriptor(SurfaceDescriptor());
    mFrontClient->SetDescriptor(SurfaceDescriptor());
  }
}

void
ContentClientDoubleBuffered::CreateFrontBufferAndNotify(const nsIntRect& aBufferRect)
{
  mFrontClient = CreateTextureClient(TEXTURE_CONTENT);
  mFrontClient->EnsureAllocated(mSize, mContentType);

  mFrontBufferRect = aBufferRect;
  mFrontBufferRotation = nsIntPoint();

  mForwarder->CreatedDoubleBuffer(this,
                                  *mFrontClient->GetDescriptor(),
                                  *mTextureClient->GetDescriptor(),
                                  mTextureInfo);
}

void
ContentClientDoubleBuffered::DestroyFrontBuffer()
{
  MOZ_ASSERT(mFrontClient);
  MOZ_ASSERT(mFrontClient->GetAccessMode() != TextureClient::ACCESS_NONE);

  mFrontClient = nullptr;
}

void
ContentClientDoubleBuffered::LockFrontBuffer()
{
  MOZ_ASSERT(mFrontClient);
  mFrontClient->SetAccessMode(TextureClient::ACCESS_NONE);
}

void
ContentClientDoubleBuffered::SwapBuffers(const nsIntRegion& aFrontUpdatedRegion)
{
  mFrontUpdatedRegion = aFrontUpdatedRegion;

  RefPtr<TextureClient> oldBack = mTextureClient;
  mTextureClient = mFrontClient;
  mFrontClient = oldBack;

  nsIntRect oldBufferRect = mBufferRect;
  mBufferRect = mFrontBufferRect;
  mFrontBufferRect = oldBufferRect;

  nsIntPoint oldBufferRotation = mBufferRotation;
  mBufferRotation = mFrontBufferRotation;
  mFrontBufferRotation = oldBufferRotation;

  MOZ_ASSERT(mFrontClient);
  mFrontClient->SetAccessMode(TextureClient::ACCESS_READ_ONLY);

  ContentClientRemote::SwapBuffers(aFrontUpdatedRegion);
}

struct AutoTextureClient {
  AutoTextureClient()
    : mTexture(nullptr)
  {}
  ~AutoTextureClient()
  {
    if (mTexture) {
      mTexture->Unlock();
    }
  }
  gfxASurface* GetSurface(TextureClient* aTexture)
  {
    MOZ_ASSERT(!mTexture);
    mTexture = aTexture;
    return mTexture->LockSurface();
  }
  DrawTarget* GetDrawTarget(TextureClient* aTexture)
  {
    MOZ_ASSERT(!mTexture);
    mTexture = aTexture;
    return mTexture->LockDrawTarget();
  }
private:
  TextureClient* mTexture;
};

void
ContentClientDoubleBuffered::SyncFrontBufferToBackBuffer()
{
  if (!mFrontAndBackBufferDiffer) {
    return;
  }
  MOZ_ASSERT(mFrontClient);
  MOZ_ASSERT(mFrontClient->GetAccessMode() == TextureClient::ACCESS_READ_ONLY);

  MOZ_LAYERS_LOG(("BasicShadowableThebes(%p): reading back <x=%d,y=%d,w=%d,h=%d>",
                  this,
                  mFrontUpdatedRegion.GetBounds().x,
                  mFrontUpdatedRegion.GetBounds().y,
                  mFrontUpdatedRegion.GetBounds().width,
                  mFrontUpdatedRegion.GetBounds().height));

  nsIntRegion updateRegion = mFrontUpdatedRegion;

  int32_t xBoundary = mBufferRect.XMost() - mBufferRotation.x;
  int32_t yBoundary = mBufferRect.YMost() - mBufferRotation.y;

  
  bool needFullCopy = (xBoundary < updateRegion.GetBounds().XMost() &&
                       xBoundary > updateRegion.GetBounds().x) ||
                      (yBoundary < updateRegion.GetBounds().YMost() &&
                       yBoundary > updateRegion.GetBounds().y);
  
  
  
  
  

  if (needFullCopy) {
    
    
    
    
    mBufferRect.MoveTo(mFrontBufferRect.TopLeft());
    mBufferRotation = nsIntPoint();
    updateRegion = mBufferRect;
  } else {
    mBufferRect = mFrontBufferRect;
    mBufferRotation = mFrontBufferRotation;
  }
 
  AutoTextureClient autoTextureFront;
  if (gfxPlatform::GetPlatform()->SupportsAzureContent()) {
    RotatedBuffer frontBuffer(autoTextureFront.GetDrawTarget(mFrontClient),
                              mFrontBufferRect,
                              mFrontBufferRotation);
    UpdateDestinationFrom(frontBuffer, updateRegion);
  } else {
    RotatedBuffer frontBuffer(autoTextureFront.GetSurface(mFrontClient),
                              mFrontBufferRect,
                              mFrontBufferRotation);
    UpdateDestinationFrom(frontBuffer, updateRegion);
  }

  mIsNewBuffer = false;
  mFrontAndBackBufferDiffer = false;
}

void
ContentClientDoubleBuffered::UpdateDestinationFrom(const RotatedBuffer& aSource,
                                                   const nsIntRegion& aUpdateRegion)
{
  nsRefPtr<gfxContext> destCtx =
    GetContextForQuadrantUpdate(aUpdateRegion.GetBounds());
  destCtx->SetOperator(gfxContext::OPERATOR_SOURCE);

  bool isClippingCheap = IsClippingCheap(destCtx, aUpdateRegion);
  if (isClippingCheap) {
    gfxUtils::ClipToRegion(destCtx, aUpdateRegion);
  }

  if (gfxPlatform::GetPlatform()->SupportsAzureContent()) {
    MOZ_ASSERT(!destCtx->IsCairo());

    if (destCtx->GetDrawTarget()->GetFormat() == FORMAT_B8G8R8A8) {
      destCtx->GetDrawTarget()->ClearRect(Rect(0, 0, mFrontBufferRect.width, mFrontBufferRect.height));
    }
    aSource.DrawBufferWithRotation(destCtx->GetDrawTarget());
  } else {
    aSource.DrawBufferWithRotation(destCtx);
  }
}

ContentClientSingleBuffered::~ContentClientSingleBuffered()
{
  if (mTextureClient) {
    mTextureClient->SetDescriptor(SurfaceDescriptor());
  }
}

void
ContentClientSingleBuffered::CreateFrontBufferAndNotify(const nsIntRect& aBufferRect)
{
  mForwarder->CreatedSingleBuffer(this,
                                  *mTextureClient->GetDescriptor(),
                                  mTextureInfo);
}

void
ContentClientSingleBuffered::SyncFrontBufferToBackBuffer()
{
  if (!mFrontAndBackBufferDiffer) {
    return;
  }

  gfxASurface* backBuffer = GetBuffer();
  if (!backBuffer && mTextureClient) {
    backBuffer = mTextureClient->LockSurface();
  }

  nsRefPtr<gfxASurface> oldBuffer;
  oldBuffer = SetBuffer(backBuffer,
                        mBufferRect,
                        mBufferRotation);

  mIsNewBuffer = false;
  mFrontAndBackBufferDiffer = false;
}

}
}
