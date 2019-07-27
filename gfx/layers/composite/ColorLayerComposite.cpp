




#include "ColorLayerComposite.h"
#include "gfxColor.h"                   
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/Effects.h"     
#include "mozilla/mozalloc.h"           

namespace mozilla {
namespace layers {

void
ColorLayerComposite::RenderLayer(const gfx::IntRect& aClipRect)
{
  EffectChain effects(this);

  GenEffectChain(effects);

  gfx::IntRect boundRect = GetBounds();

  LayerManagerComposite::AutoAddMaskEffect autoMaskEffect(GetMaskLayer(),
                                                          effects);

  gfx::Rect rect(boundRect.x, boundRect.y,
                 boundRect.width, boundRect.height);
  gfx::Rect clipRect(aClipRect.x, aClipRect.y,
                     aClipRect.width, aClipRect.height);

  float opacity = GetEffectiveOpacity();

  AddBlendModeEffect(effects);

  const gfx::Matrix4x4& transform = GetEffectiveTransform();
  mCompositor->DrawQuad(rect, clipRect, effects, opacity, transform);
  mCompositor->DrawDiagnostics(DiagnosticFlags::COLOR,
                               rect, clipRect,
                               transform);
}

void
ColorLayerComposite::GenEffectChain(EffectChain& aEffect)
{
  aEffect.mLayerRef = this;
  gfxRGBA color(GetColor());
  aEffect.mPrimaryEffect = new EffectSolidColor(gfx::Color(color.r,
                                                           color.g,
                                                           color.b,
                                                           color.a));
}

} 
} 
