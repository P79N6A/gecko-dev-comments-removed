





































#include "ColorLayerOGL.h"

namespace mozilla {
namespace layers {

static void
RenderColorLayer(ColorLayer* aLayer, LayerManagerOGL *aManager,
                 const nsIntPoint& aOffset, float aOpacity,
                 const gfx3DMatrix& aMatrix)
{
  aManager->MakeCurrent();

  

  nsIntRect visibleRect = aLayer->GetEffectiveVisibleRegion().GetBounds();
  
  




  float opacity = aLayer->GetOpacity() * aOpacity;
  gfxRGBA color(aLayer->GetColor());
  color.r *= opacity;
  color.g *= opacity;
  color.b *= opacity;
  color.a *= opacity;

  SolidColorLayerProgram *program = aManager->GetColorLayerProgram();
  program->Activate();
  program->SetLayerQuadRect(visibleRect);
  program->SetLayerTransform(aLayer->GetEffectiveTransform() * aMatrix);
  program->SetRenderOffset(aOffset);
  program->SetRenderColor(color);

  aManager->BindAndDrawQuad(program);

  DEBUG_GL_ERROR_CHECK(aManager->gl());
}

void
ColorLayerOGL::RenderLayer(int,
                           const nsIntPoint& aOffset,
                           float aOpacity,
                           const gfx3DMatrix& aMatrix)
{
  return RenderColorLayer(this, mOGLManager, aOffset, aOpacity, aMatrix);
}

#ifdef MOZ_IPC
void
ShadowColorLayerOGL::RenderLayer(int,
                                 const nsIntPoint& aOffset,
                                 float aOpacity,
                                 const gfx3DMatrix& aMatrix)
{
  return RenderColorLayer(this, mOGLManager, aOffset, aOpacity, aMatrix);
}
#endif  


} 
} 
