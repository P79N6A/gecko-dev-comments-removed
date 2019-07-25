




































#include "ColorLayerOGL.h"

namespace mozilla {
namespace layers {

LayerOGL::LayerType
ColorLayerOGL::GetType()
{
  return TYPE_COLOR;
}

Layer*
ColorLayerOGL::GetLayer()
{
  return this;
}

void
ColorLayerOGL::RenderLayer(int, DrawThebesLayerCallback, void*)
{
  static_cast<LayerManagerOGL*>(mManager)->MakeCurrent();

  

  float quadTransform[4][4];
  nsIntRect visibleRect = mVisibleRegion.GetBounds();
  
  memset(&quadTransform, 0, sizeof(quadTransform));
  quadTransform[0][0] = (float)visibleRect.width;
  quadTransform[1][1] = (float)visibleRect.height;
  quadTransform[2][2] = 1.0f;
  quadTransform[3][0] = (float)visibleRect.x;
  quadTransform[3][1] = (float)visibleRect.y;
  quadTransform[3][3] = 1.0f;
  
  ColorLayerProgram *program =
    static_cast<LayerManagerOGL*>(mManager)->GetColorLayerProgram();

  program->Activate();

  program->SetLayerQuadTransform(&quadTransform[0][0]);

  gfxRGBA color = mColor;
  
  color.r *= GetOpacity();
  color.g *= GetOpacity();
  color.b *= GetOpacity();
  color.a *= GetOpacity();
  program->SetLayerColor(color);
  program->SetLayerTransform(&mTransform._11);
  program->Apply();

  gl()->fDrawArrays(LOCAL_GL_TRIANGLE_STRIP, 0, 4);
}

} 
} 
