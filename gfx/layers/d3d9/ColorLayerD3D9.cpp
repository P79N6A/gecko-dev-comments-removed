




#include "ColorLayerD3D9.h"

namespace mozilla {
namespace layers {

Layer*
ColorLayerD3D9::GetLayer()
{
  return this;
}

static void
RenderColorLayerD3D9(ColorLayer* aLayer, LayerManagerD3D9 *aManager)
{
  
  
  if (aManager->CompositingDisabled()) {
    return;
  }

  nsIntRect bounds = aLayer->GetBounds();

  aManager->device()->SetVertexShaderConstantF(
    CBvLayerQuad,
    ShaderConstantRect(bounds.x,
                       bounds.y,
                       bounds.width,
                       bounds.height),
    1);

  const gfx::Matrix4x4& transform = aLayer->GetEffectiveTransform();
  aManager->device()->SetVertexShaderConstantF(CBmLayerTransform, &transform._11, 4);

  gfxRGBA layerColor(aLayer->GetColor());
  float color[4];
  float opacity = aLayer->GetEffectiveOpacity() * layerColor.a;
  
  
  color[0] = (float)(layerColor.r * opacity);
  color[1] = (float)(layerColor.g * opacity);
  color[2] = (float)(layerColor.b * opacity);
  color[3] = (float)(opacity);

  aManager->device()->SetPixelShaderConstantF(CBvColor, color, 1);

  aManager->SetShaderMode(DeviceManagerD3D9::SOLIDCOLORLAYER,
                          aLayer->GetMaskLayer());

  aManager->device()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
}

void
ColorLayerD3D9::RenderLayer()
{
  return RenderColorLayerD3D9(this, mD3DManager);
}

} 
} 
