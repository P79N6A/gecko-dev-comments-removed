





































#include "ColorLayerOGL.h"

namespace mozilla {
namespace layers {

Layer*
ColorLayerOGL::GetLayer()
{
  return this;
}

void
ColorLayerOGL::RenderLayer(int,
                           const nsIntPoint& aOffset)
{
  mOGLManager->MakeCurrent();

  

  nsIntRect visibleRect = GetEffectiveVisibleRegion().GetBounds();
  
  




  float opacity = GetOpacity();
  gfxRGBA color(mColor);
  color.r *= opacity;
  color.g *= opacity;
  color.b *= opacity;
  color.a *= opacity;

  SolidColorLayerProgram *program = mOGLManager->GetColorLayerProgram();
  program->Activate();
  program->SetLayerQuadRect(visibleRect);
  program->SetLayerTransform(GetEffectiveTransform());
  program->SetRenderOffset(aOffset);
  program->SetRenderColor(color);

  mOGLManager->BindAndDrawQuad(program);

  DEBUG_GL_ERROR_CHECK(gl());
}

} 
} 
