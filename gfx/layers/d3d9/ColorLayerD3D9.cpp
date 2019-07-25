





































#include "ColorLayerD3D9.h"

namespace mozilla {
namespace layers {

Layer*
ColorLayerD3D9::GetLayer()
{
  return this;
}

void
ColorLayerD3D9::RenderLayer()
{
  
  

  nsIntRect visibleRect = mVisibleRegion.GetBounds();

  device()->SetVertexShaderConstantF(
    CBvLayerQuad,
    ShaderConstantRect(visibleRect.x,
                       visibleRect.y,
                       visibleRect.width,
                       visibleRect.height),
    1);

  const gfx3DMatrix& transform = GetEffectiveTransform();
  device()->SetVertexShaderConstantF(CBmLayerTransform, &transform._11, 4);

  float color[4];
  float opacity = GetEffectiveOpacity();
  
  color[0] = (float)(mColor.r * opacity);
  color[1] = (float)(mColor.g * opacity);
  color[2] = (float)(mColor.b * opacity);
  color[3] = (float)(mColor.a * opacity);

  device()->SetPixelShaderConstantF(0, color, 1);

  mD3DManager->SetShaderMode(DeviceManagerD3D9::SOLIDCOLORLAYER);

  device()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
}

} 
} 
