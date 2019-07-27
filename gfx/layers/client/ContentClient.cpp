




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
#include "mozilla/layers/CompositorChild.h" 
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/LayersMessages.h"  
#include "mozilla/layers/LayersTypes.h"
#include "nsAutoPtr.h"                  
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsIWidget.h"                  
#include "prenv.h"                      
#include "nsLayoutUtils.h"
#ifdef XP_WIN
#include "gfxWindowsPlatform.h"
#endif
#ifdef MOZ_WIDGET_GTK
#include "gfxPlatformGtk.h"
#endif
#include "gfx2DGlue.h"
#include "ReadbackLayer.h"

#include <vector>

using namespace std;

namespace mozilla {

using namespace gfx;

namespace layers {

static TextureFlags TextureFlagsForRotatedContentBufferFlags(uint32_t aBufferFlags)
{
  TextureFlags result = TextureFlags::NO_FLAGS;

  if (aBufferFlags & RotatedContentBuffer::BUFFER_COMPONENT_ALPHA) {
    result |= TextureFlags::COMPONENT_ALPHA;
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
#ifdef MOZ_WIDGET_GTK
  
  
  
  if (!gfxPlatformGtk::GetPlatform()->UseImageOffscreenSurfaces() ||
      !gfxPlatformGtk::GetPlatform()->UseXRender())
#endif
  {
    useDoubleBuffering = (LayerManagerComposite::SupportsDirectTexturing() &&
                         backend != LayersBackend::LAYERS_D3D9) ||
                         backend == LayersBackend::LAYERS_BASIC;
  }

  if (useDoubleBuffering || PR_GetEnv("MOZ_FORCE_DOUBLE_BUFFERING")) {
    return new ContentClientDoubleBuffered(aForwarder);
  }
  return new ContentClientSingleBuffered(aForwarder);
}

void
ContentClient::EndPaint(nsTArray<ReadbackProcessor::Update>* aReadbackUpdates)
{
  
  
  
  
  
  
  
  
  
  
  
  OnTransaction();
}

void
ContentClient::PrintInfo(std::stringstream& aStream, const char* aPrefix)
{
  aStream << aPrefix;
  aStream << nsPrintfCString("ContentClient (0x%p)", this).get();

  if (profiler_feature_active("displaylistdump")) {
    nsAutoCString pfx(aPrefix);
    pfx += "  ";

    Dump(aStream, pfx.get(), false);
  }
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

class RemoteBufferReadbackProcessor : public TextureReadbackSink
{
public:
  RemoteBufferReadbackProcessor(nsTArray<ReadbackProcessor::Update>* aReadbackUpdates,
                                const nsIntRect& aBufferRect, const nsIntPoint& aBufferRotation)
    : mReadbackUpdates(*aReadbackUpdates)
    , mBufferRect(aBufferRect)
    , mBufferRotation(aBufferRotation)
  {
    for (uint32_t i = 0; i < mReadbackUpdates.Length(); ++i) {
      mLayerRefs.push_back(mReadbackUpdates[i].mLayer);
    }
  }

  virtual void ProcessReadback(gfx::DataSourceSurface *aSourceSurface)
  {
    SourceRotatedBuffer rotBuffer(aSourceSurface, nullptr, mBufferRect, mBufferRotation);

    for (uint32_t i = 0; i < mReadbackUpdates.Length(); ++i) {
      ReadbackProcessor::Update& update = mReadbackUpdates[i];
      nsIntPoint offset = update.mLayer->GetBackgroundLayerOffset();

      ReadbackSink* sink = update.mLayer->GetSink();

      if (!sink) {
        continue;
      }

      if (!aSourceSurface) {
        sink->SetUnknown(update.mSequenceCounter);
        continue;
      }

      nsRefPtr<gfxContext> ctx =
        sink->BeginUpdate(update.mUpdateRect + offset, update.mSequenceCounter);

      if (!ctx) {
        continue;
      }

      DrawTarget* dt = ctx->GetDrawTarget();
      dt->SetTransform(Matrix::Translation(offset.x, offset.y));

      rotBuffer.DrawBufferWithRotation(dt, RotatedBuffer::BUFFER_BLACK);

      update.mLayer->GetSink()->EndUpdate(ctx, update.mUpdateRect + offset);
    }
  }

private:
  nsTArray<ReadbackProcessor::Update> mReadbackUpdates;
  
  vector<RefPtr<Layer>> mLayerRefs;

  nsIntRect mBufferRect;
  nsIntPoint mBufferRotation;
};

void
ContentClientRemoteBuffer::BeginPaint()
{
  EnsureBackBufferIfFrontBuffer();

  
  
  if (mTextureClient) {
    SetBufferProvider(mTextureClient);
  }
  if (mTextureClientOnWhite) {
    SetBufferProviderOnWhite(mTextureClientOnWhite);
  }
}

void
ContentClientRemoteBuffer::EndPaint(nsTArray<ReadbackProcessor::Update>* aReadbackUpdates)
{
  MOZ_ASSERT(!mTextureClientOnWhite || !aReadbackUpdates || aReadbackUpdates->Length() == 0);

  
  
  SetBufferProvider(nullptr);
  SetBufferProviderOnWhite(nullptr);
  for (unsigned i = 0; i< mOldTextures.Length(); ++i) {
    if (mOldTextures[i]->IsLocked()) {
      mOldTextures[i]->Unlock();
    }
  }
  mOldTextures.Clear();

  if (mTextureClient && mTextureClient->IsLocked()) {
    if (aReadbackUpdates->Length() > 0) {
      RefPtr<TextureReadbackSink> readbackSink = new RemoteBufferReadbackProcessor(aReadbackUpdates, mBufferRect, mBufferRotation);

      mTextureClient->SetReadbackSink(readbackSink);
    }

    mTextureClient->Unlock();
    mTextureClient->SyncWithObject(mForwarder->GetSyncObject());
  }
  if (mTextureClientOnWhite && mTextureClientOnWhite->IsLocked()) {
    mTextureClientOnWhite->Unlock();
    mTextureClientOnWhite->SyncWithObject(mForwarder->GetSyncObject());
  }

  ContentClientRemote::EndPaint(aReadbackUpdates);
}

void
ContentClientRemoteBuffer::BuildTextureClients(SurfaceFormat aFormat,
                                               const nsIntRect& aRect,
                                               uint32_t aFlags)
{
  
  
  
  
  
  
  MOZ_ASSERT(!mIsNewBuffer,
             "Bad! Did we create a buffer twice without painting?");

  mIsNewBuffer = true;

  DestroyBuffers();

  mSurfaceFormat = aFormat;
  mSize = gfx::IntSize(aRect.width, aRect.height);
  mTextureFlags = TextureFlagsForRotatedContentBufferFlags(aFlags);

  if (aFlags & BUFFER_COMPONENT_ALPHA) {
    mTextureFlags |= TextureFlags::COMPONENT_ALPHA;
  }

  CreateBackBuffer(mBufferRect);
}

void
ContentClientRemoteBuffer::CreateBackBuffer(const nsIntRect& aBufferRect)
{
  
  mTextureClient = CreateTextureClientForDrawing(
    mSurfaceFormat, mSize, gfx::BackendType::NONE,
    mTextureFlags,
    TextureAllocationFlags::ALLOC_CLEAR_BUFFER
  );
  if (!mTextureClient || !AddTextureClient(mTextureClient)) {
    AbortTextureClientCreation();
    return;
  }

  if (mTextureFlags & TextureFlags::COMPONENT_ALPHA) {
    mTextureClientOnWhite = mTextureClient->CreateSimilar(
      mTextureFlags,
      TextureAllocationFlags::ALLOC_CLEAR_BUFFER_WHITE
    );
    if (!mTextureClientOnWhite || !AddTextureClient(mTextureClientOnWhite)) {
      AbortTextureClientCreation();
      return;
    }
  }
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

  
  
  DebugOnly<bool> locked = mTextureClient->Lock(OpenMode::OPEN_READ_WRITE);
  MOZ_ASSERT(locked, "Could not lock the TextureClient");

  *aBlackDT = mTextureClient->BorrowDrawTarget();
  if (aFlags & BUFFER_COMPONENT_ALPHA) {
    locked = mTextureClientOnWhite->Lock(OpenMode::OPEN_READ_WRITE);
    MOZ_ASSERT(locked, "Could not lock the second TextureClient for component alpha");

    *aWhiteDT = mTextureClientOnWhite->BorrowDrawTarget();
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
  MOZ_ASSERT(mTextureClient, "should have a back buffer by now");

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
  mFrontAndBackBufferDiffer = true;
}

void
ContentClientRemoteBuffer::Dump(std::stringstream& aStream,
                                const char* aPrefix,
                                bool aDumpHtml)
{
  
  aStream << "\n" << aPrefix << "Surface: ";
  CompositableClient::DumpTextureClient(aStream, mTextureClient);
}

void
ContentClientDoubleBuffered::Dump(std::stringstream& aStream,
                                const char* aPrefix,
                                bool aDumpHtml)
{
  
  aStream << "\n" << aPrefix << "Surface: ";
  CompositableClient::DumpTextureClient(aStream, mFrontClient);
}

void
ContentClientDoubleBuffered::DestroyFrontBuffer()
{
  if (mFrontClient) {
    mOldTextures.AppendElement(mFrontClient);
    mFrontClient = nullptr;
  }

  if (mFrontClientOnWhite) {
    mOldTextures.AppendElement(mFrontClientOnWhite);
    mFrontClientOnWhite = nullptr;
  }
}

void
ContentClientDoubleBuffered::Updated(const nsIntRegion& aRegionToDraw,
                                     const nsIntRegion& aVisibleRegion,
                                     bool aDidSelfCopy)
{
  ContentClientRemoteBuffer::Updated(aRegionToDraw, aVisibleRegion, aDidSelfCopy);

#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
  if (mFrontClient) {
    
    RefPtr<AsyncTransactionTracker> tracker = new RemoveTextureFromCompositableTracker();
    
    tracker->SetTextureClient(mFrontClient);
    mFrontClient->SetRemoveFromCompositableTracker(tracker);
    
    GetForwarder()->RemoveTextureFromCompositableAsync(tracker, this, mFrontClient);
  }

  if (mFrontClientOnWhite) {
    
    RefPtr<AsyncTransactionTracker> tracker = new RemoveTextureFromCompositableTracker();
    
    tracker->SetTextureClient(mFrontClientOnWhite);
    mFrontClientOnWhite->SetRemoveFromCompositableTracker(tracker);
    
    GetForwarder()->RemoveTextureFromCompositableAsync(tracker, this, mFrontClientOnWhite);
  }
#endif
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
ContentClientDoubleBuffered::BeginPaint()
{
  ContentClientRemoteBuffer::BeginPaint();

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
    DebugOnly<bool> locked = mTextureClient->Lock(OpenMode::OPEN_READ_WRITE);
    MOZ_ASSERT(locked);
  }
  if (mTextureClientOnWhite) {
    DebugOnly<bool> locked = mTextureClientOnWhite->Lock(OpenMode::OPEN_READ_WRITE);
    MOZ_ASSERT(locked);
  }

  if (!mFrontAndBackBufferDiffer) {
    MOZ_ASSERT(!mDidSelfCopy, "If we have to copy the world, then our buffers are different, right?");
    return;
  }
  MOZ_ASSERT(mFrontClient);
  if (!mFrontClient) {
    return;
  }

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

  
  
  if (!mFrontClient->Lock(OpenMode::OPEN_READ_ONLY)) {
    return;
  }
  if (mFrontClientOnWhite &&
      !mFrontClientOnWhite->Lock(OpenMode::OPEN_READ_ONLY)) {
    mFrontClient->Unlock();
    return;
  }
  {
    
    
    
    RefPtr<SourceSurface> surf = mFrontClient->BorrowDrawTarget()->Snapshot();
    RefPtr<SourceSurface> surfOnWhite = mFrontClientOnWhite
      ? mFrontClientOnWhite->BorrowDrawTarget()->Snapshot()
      : nullptr;
    SourceRotatedBuffer frontBuffer(surf,
                                    surfOnWhite,
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
ContentClientDoubleBuffered::EnsureBackBufferIfFrontBuffer()
{
  if (!mTextureClient && mFrontClient) {
    CreateBackBuffer(mFrontBufferRect);

    mBufferRect = mFrontBufferRect;
    mBufferRotation = mFrontBufferRotation;
  }
}

void
ContentClientDoubleBuffered::UpdateDestinationFrom(const RotatedBuffer& aSource,
                                                   const nsIntRegion& aUpdateRegion)
{
  DrawIterator iter;
  while (DrawTarget* destDT =
    BorrowDrawTargetForQuadrantUpdate(aUpdateRegion.GetBounds(), BUFFER_BLACK, &iter)) {
    bool isClippingCheap = IsClippingCheap(destDT, iter.mDrawRegion);
    if (isClippingCheap) {
      gfxUtils::ClipToRegion(destDT, iter.mDrawRegion);
    }

    aSource.DrawBufferWithRotation(destDT, BUFFER_BLACK, 1.0, CompositionOp::OP_SOURCE);
    if (isClippingCheap) {
      destDT->PopClip();
    }
    
    destDT->Flush();
    ReturnDrawTargetToBuffer(destDT);
  }

  if (aSource.HaveBufferOnWhite()) {
    MOZ_ASSERT(HaveBufferOnWhite());
    DrawIterator whiteIter;
    while (DrawTarget* destDT =
      BorrowDrawTargetForQuadrantUpdate(aUpdateRegion.GetBounds(), BUFFER_WHITE, &whiteIter)) {
      bool isClippingCheap = IsClippingCheap(destDT, whiteIter.mDrawRegion);
      if (isClippingCheap) {
        gfxUtils::ClipToRegion(destDT, whiteIter.mDrawRegion);
      }

      aSource.DrawBufferWithRotation(destDT, BUFFER_WHITE, 1.0, CompositionOp::OP_SOURCE);
      if (isClippingCheap) {
        destDT->PopClip();
      }
      
      destDT->Flush();
      ReturnDrawTargetToBuffer(destDT);
    }
  }
}

void
ContentClientSingleBuffered::FinalizeFrame(const nsIntRegion& aRegionToDraw)
{
  if (mTextureClient) {
    DebugOnly<bool> locked = mTextureClient->Lock(OpenMode::OPEN_READ_WRITE);
    MOZ_ASSERT(locked);
  }
  if (mTextureClientOnWhite) {
    DebugOnly<bool> locked = mTextureClientOnWhite->Lock(OpenMode::OPEN_READ_WRITE);
    MOZ_ASSERT(locked);
  }
}

}
}
