




#include "ColorLayerD3D10.h"

#include "../d3d9/Nv3DVUtils.h"

namespace mozilla {
namespace layers {

ColorLayerD3D10::ColorLayerD3D10(LayerManagerD3D10 *aManager)
  : ColorLayer(aManager, NULL)
  , LayerD3D10(aManager)
{
  mImplData = static_cast<LayerD3D10*>(this);
}

Layer*
ColorLayerD3D10::GetLayer()
{
  return this;
}

void
ColorLayerD3D10::RenderLayer()
{
  float color[4];
  
  
  float opacity = GetEffectiveOpacity() * mColor.a;
  color[0] = (float)(mColor.r * opacity);
  color[1] = (float)(mColor.g * opacity);
  color[2] = (float)(mColor.b * opacity);
  color[3] = opacity;

  const gfx3DMatrix& transform = GetEffectiveTransform();
  void* raw = &const_cast<gfx3DMatrix&>(transform)._11;
  effect()->GetVariableByName("mLayerTransform")->SetRawValue(raw, 0, 64);
  effect()->GetVariableByName("fLayerColor")->AsVector()->SetFloatVector(color);

  ID3D10EffectTechnique *technique = SelectShader(SHADER_SOLID | LoadMaskTexture());

  nsIntRect bounds = GetBounds();

  effect()->GetVariableByName("vLayerQuad")->AsVector()->SetFloatVector(
    ShaderConstantRectD3D10(
      (float)bounds.x,
      (float)bounds.y,
      (float)bounds.width,
      (float)bounds.height)
    );

  technique->GetPassByIndex(0)->Apply(0);
  device()->Draw(4, 0);
}

} 
} 
