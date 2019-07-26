




#include "mozilla/layers/ContentClient.h"
#include "BasicLayers.h"                
#include "gfxColor.h"                   
#include "gfxContext.h"                 
#include "gfxPlatform.h"                
#include "gfxPrefs.h"                   
#include "gfxPoint.h"                   
#include "gfxTeeSurface.h"              
#include "gfxUtils.h"                   
#include "ipc/ShadowLayers.h"           
#include "mozilla/ArrayUtils.h"         
#include "mozilla/gfx/2D.h"             
#include "mozilla/gfx/BasePoint.h"      
#include "mozilla/gfx/BaseSize.h"       
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/LayersMessages.h"  
#include "mozilla/layers/LayersTypes.h"
#include "nsAutoPtr.h"                  
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsIWidget.h"                  
#include "prenv.h"                      
#ifdef XP_WIN
#include "gfxWindowsPlatform.h"
#endif
#include "gfx2DGlue.h"

namespace mozilla {

using namespace gfx;

namespace layers {

static TextureFlags TextureFlagsForRotatedContentBufferFlags(uint32_t aBufferFlags)
{
  TextureFlags result = 0;

  if (aBufferFlags & RotatedContentBuffer::BUFFER_COMPONENT_ALPHA) {
    result |= TEXTURE_COMPONENT_ALPHA;
  }

  if (aBufferFlags & RotatedContentBuffer::ALLOW_REPEAT) {
    result |= TEXTURE_ALLOW_REPEAT;
  }

  return result;
}


 TemporaryRef<ContentClient>
ContentClient::CreateContentClient(CompositableForwarder* aForwarder)
{
  LayersBackend backend = aForwarder->GetCompositorBackendType();
  if (backend != LayersBackend::LAYERS_OPENGL &&
      backend != LayersBackend::LAYERS_D3D9 &&
      backend != LayersBackend::LAYERS_D3D11 &&
      backend != LayersBackend::LAYERS_BASIC) {
    return nullptr;
  }

  bool useDoubleBuffering = false;

#ifdef XP_WIN
  if (backend == LayersBackend::LAYERS_D3D11) {
    useDoubleBuffering = !!gfxWindowsPlatform::GetPlatform()->GetD2DDevice();
  } else
#endif
  {
    useDoubleBuffering = (LayerManagerComposite::SupportsDirectTexturing() &&
                         backend != LayersBackend::LAYERS_D3D9) ||
                         backend == LayersBackend::LAYERS_BASIC;
  }

  if (useDoubleBuffering || PR_GetEnv("MOZ_FORCE_DOUBLE_BUFFERING")) {
    return new ContentClientDoubleBuffered(aForwarder);
  }
#ifdef XP_MACOSX
  if (backend == LayersBackend::LAYERS_OPENGL) {
    return new ContentClientIncremental(aForwarder);
  }
#endif
  return new ContentClientSingleBuffered(aForwarder);
}



ContentClientBasic::ContentClientBasic()
  : ContentClient(nullptr)
  , RotatedContentBuffer(ContainsVisibleBounds)
{}

void
ContentClientBasic::CreateBuffer(ContentType aType,
                                 const nsIntRect& aRect,
                                 uint32_t aFlags,
                                 RefPtr<gfx::DrawTarget>* aBlackDT,
                                 RefPtr<gfx::DrawTarget>* aWhiteDT)
{
  MOZ_ASSERT(!(aFlags & BUFFER_COMPONENT_ALPHA));
  gfxImageFormat format =
    gfxPlatform::GetPlatform()->OptimalFormatForContent(aType);

  *aBlackDT = gfxPlatform::GetPlatform()->CreateOffscreenContentDrawTarget(
    IntSize(aRect.width, aRect.height),
    ImageFormatToSurfaceFormat(format));
}

void
ContentClientRemoteBuffer::DestroyBuffers()
{
  if (!mTextureClient) {
    return;
  }

  mOldTextures.AppendElement(mTextureClient);
  mTextureClient = nullptr;
  if (mTextureClientOnWhite) {
    mOldTextures.AppendElement(mTextureClientOnWhite);
    mTextureClientOnWhite = nullptr;
  }

  DestroyFrontBuffer();
}

void
ContentClientRemoteBuffer::BeginPaint()
{
  
  
  if (mTextureClient) {
    SetBufferProvider(mTextureClient);
  }
  if (mTextureClientOnWhite) {
    SetBufferProviderOnWhite(mTextureClientOnWhite);
  }
}

void
ContentClientRemoteBuffer::EndPaint()
{
  
  
  SetBufferProvider(nullptr);
  SetBufferProviderOnWhite(nullptr);
  for (unsigned i = 0; i< mOldTextures.Length(); ++i) {
    if (mOldTextures[i]->IsLocked()) {
      mOldTextures[i]->Unlock();
    }
  }
  mOldTextures.Clear();

  if (mTextureClient && mTextureClient->IsLocked()) {
    mTextureClient->Unlock();
  }
  if (mTextureClientOnWhite && mTextureClientOnWhite->IsLocked()) {
    mTextureClientOnWhite->Unlock();
  }
}

bool
ContentClientRemoteBuffer::CreateAndAllocateTextureClient(RefPtr<TextureClient>& aClient,
                                                          TextureFlags aFlags)
{
  
  aClient = CreateTextureClientForDrawing(mSurfaceFormat,
                                          mTextureInfo.mTextureFlags | aFlags,
                                          gfx::BackendType::NONE,
                                          mSize);
  if (!aClient) {
    return false;
  }

  if (!aClient->AsTextureClientDrawTarget()->AllocateForSurface(mSize, ALLOC_CLEAR_BUFFER)) {
    aClient = CreateTextureClientForDrawing(mSurfaceFormat,
                mTextureInfo.mTextureFlags | TEXTURE_ALLOC_FALLBACK | aFlags,
                gfx::BackendType::NONE,
                mSize);
    if (!aClient) {
      return false;
    }
    if (!aClient->AsTextureClientDrawTarget()->AllocateForSurface(mSize, ALLOC_CLEAR_BUFFER)) {
      NS_WARNING("Could not allocate texture client");
      aClient = nullptr;
      return false;
    }
  }

  NS_WARN_IF_FALSE(aClient->IsValid(), "Created an invalid texture client");
  return true;
}

void
ContentClientRemoteBuffer::BuildTextureClients(SurfaceFormat aFormat,
                                               const nsIntRect& aRect,
                                               uint32_t aFlags)
{
  
  
  
  
  
  
  NS_ABORT_IF_FALSE(!mIsNewBuffer,
                    "Bad! Did we create a buffer twice without painting?");

  mIsNewBuffer = true;

  DestroyBuffers();

  mSurfaceFormat = aFormat;
  mSize = gfx::IntSize(aRect.width, aRect.height);
  mTextureInfo.mTextureFlags = TextureFlagsForRotatedContentBufferFlags(aFlags);

  if (!CreateAndAllocateTextureClient(mTextureClient, TEXTURE_ON_BLACK) ||
      !AddTextureClient(mTextureClient)) {
    AbortTextureClientCreation();
    return;
  }

  if (aFlags & BUFFER_COMPONENT_ALPHA) {
    if (!CreateAndAllocateTextureClient(mTextureClientOnWhite, TEXTURE_ON_WHITE) ||
        !AddTextureClient(mTextureClientOnWhite)) {
      AbortTextureClientCreation();
      return;
    }
    mTextureInfo.mTextureFlags |= TEXTURE_COMPONENT_ALPHA;
  }

  CreateFrontBuffer(aRect);
}

void
ContentClientRemoteBuffer::CreateBuffer(ContentType aType,
                                        const nsIntRect& aRect,
                                        uint32_t aFlags,
                                        RefPtr<gfx::DrawTarget>* aBlackDT,
                                        RefPtr<gfx::DrawTarget>* aWhiteDT)
{
  BuildTextureClients(gfxPlatform::GetPlatform()->Optimal2DFormatForContent(aType), aRect, aFlags);
  if (!mTextureClient) {
    return;
  }

  
  
  DebugOnly<bool> locked = mTextureClient->Lock(OPEN_READ_WRITE);
  MOZ_ASSERT(locked, "Could not lock the TextureClient");

  *aBlackDT = mTextureClient->AsTextureClientDrawTarget()->GetAsDrawTarget();
  if (aFlags & BUFFER_COMPONENT_ALPHA) {
    locked = mTextureClientOnWhite->Lock(OPEN_READ_WRITE);
    MOZ_ASSERT(locked, "Could not lock the second TextureClient for component alpha");

    *aWhiteDT = mTextureClientOnWhite->AsTextureClientDrawTarget()->GetAsDrawTarget();
  }
}

nsIntRegion
ContentClientRemoteBuffer::GetUpdatedRegion(const nsIntRegion& aRegionToDraw,
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
ContentClientRemoteBuffer::Updated(const nsIntRegion& aRegionToDraw,
                                   const nsIntRegion& aVisibleRegion,
                                   bool aDidSelfCopy)
{
  nsIntRegion updatedRegion = GetUpdatedRegion(aRegionToDraw,
                                               aVisibleRegion,
                                               aDidSelfCopy);

  MOZ_ASSERT(mTextureClient);
  if (mTextureClientOnWhite) {
    mForwarder->UseComponentAlphaTextures(this, mTextureClient,
                                          mTextureClientOnWhite);
  } else {
    mForwarder->UseTexture(this, mTextureClient);
  }
  mForwarder->UpdateTextureRegion(this,
                                  ThebesBufferData(BufferRect(),
                                                   BufferRotation()),
                                  updatedRegion);
}

void
ContentClientRemoteBuffer::SwapBuffers(const nsIntRegion& aFrontUpdatedRegion)
{
  MOZ_ASSERT(mTextureClient);
  mFrontAndBackBufferDiffer = true;
}

void
ContentClientDoubleBuffered::CreateFrontBuffer(const nsIntRect& aBufferRect)
{
  if (!CreateAndAllocateTextureClient(mFrontClient, TEXTURE_ON_BLACK) ||
      !AddTextureClient(mFrontClient)) {
    AbortTextureClientCreation();
    return;
  }
  if (mTextureInfo.mTextureFlags & TEXTURE_COMPONENT_ALPHA) {
    if (!CreateAndAllocateTextureClient(mFrontClientOnWhite, TEXTURE_ON_WHITE) ||
        !AddTextureClient(mFrontClientOnWhite)) {
      AbortTextureClientCreation();
      return;
    }
  }

  mFrontBufferRect = aBufferRect;
  mFrontBufferRotation = nsIntPoint();
}

void
ContentClientDoubleBuffered::DestroyFrontBuffer()
{
  MOZ_ASSERT(mFrontClient);

  mOldTextures.AppendElement(mFrontClient);
  mFrontClient = nullptr;
  if (mFrontClientOnWhite) {
    mOldTextures.AppendElement(mFrontClientOnWhite);
    mFrontClientOnWhite = nullptr;
  }
}

void
ContentClientDoubleBuffered::SwapBuffers(const nsIntRegion& aFrontUpdatedRegion)
{
  mFrontUpdatedRegion = aFrontUpdatedRegion;

  RefPtr<TextureClient> oldBack = mTextureClient;
  mTextureClient = mFrontClient;
  mFrontClient = oldBack;

  oldBack = mTextureClientOnWhite;
  mTextureClientOnWhite = mFrontClientOnWhite;
  mFrontClientOnWhite = oldBack;

  nsIntRect oldBufferRect = mBufferRect;
  mBufferRect = mFrontBufferRect;
  mFrontBufferRect = oldBufferRect;

  nsIntPoint oldBufferRotation = mBufferRotation;
  mBufferRotation = mFrontBufferRotation;
  mFrontBufferRotation = oldBufferRotation;

  MOZ_ASSERT(mFrontClient);

  ContentClientRemoteBuffer::SwapBuffers(aFrontUpdatedRegion);
}

void
ContentClientDoubleBuffered::PrepareFrame()
{
  mIsNewBuffer = false;

  if (!mFrontAndBackBufferDiffer) {
    return;
  }

  if (mDidSelfCopy) {
    
    
    
    
    mBufferRect.MoveTo(mFrontBufferRect.TopLeft());
    mBufferRotation = nsIntPoint();
    return;
  }
  mBufferRect = mFrontBufferRect;
  mBufferRotation = mFrontBufferRotation;
}





void
ContentClientDoubleBuffered::FinalizeFrame(const nsIntRegion& aRegionToDraw)
{
  if (mTextureClient) {
    DebugOnly<bool> locked = mTextureClient->Lock(OPEN_READ_WRITE);
    MOZ_ASSERT(locked);
  }
  if (mTextureClientOnWhite) {
    DebugOnly<bool> locked = mTextureClientOnWhite->Lock(OPEN_READ_WRITE);
    MOZ_ASSERT(locked);
  }

  if (!mFrontAndBackBufferDiffer) {
    MOZ_ASSERT(!mDidSelfCopy, "If we have to copy the world, then our buffers are different, right?");
    return;
  }
  MOZ_ASSERT(mFrontClient);

  MOZ_LAYERS_LOG(("BasicShadowableThebes(%p): reading back <x=%d,y=%d,w=%d,h=%d>",
                  this,
                  mFrontUpdatedRegion.GetBounds().x,
                  mFrontUpdatedRegion.GetBounds().y,
                  mFrontUpdatedRegion.GetBounds().width,
                  mFrontUpdatedRegion.GetBounds().height));

  mFrontAndBackBufferDiffer = false;

  nsIntRegion updateRegion = mFrontUpdatedRegion;
  if (mDidSelfCopy) {
    mDidSelfCopy = false;
    updateRegion = mBufferRect;
  }

  
  
  updateRegion.Sub(updateRegion, aRegionToDraw);
  if (updateRegion.IsEmpty()) {
    return;
  }

  
  
  if (!mFrontClient->Lock(OPEN_READ_ONLY)) {
    return;
  }
  if (mFrontClientOnWhite &&
      !mFrontClientOnWhite->Lock(OPEN_READ_ONLY)) {
    mFrontClient->Unlock();
    return;
  }
  {
    
    
    
    RefPtr<DrawTarget> dt =
      mFrontClient->AsTextureClientDrawTarget()->GetAsDrawTarget();
    RefPtr<DrawTarget> dtOnWhite = mFrontClientOnWhite
      ? mFrontClientOnWhite->AsTextureClientDrawTarget()->GetAsDrawTarget()
      : nullptr;
    RotatedBuffer frontBuffer(dt,
                              dtOnWhite,
                              mFrontBufferRect,
                              mFrontBufferRotation);
    UpdateDestinationFrom(frontBuffer, updateRegion);
  }

  mFrontClient->Unlock();
  if (mFrontClientOnWhite) {
    mFrontClientOnWhite->Unlock();
  }
}

void
ContentClientDoubleBuffered::UpdateDestinationFrom(const RotatedBuffer& aSource,
                                                   const nsIntRegion& aUpdateRegion)
{
  DrawTarget* destDT =
    BorrowDrawTargetForQuadrantUpdate(aUpdateRegion.GetBounds(), BUFFER_BLACK);
  if (!destDT) {
    return;
  }

  bool isClippingCheap = IsClippingCheap(destDT, aUpdateRegion);
  if (isClippingCheap) {
    gfxUtils::ClipToRegion(destDT, aUpdateRegion);
  }

  aSource.DrawBufferWithRotation(destDT, BUFFER_BLACK, 1.0, CompositionOp::OP_SOURCE);
  if (isClippingCheap) {
    destDT->PopClip();
  }
  
  destDT->Flush();
  ReturnDrawTargetToBuffer(destDT);

  if (aSource.HaveBufferOnWhite()) {
    MOZ_ASSERT(HaveBufferOnWhite());
    DrawTarget* destDT =
      BorrowDrawTargetForQuadrantUpdate(aUpdateRegion.GetBounds(), BUFFER_WHITE);
    if (!destDT) {
      return;
    }

    bool isClippingCheap = IsClippingCheap(destDT, aUpdateRegion);
    if (isClippingCheap) {
      gfxUtils::ClipToRegion(destDT, aUpdateRegion);
    }

    aSource.DrawBufferWithRotation(destDT, BUFFER_WHITE, 1.0, CompositionOp::OP_SOURCE);
    if (isClippingCheap) {
      destDT->PopClip();
    }
    
    destDT->Flush();
    ReturnDrawTargetToBuffer(destDT);
  }
}

void
ContentClientSingleBuffered::PrepareFrame()
{
  if (!mFrontAndBackBufferDiffer) {
    if (mTextureClient) {
      DebugOnly<bool> locked = mTextureClient->Lock(OPEN_READ_WRITE);
      MOZ_ASSERT(locked);
    }
    if (mTextureClientOnWhite) {
      DebugOnly<bool> locked = mTextureClientOnWhite->Lock(OPEN_READ_WRITE);
      MOZ_ASSERT(locked);
    }
    return;
  }

  RefPtr<DrawTarget> backBuffer = GetDTBuffer();
  if (!backBuffer && mTextureClient) {
    DebugOnly<bool> locked = mTextureClient->Lock(OPEN_READ_WRITE);
    MOZ_ASSERT(locked);
    backBuffer = mTextureClient->AsTextureClientDrawTarget()->GetAsDrawTarget();
  }

  RefPtr<DrawTarget> oldBuffer;
  oldBuffer = SetDTBuffer(backBuffer,
                          mBufferRect,
                          mBufferRotation);

  backBuffer = GetDTBufferOnWhite();
  if (!backBuffer && mTextureClientOnWhite) {
    DebugOnly<bool> locked = mTextureClientOnWhite->Lock(OPEN_READ_WRITE);
    MOZ_ASSERT(locked);
    backBuffer = mTextureClientOnWhite->AsTextureClientDrawTarget()->GetAsDrawTarget();
  }

  oldBuffer = SetDTBufferOnWhite(backBuffer);

  mIsNewBuffer = false;
  mFrontAndBackBufferDiffer = false;
}

static void
WrapRotationAxis(int32_t* aRotationPoint, int32_t aSize)
{
  if (*aRotationPoint < 0) {
    *aRotationPoint += aSize;
  } else if (*aRotationPoint >= aSize) {
    *aRotationPoint -= aSize;
  }
}

static void
FillSurface(DrawTarget* aDT, const nsIntRegion& aRegion,
            const nsIntPoint& aOffset, const gfxRGBA& aColor)
{
  nsIntRegionRectIterator iter(aRegion);
  const nsIntRect* r;
  while ((r = iter.Next()) != nullptr) {
    aDT->FillRect(Rect(r->x - aOffset.x, r->y - aOffset.y,
                       r->width, r->height),
                  ColorPattern(ToColor(aColor)));
  }
}

RotatedContentBuffer::PaintState
ContentClientIncremental::BeginPaintBuffer(ThebesLayer* aLayer,
                                           uint32_t aFlags)
{
  mTextureInfo.mDeprecatedTextureHostFlags = 0;
  PaintState result;
  
  
  bool canHaveRotation =  !(aFlags & RotatedContentBuffer::PAINT_WILL_RESAMPLE);

  nsIntRegion validRegion = aLayer->GetValidRegion();

  bool canUseOpaqueSurface = aLayer->CanUseOpaqueSurface();
  ContentType contentType =
    canUseOpaqueSurface ? gfxContentType::COLOR :
                          gfxContentType::COLOR_ALPHA;

  SurfaceMode mode;
  nsIntRegion neededRegion;
  bool canReuseBuffer;
  nsIntRect destBufferRect;

  while (true) {
    mode = aLayer->GetSurfaceMode();
    neededRegion = aLayer->GetVisibleRegion();
    
    canReuseBuffer = neededRegion.GetBounds().Size() <= mBufferRect.Size() &&
      mHasBuffer &&
      (!(aFlags & RotatedContentBuffer::PAINT_WILL_RESAMPLE) ||
       !(mTextureInfo.mTextureFlags & TEXTURE_ALLOW_REPEAT));

    if (canReuseBuffer) {
      if (mBufferRect.Contains(neededRegion.GetBounds())) {
        
        destBufferRect = mBufferRect;
      } else {
        
        
        destBufferRect = nsIntRect(neededRegion.GetBounds().TopLeft(), mBufferRect.Size());
      }
    } else {
      destBufferRect = neededRegion.GetBounds();
    }

    if (mode == SurfaceMode::SURFACE_COMPONENT_ALPHA) {
      if (!gfxPrefs::ComponentAlphaEnabled() ||
          !aLayer->GetParent() ||
          !aLayer->GetParent()->SupportsComponentAlphaChildren()) {
        mode = SurfaceMode::SURFACE_SINGLE_CHANNEL_ALPHA;
      } else {
        contentType = gfxContentType::COLOR;
      }
    }

    if ((aFlags & RotatedContentBuffer::PAINT_WILL_RESAMPLE) &&
        (!neededRegion.GetBounds().IsEqualInterior(destBufferRect) ||
         neededRegion.GetNumRects() > 1)) {
      
      if (mode == SurfaceMode::SURFACE_OPAQUE) {
        contentType = gfxContentType::COLOR_ALPHA;
        mode = SurfaceMode::SURFACE_SINGLE_CHANNEL_ALPHA;
      }
      

      
      
      neededRegion = destBufferRect;
    }

    if (mHasBuffer &&
        (mContentType != contentType ||
         (mode == SurfaceMode::SURFACE_COMPONENT_ALPHA) != mHasBufferOnWhite)) {
      
      
      result.mRegionToInvalidate = aLayer->GetValidRegion();
      validRegion.SetEmpty();
      mHasBuffer = false;
      mHasBufferOnWhite = false;
      mBufferRect.SetRect(0, 0, 0, 0);
      mBufferRotation.MoveTo(0, 0);
      
      
      continue;
    }

    break;
  }

  result.mRegionToDraw.Sub(neededRegion, validRegion);
  if (result.mRegionToDraw.IsEmpty())
    return result;

  if (destBufferRect.width > mForwarder->GetMaxTextureSize() ||
      destBufferRect.height > mForwarder->GetMaxTextureSize()) {
    return result;
  }

  
  
  if (!mForwarder->SupportsTextureBlitting() ||
      !mForwarder->SupportsPartialUploads()) {
    result.mRegionToDraw = neededRegion;
    validRegion.SetEmpty();
    mHasBuffer = false;
    mHasBufferOnWhite = false;
    mBufferRect.SetRect(0, 0, 0, 0);
    mBufferRotation.MoveTo(0, 0);
    canReuseBuffer = false;
  }

  nsIntRect drawBounds = result.mRegionToDraw.GetBounds();
  bool createdBuffer = false;

  uint32_t bufferFlags = canHaveRotation ? TEXTURE_ALLOW_REPEAT : 0;
  if (mode == SurfaceMode::SURFACE_COMPONENT_ALPHA) {
    bufferFlags |= TEXTURE_COMPONENT_ALPHA;
  }
  if (canReuseBuffer) {
    nsIntRect keepArea;
    if (keepArea.IntersectRect(destBufferRect, mBufferRect)) {
      
      
      
      nsIntPoint newRotation = mBufferRotation +
        (destBufferRect.TopLeft() - mBufferRect.TopLeft());
      WrapRotationAxis(&newRotation.x, mBufferRect.width);
      WrapRotationAxis(&newRotation.y, mBufferRect.height);
      NS_ASSERTION(nsIntRect(nsIntPoint(0,0), mBufferRect.Size()).Contains(newRotation),
                   "newRotation out of bounds");
      int32_t xBoundary = destBufferRect.XMost() - newRotation.x;
      int32_t yBoundary = destBufferRect.YMost() - newRotation.y;
      if ((drawBounds.x < xBoundary && xBoundary < drawBounds.XMost()) ||
          (drawBounds.y < yBoundary && yBoundary < drawBounds.YMost()) ||
          (newRotation != nsIntPoint(0,0) && !canHaveRotation)) {
        
        
        
        
        
        
        destBufferRect = neededRegion.GetBounds();
        createdBuffer = true;
      } else {
        mBufferRect = destBufferRect;
        mBufferRotation = newRotation;
      }
    } else {
      
      
      
      mBufferRect = destBufferRect;
      mBufferRotation = nsIntPoint(0,0);
    }
  } else {
    
    createdBuffer = true;
  }
  NS_ASSERTION(!(aFlags & RotatedContentBuffer::PAINT_WILL_RESAMPLE) ||
               destBufferRect == neededRegion.GetBounds(),
               "If we're resampling, we need to validate the entire buffer");

  if (!createdBuffer && !mHasBuffer) {
    return result;
  }

  if (createdBuffer) {
    if (mHasBuffer &&
        (mode != SurfaceMode::SURFACE_COMPONENT_ALPHA || mHasBufferOnWhite)) {
      mTextureInfo.mDeprecatedTextureHostFlags = TEXTURE_HOST_COPY_PREVIOUS;
    }

    mHasBuffer = true;
    if (mode == SurfaceMode::SURFACE_COMPONENT_ALPHA) {
      mHasBufferOnWhite = true;
    }
    mBufferRect = destBufferRect;
    mBufferRotation = nsIntPoint(0,0);
    NotifyBufferCreated(contentType, bufferFlags);
  }

  NS_ASSERTION(canHaveRotation || mBufferRotation == nsIntPoint(0,0),
               "Rotation disabled, but we have nonzero rotation?");

  nsIntRegion invalidate;
  invalidate.Sub(aLayer->GetValidRegion(), destBufferRect);
  result.mRegionToInvalidate.Or(result.mRegionToInvalidate, invalidate);

  
  
  
  
  
  
  
  result.mClip = DrawRegionClip::DRAW;
  result.mMode = mode;

  return result;
}

DrawTarget*
ContentClientIncremental::BorrowDrawTargetForPainting(const PaintState& aPaintState,
                                                      RotatedContentBuffer::DrawIterator* aIter)
{
  if (aPaintState.mMode == SurfaceMode::SURFACE_NONE) {
    return nullptr;
  }

  if (aIter) {
    if (aIter->mCount++ > 0) {
      return nullptr;
    }
    aIter->mDrawRegion = aPaintState.mRegionToDraw;
  }

  DrawTarget* result = nullptr;

  nsIntRect drawBounds = aPaintState.mRegionToDraw.GetBounds();
  MOZ_ASSERT(!mLoanedDrawTarget);

  
  
  if (aPaintState.mMode == SurfaceMode::SURFACE_COMPONENT_ALPHA) {
    nsIntRegion drawRegionCopy = aPaintState.mRegionToDraw;
    RefPtr<DrawTarget> onBlack = GetUpdateSurface(BUFFER_BLACK, drawRegionCopy);
    RefPtr<DrawTarget> onWhite = GetUpdateSurface(BUFFER_WHITE, aPaintState.mRegionToDraw);
    if (onBlack && onWhite) {
      NS_ASSERTION(aPaintState.mRegionToDraw == drawRegionCopy,
                   "BeginUpdate should always modify the draw region in the same way!");
      FillSurface(onBlack, aPaintState.mRegionToDraw, nsIntPoint(drawBounds.x, drawBounds.y), gfxRGBA(0.0, 0.0, 0.0, 1.0));
      FillSurface(onWhite, aPaintState.mRegionToDraw, nsIntPoint(drawBounds.x, drawBounds.y), gfxRGBA(1.0, 1.0, 1.0, 1.0));
      mLoanedDrawTarget = Factory::CreateDualDrawTarget(onBlack, onWhite);
    } else {
      mLoanedDrawTarget = nullptr;
    }
  } else {
    mLoanedDrawTarget = GetUpdateSurface(BUFFER_BLACK, aPaintState.mRegionToDraw);
  }
  if (!mLoanedDrawTarget) {
    NS_WARNING("unable to get context for update");
    return nullptr;
  }

  result = mLoanedDrawTarget;
  mLoanedTransform = mLoanedDrawTarget->GetTransform();
  mLoanedTransform.Translate(-drawBounds.x, -drawBounds.y);
  result->SetTransform(mLoanedTransform);
  mLoanedTransform.Translate(drawBounds.x, drawBounds.y);

  if (mContentType == gfxContentType::COLOR_ALPHA) {
    gfxUtils::ClipToRegion(result, aPaintState.mRegionToDraw);
    nsIntRect bounds = aPaintState.mRegionToDraw.GetBounds();
    result->ClearRect(Rect(bounds.x, bounds.y, bounds.width, bounds.height));
  }

  return result;
}

void
ContentClientIncremental::Updated(const nsIntRegion& aRegionToDraw,
                                  const nsIntRegion& aVisibleRegion,
                                  bool aDidSelfCopy)
{
  if (IsSurfaceDescriptorValid(mUpdateDescriptor)) {
    mForwarder->UpdateTextureIncremental(this,
                                         TextureFront,
                                         mUpdateDescriptor,
                                         aRegionToDraw,
                                         mBufferRect,
                                         mBufferRotation);
    mUpdateDescriptor = SurfaceDescriptor();
  }
  if (IsSurfaceDescriptorValid(mUpdateDescriptorOnWhite)) {
    mForwarder->UpdateTextureIncremental(this,
                                         TextureOnWhiteFront,
                                         mUpdateDescriptorOnWhite,
                                         aRegionToDraw,
                                         mBufferRect,
                                         mBufferRotation);
    mUpdateDescriptorOnWhite = SurfaceDescriptor();
  }

}

TemporaryRef<DrawTarget>
ContentClientIncremental::GetUpdateSurface(BufferType aType,
                                           const nsIntRegion& aUpdateRegion)
{
  nsIntRect rgnSize = aUpdateRegion.GetBounds();
  if (!mBufferRect.Contains(rgnSize)) {
    NS_ERROR("update outside of image");
    return nullptr;
  }
  SurfaceDescriptor desc;
  if (!mForwarder->AllocSurfaceDescriptor(rgnSize.Size().ToIntSize(),
                                          mContentType,
                                          &desc)) {
    NS_WARNING("creating SurfaceDescriptor failed!");
    return nullptr;
  }

  if (aType == BUFFER_BLACK) {
    MOZ_ASSERT(!IsSurfaceDescriptorValid(mUpdateDescriptor));
    mUpdateDescriptor = desc;
  } else {
    MOZ_ASSERT(!IsSurfaceDescriptorValid(mUpdateDescriptorOnWhite));
    MOZ_ASSERT(aType == BUFFER_WHITE);
    mUpdateDescriptorOnWhite = desc;
  }

  return GetDrawTargetForDescriptor(desc, gfx::BackendType::COREGRAPHICS);
}

}
}
