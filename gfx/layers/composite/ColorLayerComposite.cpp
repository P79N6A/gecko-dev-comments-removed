




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

using namespace mozilla::gfx;

void
ColorLayerComposite::RenderLayer(const IntRect& aClipRect)
{
  Rect rect(GetBounds());
  const Matrix4x4& transform = GetEffectiveTransform();

  RenderWithAllMasks(this, mCompositor, aClipRect,
                     [&](EffectChain& effectChain, const Rect& clipRect) {
    GenEffectChain(effectChain);
    mCompositor->DrawQuad(rect, clipRect, effectChain, GetEffectiveOpacity(),
                          transform);
  });

  mCompositor->DrawDiagnostics(DiagnosticFlags::COLOR, rect, Rect(aClipRect),
                               transform);
}

void
ColorLayerComposite::GenEffectChain(EffectChain& aEffect)
{
  aEffect.mLayerRef = this;
  gfxRGBA color(GetColor());
  aEffect.mPrimaryEffect = new EffectSolidColor(
      Color(color.r, color.g, color.b, color.a));
}

} 
} 
