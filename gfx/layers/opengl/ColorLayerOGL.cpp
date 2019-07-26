




#include "ColorLayerOGL.h"
#include "gfxColor.h"                   
#include "LayerManagerOGL.h"            
#include "LayerManagerOGLProgram.h"     

struct nsIntPoint;

namespace mozilla {
namespace layers {

static void
RenderColorLayer(ColorLayer* aLayer, LayerManagerOGL *aManager,
                 const nsIntPoint& aOffset)
{
  if (aManager->CompositingDisabled()) {
    return;
  }

  aManager->MakeCurrent();

  

  




  gfxRGBA color(aLayer->GetColor());
  float opacity = aLayer->GetEffectiveOpacity() * color.a;
  color.r *= opacity;
  color.g *= opacity;
  color.b *= opacity;
  color.a = opacity;

  ShaderProgramOGL *program = aManager->GetProgram(ColorLayerProgramType,
                                                   aLayer->GetMaskLayer());
  program->Activate();
  program->SetProjectionMatrix(aManager->mProjMatrix);
  program->SetLayerQuadRect(aLayer->GetBounds());
  program->SetLayerTransform(aLayer->GetEffectiveTransform());
  program->SetRenderOffset(aOffset);
  program->SetRenderColor(color);
  program->LoadMask(aLayer->GetMaskLayer());

  aManager->BindAndDrawQuad(program);
}

void
ColorLayerOGL::RenderLayer(int,
                           const nsIntPoint& aOffset)
{
  RenderColorLayer(this, mOGLManager, aOffset);
}

} 
} 
