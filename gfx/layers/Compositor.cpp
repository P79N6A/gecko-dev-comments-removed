




#include "mozilla/layers/Compositor.h"
#include "mozilla/layers/Effects.h"
#include "mozilla/layers/CompositorParent.h"

namespace mozilla {
namespace layers {

 LayersBackend Compositor::sBackend = LAYERS_NONE;
 LayersBackend
Compositor::GetBackend()
{
  MOZ_ASSERT(CompositorParent::CompositorLoop() ==
             MessageLoop::current(),
             "Can only call this from the compositor thread!");
  return sBackend;
}

void
Compositor::DrawDiagnostics(const gfx::Color& aColor,
                            const gfx::Rect& rect,
                            const gfx::Rect& aClipRect,
                            const gfx::Matrix4x4& aTransform,
                            const gfx::Point& aOffset)
{
  if (!mDrawColoredBorders) {
    return;
  }
  EffectChain effects;
  effects.mPrimaryEffect = new EffectSolidColor(aColor);
  int lWidth = 1;
  float opacity = 0.8;
  
  this->DrawQuad(gfx::Rect(rect.x, rect.y,
                           lWidth, rect.height),
                 aClipRect, effects, opacity,
                 aTransform, aOffset);
  
  this->DrawQuad(gfx::Rect(rect.x + lWidth, rect.y,
                           rect.width - 2 * lWidth, lWidth),
                 aClipRect, effects, opacity,
                 aTransform, aOffset);
  
  this->DrawQuad(gfx::Rect(rect.x + rect.width - lWidth, rect.y,
                           lWidth, rect.height),
                 aClipRect, effects, opacity,
                 aTransform, aOffset);
  
  this->DrawQuad(gfx::Rect(rect.x + lWidth, rect.y + rect.height-lWidth,
                           rect.width - 2 * lWidth, lWidth),
                 aClipRect, effects, opacity,
                 aTransform, aOffset);
}

} 
} 
