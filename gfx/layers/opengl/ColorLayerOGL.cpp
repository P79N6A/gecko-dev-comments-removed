





































#include "ColorLayerOGL.h"

namespace mozilla {
namespace layers {

static void
RenderColorLayer(ColorLayer* aLayer, LayerManagerOGL *aManager,
                 const nsIntPoint& aOffset)
{
  aManager->MakeCurrent();

  

  nsIntRect visibleRect = aLayer->GetEffectiveVisibleRegion().GetBounds();
  
  




  float opacity = aLayer->GetEffectiveOpacity();
  gfxRGBA color(aLayer->GetColor());
  color.r *= opacity;
  color.g *= opacity;
  color.b *= opacity;
  color.a *= opacity;

  SolidColorLayerProgram *program = aManager->GetColorLayerProgram();
  program->Activate();
  program->SetLayerQuadRect(visibleRect);
  program->SetLayerTransform(aLayer->GetEffectiveTransform());
  program->SetRenderOffset(aOffset);
  program->SetRenderColor(color);

  aManager->BindAndDrawQuad(program);

  DEBUG_GL_ERROR_CHECK(aManager->gl());
}

void
ColorLayerOGL::RenderLayer(int,
                           const nsIntPoint& aOffset)
{
  return RenderColorLayer(this, mOGLManager, aOffset);
}

#ifdef MOZ_IPC
void
ShadowColorLayerOGL::RenderLayer(int,
                                 const nsIntPoint& aOffset)
{
  return RenderColorLayer(this, mOGLManager, aOffset);
}
#endif  


} 
} 
