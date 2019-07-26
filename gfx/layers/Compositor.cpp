




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

 LayersBackend Compositor::sBackend = LAYERS_NONE;
 LayersBackend
Compositor::GetBackend()
{
  AssertOnCompositorThread();
  return sBackend;
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
  if ((aFlags & DIAGNOSTIC_TILE) && !(mDiagnosticTypes & DIAGNOSTIC_TILE_BORDERS)) {
    return false;
  }
  if ((aFlags & DIAGNOSTIC_BIGIMAGE) &&
      !(mDiagnosticTypes & DIAGNOSTIC_BIGIMAGE_BORDERS)) {
    return false;
  }
  if (!mDiagnosticTypes) {
    return false;
  }
  return true;
}

void
Compositor::DrawDiagnostics(DiagnosticFlags aFlags,
                            const nsIntRegion& aVisibleRegion,
                            const gfx::Rect& aClipRect,
                            const gfx::Matrix4x4& aTransform,
                            const gfx::Point& aOffset)
{
  if (!ShouldDrawDiagnostics(aFlags)) {
    return;
  }

  if (aVisibleRegion.GetNumRects() > 1) {
    nsIntRegionRectIterator screenIter(aVisibleRegion);

    while (const nsIntRect* rect = screenIter.Next())
    {
      DrawDiagnostics(aFlags | DIAGNOSTIC_REGION_RECT,
                      ToRect(*rect), aClipRect, aTransform, aOffset);
    }
  }

  DrawDiagnostics(aFlags, ToRect(aVisibleRegion.GetBounds()),
                  aClipRect, aTransform, aOffset);
}

void
Compositor::DrawDiagnostics(DiagnosticFlags aFlags,
                            const gfx::Rect& aVisibleRect,
                            const gfx::Rect& aClipRect,
                            const gfx::Matrix4x4& aTransform,
                            const gfx::Point& aOffset)
{
  if (!ShouldDrawDiagnostics(aFlags)) {
    return;
  }

  DrawDiagnosticsInternal(aFlags, aVisibleRect,
                          aClipRect, aTransform,
                          aOffset);
}

void
Compositor::DrawDiagnosticsInternal(DiagnosticFlags aFlags,
                                    const gfx::Rect& aVisibleRect,
                                    const gfx::Rect& aClipRect,
                                    const gfx::Matrix4x4& aTransform,
                                    const gfx::Point& aOffset)
{
#ifdef MOZ_B2G
  int lWidth = 4;
#elif defined(ANDROID)
  int lWidth = 10;
#else
  int lWidth = 2;
#endif
  float opacity = 0.7;

  gfx::Color color;
  if (aFlags & DIAGNOSTIC_CONTENT) {
    color = gfx::Color(0.0, 1.0, 0.0, 1.0); 
    if (aFlags & DIAGNOSTIC_COMPONENT_ALPHA) {
      color = gfx::Color(0.0, 1.0, 1.0, 1.0); 
    }
  } else if (aFlags & DIAGNOSTIC_IMAGE) {
    color = gfx::Color(1.0, 0.0, 0.0, 1.0); 
  } else if (aFlags & DIAGNOSTIC_COLOR) {
    color = gfx::Color(0.0, 0.0, 1.0, 1.0); 
  } else if (aFlags & DIAGNOSTIC_CONTAINER) {
    color = gfx::Color(0.8, 0.0, 0.8, 1.0); 
  }

  
  if (aFlags & DIAGNOSTIC_TILE ||
      aFlags & DIAGNOSTIC_BIGIMAGE ||
      aFlags & DIAGNOSTIC_REGION_RECT) {
    lWidth = 1;
    opacity = 0.5;
    color.r *= 0.7;
    color.g *= 0.7;
    color.b *= 0.7;
  }

  EffectChain effects;

  effects.mPrimaryEffect = new EffectSolidColor(color);
  
  this->DrawQuad(gfx::Rect(aVisibleRect.x, aVisibleRect.y,
                           lWidth, aVisibleRect.height),
                 aClipRect, effects, opacity,
                 aTransform, aOffset);
  
  this->DrawQuad(gfx::Rect(aVisibleRect.x + lWidth, aVisibleRect.y,
                           aVisibleRect.width - 2 * lWidth, lWidth),
                 aClipRect, effects, opacity,
                 aTransform, aOffset);
  
  this->DrawQuad(gfx::Rect(aVisibleRect.x + aVisibleRect.width - lWidth, aVisibleRect.y,
                           lWidth, aVisibleRect.height),
                 aClipRect, effects, opacity,
                 aTransform, aOffset);
  
  this->DrawQuad(gfx::Rect(aVisibleRect.x + lWidth, aVisibleRect.y + aVisibleRect.height-lWidth,
                           aVisibleRect.width - 2 * lWidth, lWidth),
                 aClipRect, effects, opacity,
                 aTransform, aOffset);
}

} 
} 
