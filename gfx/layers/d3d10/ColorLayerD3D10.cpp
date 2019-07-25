





































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
ColorLayerD3D10::RenderLayer(float aOpacity, const gfx3DMatrix &aTransform)
{
  float color[4];
  
  color[0] = (float)(mColor.r * GetOpacity() * aOpacity);
  color[1] = (float)(mColor.g * GetOpacity() * aOpacity);
  color[2] = (float)(mColor.b * GetOpacity() * aOpacity);
  color[3] = (float)(mColor.a * GetOpacity() * aOpacity);

  gfx3DMatrix transform = mTransform * aTransform;

  effect()->GetVariableByName("mLayerTransform")->SetRawValue(&transform._11, 0, 64);
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
