




#include "ColorLayerComposite.h"
#include "mozilla/layers/Effects.h"
#include "gfx2DGlue.h"

namespace mozilla {
namespace layers {

void
ColorLayerComposite::RenderLayer(const nsIntPoint& aOffset,
                                 const nsIntRect& aClipRect)
{
  EffectChain effects;
  gfxRGBA color(GetColor());
  effects.mPrimaryEffect = new EffectSolidColor(gfx::Color(color.r,
                                                           color.g,
                                                           color.b,
                                                           color.a));
  nsIntRect boundRect = GetBounds();

  LayerManagerComposite::AddMaskEffect(GetMaskLayer(), effects);

  gfx::Rect rect(boundRect.x, boundRect.y,
                 boundRect.width, boundRect.height);
  gfx::Rect clipRect(aClipRect.x, aClipRect.y,
                     aClipRect.width, aClipRect.height);

  float opacity = GetEffectiveOpacity();

  gfx::Matrix4x4 transform;
  ToMatrix4x4(GetEffectiveTransform(), transform);

  mCompositor->DrawQuad(rect, clipRect, effects, opacity,
                        transform, gfx::Point(aOffset.x, aOffset.y));
  mCompositor->DrawDiagnostics(gfx::Color(0.0, 1.0, 1.0, 1.0),
                               rect, clipRect,
                               transform, gfx::Point(aOffset.x, aOffset.y));

}

} 
} 
