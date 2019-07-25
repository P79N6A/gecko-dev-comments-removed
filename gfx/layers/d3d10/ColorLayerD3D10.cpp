





































#include "ColorLayerD3D10.h"

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
  
  float opacity = GetEffectiveOpacity();
  color[0] = (float)(mColor.r * opacity);
  color[1] = (float)(mColor.g * opacity);
  color[2] = (float)(mColor.b * opacity);
  color[3] = (float)(mColor.a * opacity);

  const gfx3DMatrix& transform = GetEffectiveTransform();
  void* raw = &const_cast<gfx3DMatrix&>(transform)._11;
  effect()->GetVariableByName("mLayerTransform")->SetRawValue(raw, 0, 64);
  effect()->GetVariableByName("fLayerColor")->AsVector()->SetFloatVector(color);

  ID3D10EffectTechnique *technique;
  technique = effect()->GetTechniqueByName("RenderSolidColorLayer");

  nsIntRegionRectIterator iter(mVisibleRegion);

  const nsIntRect *iterRect;
  while ((iterRect = iter.Next())) {
    effect()->GetVariableByName("vLayerQuad")->AsVector()->SetFloatVector(
      ShaderConstantRectD3D10(
        (float)iterRect->x,
        (float)iterRect->y,
        (float)iterRect->width,
        (float)iterRect->height)
      );

    technique->GetPassByIndex(0)->Apply(0);
    device()->Draw(4, 0);
  }
}

} 
} 
