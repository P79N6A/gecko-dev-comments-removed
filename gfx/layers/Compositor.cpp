




#include "mozilla/layers/Compositor.h"
#include "base/message_loop.h"          
#include "mozilla/layers/CompositorParent.h"  
#include "mozilla/layers/Effects.h"     
#include "mozilla/mozalloc.h"           
#include "gfx2DGlue.h"

namespace mozilla {
namespace gfx {
class Matrix4x4;
}

namespace layers {

 LayersBackend Compositor::sBackend = LayersBackend::LAYERS_NONE;
 LayersBackend
Compositor::GetBackend()
{
  AssertOnCompositorThread();
  return sBackend;
}

 void
Compositor::SetBackend(LayersBackend backend)
{
  if (sBackend != LayersBackend::LAYERS_NONE && sBackend != backend) {
    
    

#ifdef XP_MACOSX
    printf("ERROR: Changing compositor from %u to %u.\n",
           unsigned(sBackend), unsigned(backend));
#endif
  }
  sBackend = backend;
}

 void
Compositor::AssertOnCompositorThread()
{
  MOZ_ASSERT(CompositorParent::CompositorLoop() ==
             MessageLoop::current(),
             "Can only call this from the compositor thread!");
}

bool
Compositor::ShouldDrawDiagnostics(DiagnosticFlags aFlags)
{
  if ((aFlags & DiagnosticFlags::TILE) && !(mDiagnosticTypes & DiagnosticTypes::TILE_BORDERS)) {
    return false;
  }
  if ((aFlags & DiagnosticFlags::BIGIMAGE) &&
      !(mDiagnosticTypes & DiagnosticTypes::BIGIMAGE_BORDERS)) {
    return false;
  }
  if (mDiagnosticTypes == DiagnosticTypes::NO_DIAGNOSTIC) {
    return false;
  }
  return true;
}

void
Compositor::DrawDiagnostics(DiagnosticFlags aFlags,
                            const nsIntRegion& aVisibleRegion,
                            const gfx::Rect& aClipRect,
                            const gfx::Matrix4x4& aTransform,
                            uint32_t aFlashCounter)
{
  if (!ShouldDrawDiagnostics(aFlags)) {
    return;
  }

  if (aVisibleRegion.GetNumRects() > 1) {
    nsIntRegionRectIterator screenIter(aVisibleRegion);

    while (const nsIntRect* rect = screenIter.Next())
    {
      DrawDiagnostics(aFlags | DiagnosticFlags::REGION_RECT,
                      ToRect(*rect), aClipRect, aTransform, aFlashCounter);
    }
  }

  DrawDiagnostics(aFlags, ToRect(aVisibleRegion.GetBounds()),
                  aClipRect, aTransform, aFlashCounter);
}

void
Compositor::DrawDiagnostics(DiagnosticFlags aFlags,
                            const gfx::Rect& aVisibleRect,
                            const gfx::Rect& aClipRect,
                            const gfx::Matrix4x4& aTransform,
                            uint32_t aFlashCounter)
{
  if (!ShouldDrawDiagnostics(aFlags)) {
    return;
  }

  DrawDiagnosticsInternal(aFlags, aVisibleRect, aClipRect, aTransform,
                          aFlashCounter);
}

RenderTargetRect
Compositor::ClipRectInLayersCoordinates(Layer* aLayer, RenderTargetIntRect aClip) const {
  ContainerLayer* parent = aLayer->AsContainerLayer() ? aLayer->AsContainerLayer() : aLayer->GetParent();
  while (!parent->UseIntermediateSurface() && parent->GetParent()) {
    parent = parent->GetParent();
  }

  RenderTargetIntPoint renderTargetOffset = RenderTargetIntRect::FromUntyped(
    parent->GetEffectiveVisibleRegion().GetBounds()).TopLeft();

  RenderTargetRect result;
  aClip = aClip + renderTargetOffset;
  RenderTargetIntSize destSize = RenderTargetIntSize(GetWidgetSize().width,
                                                     GetWidgetSize().height);

  switch (mScreenRotation) {
    case ROTATION_0:
      result = RenderTargetRect(aClip.x, aClip.y, aClip.width, aClip.height);
      break;
    case ROTATION_90:
      result = RenderTargetRect(aClip.y,
                                destSize.width - aClip.x - aClip.width,
                                aClip.height, aClip.width);
      break;
    case ROTATION_270:
      result = RenderTargetRect(destSize.height - aClip.y - aClip.height,
                                aClip.x,
                                aClip.height, aClip.width);
      break;
    case ROTATION_180:
      result = RenderTargetRect(destSize.width - aClip.x - aClip.width,
                                destSize.height - aClip.y - aClip.height,
                                aClip.width, aClip.height);
      break;
      
      
    default: {}
  }
  return result;
}

void
Compositor::DrawDiagnosticsInternal(DiagnosticFlags aFlags,
                                    const gfx::Rect& aVisibleRect,
                                    const gfx::Rect& aClipRect,
                                    const gfx::Matrix4x4& aTransform,
                                    uint32_t aFlashCounter)
{
#ifdef MOZ_B2G
  int lWidth = 4;
#elif defined(ANDROID)
  int lWidth = 10;
#else
  int lWidth = 2;
#endif
  float opacity = 0.7f;

  gfx::Color color;
  if (aFlags & DiagnosticFlags::CONTENT) {
    color = gfx::Color(0.0f, 1.0f, 0.0f, 1.0f); 
    if (aFlags & DiagnosticFlags::COMPONENT_ALPHA) {
      color = gfx::Color(0.0f, 1.0f, 1.0f, 1.0f); 
    }
  } else if (aFlags & DiagnosticFlags::IMAGE) {
    color = gfx::Color(1.0f, 0.0f, 0.0f, 1.0f); 
  } else if (aFlags & DiagnosticFlags::COLOR) {
    color = gfx::Color(0.0f, 0.0f, 1.0f, 1.0f); 
  } else if (aFlags & DiagnosticFlags::CONTAINER) {
    color = gfx::Color(0.8f, 0.0f, 0.8f, 1.0f); 
  }

  
  if (aFlags & DiagnosticFlags::TILE ||
      aFlags & DiagnosticFlags::BIGIMAGE ||
      aFlags & DiagnosticFlags::REGION_RECT) {
    lWidth = 1;
    opacity = 0.5f;
    color.r *= 0.7f;
    color.g *= 0.7f;
    color.b *= 0.7f;
  }

  if (mDiagnosticTypes & DiagnosticTypes::FLASH_BORDERS) {
    float flash = (float)aFlashCounter / (float)DIAGNOSTIC_FLASH_COUNTER_MAX;
    color.r *= flash;
    color.g *= flash;
    color.b *= flash;
  }

  EffectChain effects;

  effects.mPrimaryEffect = new EffectSolidColor(color);
  
  this->DrawQuad(gfx::Rect(aVisibleRect.x, aVisibleRect.y,
                           lWidth, aVisibleRect.height),
                 aClipRect, effects, opacity,
                 aTransform);
  
  this->DrawQuad(gfx::Rect(aVisibleRect.x + lWidth, aVisibleRect.y,
                           aVisibleRect.width - 2 * lWidth, lWidth),
                 aClipRect, effects, opacity,
                 aTransform);
  
  this->DrawQuad(gfx::Rect(aVisibleRect.x + aVisibleRect.width - lWidth, aVisibleRect.y,
                           lWidth, aVisibleRect.height),
                 aClipRect, effects, opacity,
                 aTransform);
  
  this->DrawQuad(gfx::Rect(aVisibleRect.x + lWidth, aVisibleRect.y + aVisibleRect.height-lWidth,
                           aVisibleRect.width - 2 * lWidth, lWidth),
                 aClipRect, effects, opacity,
                 aTransform);
}

} 
} 
