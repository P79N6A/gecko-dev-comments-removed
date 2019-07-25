





































#include "ColorLayerOGL.h"

namespace mozilla {
namespace layers {

static void
RenderColorLayer(ColorLayer* aLayer, LayerManagerOGL *aManager,
                 const nsIntPoint& aOffset)
{
  aManager->MakeCurrent();

  

  nsIntRect visibleRect = aLayer->GetEffectiveVisibleRegion().GetBounds();
  
  




  gfxRGBA color(aLayer->GetColor());
  float opacity = aLayer->GetEffectiveOpacity() * color.a;
  color.r *= opacity;
  color.g *= opacity;
  color.b *= opacity;
  color.a = opacity;

  SolidColorLayerProgram *program = aManager->GetColorLayerProgram();
  program->Activate();
  program->SetLayerQuadRect(visibleRect);
  program->SetLayerTransform(aLayer->GetEffectiveTransform());
  program->SetRenderOffset(aOffset);
  program->SetRenderColor(color);

  aManager->BindAndDrawQuad(program);
}

void
ColorLayerOGL::RenderLayer(int,
                           const nsIntPoint& aOffset)
{
  return RenderColorLayer(this, mOGLManager, aOffset);
}

void
ShadowColorLayerOGL::RenderLayer(int,
                                 const nsIntPoint& aOffset)
{
  return RenderColorLayer(this, mOGLManager, aOffset);
}


} 
} 
