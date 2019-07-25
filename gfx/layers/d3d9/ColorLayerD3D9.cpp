





































#include "ColorLayerD3D9.h"

namespace mozilla {
namespace layers {

Layer*
ColorLayerD3D9::GetLayer()
{
  return this;
}

void
ColorLayerD3D9::RenderLayer(float aOpacity, const gfx3DMatrix &aTransform)
{
  
  

  nsIntRect visibleRect = mVisibleRegion.GetBounds();

  device()->SetVertexShaderConstantF(
    CBvLayerQuad,
    ShaderConstantRect(visibleRect.x,
                       visibleRect.y,
                       visibleRect.width,
                       visibleRect.height),
    1);

  gfx3DMatrix transform = mTransform * aTransform;
  device()->SetVertexShaderConstantF(CBmLayerTransform, &transform._11, 4);

  float color[4];
  
  color[0] = (float)(mColor.r * GetOpacity() * aOpacity);
  color[1] = (float)(mColor.g * GetOpacity() * aOpacity);
  color[2] = (float)(mColor.b * GetOpacity() * aOpacity);
  color[3] = (float)(mColor.a * GetOpacity() * aOpacity);

  device()->SetPixelShaderConstantF(0, color, 1);

  mD3DManager->SetShaderMode(DeviceManagerD3D9::SOLIDCOLORLAYER);

  device()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
}

} 
} 
