




#include "ClientPaintedLayer.h"
#include "ClientTiledPaintedLayer.h"     
#include <stdint.h>                     
#include "GeckoProfiler.h"              
#include "client/ClientLayerManager.h"  
#include "gfxContext.h"                 
#include "gfxRect.h"                    
#include "gfxPrefs.h"                   
#include "mozilla/Assertions.h"         
#include "mozilla/gfx/2D.h"             
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/LayersTypes.h"
#include "mozilla/Preferences.h"
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "gfx2DGlue.h"
#include "ReadbackProcessor.h"

#ifdef XP_WIN
#include "gfxWindowsPlatform.h"
#endif

namespace mozilla {
namespace layers {

using namespace mozilla::gfx;

void
ClientPaintedLayer::PaintThebes()
{
#ifdef XP_WIN
  if (gfxWindowsPlatform::GetPlatform()->DidRenderingDeviceReset()) {
    
    return;
  }
#endif

  PROFILER_LABEL("ClientPaintedLayer", "PaintThebes",
    js::ProfileEntry::Category::GRAPHICS);

  NS_ASSERTION(ClientManager()->InDrawing(),
               "Can only draw in drawing phase");
  
  uint32_t flags = RotatedContentBuffer::PAINT_CAN_DRAW_ROTATED;
#ifndef MOZ_IGNORE_PAINT_WILL_RESAMPLE
  if (ClientManager()->CompositorMightResample()) {
    flags |= RotatedContentBuffer::PAINT_WILL_RESAMPLE;
  }
  if (!(flags & RotatedContentBuffer::PAINT_WILL_RESAMPLE)) {
    if (MayResample()) {
      flags |= RotatedContentBuffer::PAINT_WILL_RESAMPLE;
    }
  }
#endif
  PaintState state =
    mContentClient->BeginPaintBuffer(this, flags);
  mValidRegion.Sub(mValidRegion, state.mRegionToInvalidate);

  if (!state.mRegionToDraw.IsEmpty() && !ClientManager()->GetPaintedLayerCallback()) {
    ClientManager()->SetTransactionIncomplete();
    return;
  }

  
  
  
  
  state.mRegionToInvalidate.And(state.mRegionToInvalidate,
                                GetEffectiveVisibleRegion());

  bool didUpdate = false;
  RotatedContentBuffer::DrawIterator iter;
  while (DrawTarget* target = mContentClient->BorrowDrawTargetForPainting(state, &iter)) {
    SetAntialiasingFlags(this, target);

    nsRefPtr<gfxContext> ctx = gfxContext::ContextForDrawTarget(target);

    ClientManager()->GetPaintedLayerCallback()(this,
                                              ctx,
                                              iter.mDrawRegion,
                                              state.mClip,
                                              state.mRegionToInvalidate,
                                              ClientManager()->GetPaintedLayerCallbackData());

    ctx = nullptr;
    mContentClient->ReturnDrawTargetToBuffer(target);
    didUpdate = true;
  }

  if (didUpdate) {
    Mutated();

    mValidRegion.Or(mValidRegion, state.mRegionToDraw);

    ContentClientRemote* contentClientRemote = static_cast<ContentClientRemote*>(mContentClient.get());
    MOZ_ASSERT(contentClientRemote->GetIPDLActor());

    
    
    
    ClientManager()->Hold(this);
    contentClientRemote->Updated(state.mRegionToDraw,
                                 mVisibleRegion,
                                 state.mDidSelfCopy);
  }
}

void
ClientPaintedLayer::RenderLayerWithReadback(ReadbackProcessor *aReadback)
{
  RenderMaskLayers(this);
  
  if (!mContentClient) {
    mContentClient = ContentClient::CreateContentClient(ClientManager()->AsShadowForwarder());
    if (!mContentClient) {
      return;
    }
    mContentClient->Connect();
    ClientManager()->AsShadowForwarder()->Attach(mContentClient, this);
    MOZ_ASSERT(mContentClient->GetForwarder());
  }

  nsTArray<ReadbackProcessor::Update> readbackUpdates;
  nsIntRegion readbackRegion;
  if (aReadback && UsedForReadback()) {
    aReadback->GetPaintedLayerUpdates(this, &readbackUpdates);
  }

  IntPoint origin(mVisibleRegion.GetBounds().x, mVisibleRegion.GetBounds().y);
  mContentClient->BeginPaint();
  PaintThebes();
  mContentClient->EndPaint(&readbackUpdates);
}

bool
ClientLayerManager::IsOptimizedFor(PaintedLayer* aLayer, PaintedLayerCreationHint aHint)
{
#ifdef MOZ_B2G
  
  
  
  
  
  
  return aHint == aLayer->GetCreationHint();
#else
  return LayerManager::IsOptimizedFor(aLayer, aHint);
#endif
}

already_AddRefed<PaintedLayer>
ClientLayerManager::CreatePaintedLayer()
{
  return CreatePaintedLayerWithHint(NONE);
}

already_AddRefed<PaintedLayer>
ClientLayerManager::CreatePaintedLayerWithHint(PaintedLayerCreationHint aHint)
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  if (
#ifdef MOZ_B2G
      aHint == SCROLLABLE &&
#endif
      gfxPrefs::LayersTilesEnabled()
#ifndef MOZ_X11
      && (AsShadowForwarder()->GetCompositorBackendType() == LayersBackend::LAYERS_OPENGL ||
          AsShadowForwarder()->GetCompositorBackendType() == LayersBackend::LAYERS_D3D9 ||
          AsShadowForwarder()->GetCompositorBackendType() == LayersBackend::LAYERS_D3D11)
#endif
  ) {
    nsRefPtr<ClientTiledPaintedLayer> layer = new ClientTiledPaintedLayer(this, aHint);
    CREATE_SHADOW(Painted);
    return layer.forget();
  } else {
    nsRefPtr<ClientPaintedLayer> layer = new ClientPaintedLayer(this, aHint);
    CREATE_SHADOW(Painted);
    return layer.forget();
  }
}

void
ClientPaintedLayer::PrintInfo(std::stringstream& aStream, const char* aPrefix)
{
  PaintedLayer::PrintInfo(aStream, aPrefix);
  if (mContentClient) {
    aStream << "\n";
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    mContentClient->PrintInfo(aStream, pfx.get());
  }
}

}
}
