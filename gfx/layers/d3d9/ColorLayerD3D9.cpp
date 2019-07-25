





































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
  
  

  float quadTransform[4][4];
  nsIntRect visibleRect = mVisibleRegion.GetBounds();
  
  memset(&quadTransform, 0, sizeof(quadTransform));
  quadTransform[0][0] = (float)visibleRect.width;
  quadTransform[1][1] = (float)visibleRect.height;
  quadTransform[2][2] = 1.0f;
  quadTransform[3][0] = (float)visibleRect.x;
  quadTransform[3][1] = (float)visibleRect.y;
  quadTransform[3][3] = 1.0f;

  device()->SetVertexShaderConstantF(0, &quadTransform[0][0], 4);
  device()->SetVertexShaderConstantF(4, &mTransform._11, 4);

  float color[4];
  
  color[0] = (float)(mColor.r * GetOpacity());
  color[1] = (float)(mColor.g * GetOpacity());
  color[2] = (float)(mColor.b * GetOpacity());
  color[3] = (float)(mColor.a * GetOpacity());

  device()->SetPixelShaderConstantF(0, color, 1);

  mD3DManager->SetShaderMode(DeviceManagerD3D9::SOLIDCOLORLAYER);

  device()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
}

} 
} 
