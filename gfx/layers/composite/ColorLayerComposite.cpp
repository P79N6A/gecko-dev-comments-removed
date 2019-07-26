




#include "ColorLayerComposite.h"
#include "gfx2DGlue.h"                  
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
#include "nsPoint.h"                    
#include "nsRect.h"                     

namespace mozilla {
namespace layers {

void
ColorLayerComposite::RenderLayer(const nsIntRect& aClipRect)
{
  EffectChain effects;
  gfxRGBA color(GetColor());
  effects.mPrimaryEffect = new EffectSolidColor(gfx::Color(color.r,
                                                           color.g,
                                                           color.b,
                                                           color.a));
  nsIntRect boundRect = GetBounds();

  LayerManagerComposite::AutoAddMaskEffect autoMaskEffect(GetMaskLayer(),
                                                          effects);

  gfx::Rect rect(boundRect.x, boundRect.y,
                 boundRect.width, boundRect.height);
  gfx::Rect clipRect(aClipRect.x, aClipRect.y,
                     aClipRect.width, aClipRect.height);

  float opacity = GetEffectiveOpacity();

  gfx::Matrix4x4 transform;
  ToMatrix4x4(GetEffectiveTransform(), transform);

  mCompositor->DrawQuad(rect, clipRect, effects, opacity, transform);
  mCompositor->DrawDiagnostics(DIAGNOSTIC_COLOR,
                               rect, clipRect,
                               transform);
}

} 
} 
