




































#include "ContainerLayerD3D9.h"

namespace mozilla {
namespace layers {

ContainerLayerD3D9::ContainerLayerD3D9(LayerManagerD3D9 *aManager)
  : ContainerLayer(aManager, NULL)
  , LayerD3D9(aManager)
{
  mImplData = static_cast<LayerD3D9*>(this);
}

const nsIntRect&
ContainerLayerD3D9::GetVisibleRect()
{
  return mVisibleRect;
}

void
ContainerLayerD3D9::SetVisibleRegion(const nsIntRegion &aRegion)
{
  mVisibleRect = aRegion.GetBounds();
}

void
ContainerLayerD3D9::InsertAfter(Layer* aChild, Layer* aAfter)
{
  LayerD3D9 *newChild = static_cast<LayerD3D9*>(aChild->ImplData());
  aChild->SetParent(this);
  if (!aAfter) {
    LayerD3D9 *oldFirstChild = GetFirstChildD3D9();
    mFirstChild = newChild->GetLayer();
    newChild->SetNextSibling(oldFirstChild);
    NS_ADDREF(aChild);
    return;
  }
  for (LayerD3D9 *child = GetFirstChildD3D9(); 
    child; child = child->GetNextSibling()) {
    if (aAfter == child->GetLayer()) {
      LayerD3D9 *oldNextSibling = child->GetNextSibling();
      child->SetNextSibling(newChild);
      child->GetNextSibling()->SetNextSibling(oldNextSibling);
      NS_ADDREF(aChild);
      return;
    }
  }
  NS_WARNING("Failed to find aAfter layer!");
}

void
ContainerLayerD3D9::RemoveChild(Layer *aChild)
{
  if (GetFirstChild() == aChild) {
    mFirstChild = GetFirstChildD3D9()->GetNextSibling() ?
      GetFirstChildD3D9()->GetNextSibling()->GetLayer() : nsnull;
    NS_RELEASE(aChild);
    return;
  }
  LayerD3D9 *lastChild = NULL;
  for (LayerD3D9 *child = GetFirstChildD3D9(); child; 
    child = child->GetNextSibling()) {
    if (child->GetLayer() == aChild) {
      
      lastChild->SetNextSibling(child->GetNextSibling());
      child->SetNextSibling(NULL);
      child->GetLayer()->SetParent(NULL);
      NS_RELEASE(aChild);
      return;
    }
    lastChild = child;
  }
}

LayerD3D9::LayerType
ContainerLayerD3D9::GetType()
{
  return TYPE_CONTAINER;
}

Layer*
ContainerLayerD3D9::GetLayer()
{
  return this;
}

LayerD3D9*
ContainerLayerD3D9::GetFirstChildD3D9()
{
  if (!mFirstChild) {
    return nsnull;
  }
  return static_cast<LayerD3D9*>(mFirstChild->ImplData());
}

void
ContainerLayerD3D9::RenderLayer()
{
  float opacity = GetOpacity();
  nsRefPtr<IDirect3DSurface9> previousRenderTarget;
  nsRefPtr<IDirect3DTexture9> renderTexture;
  float previousRenderTargetOffset[4];
  float renderTargetOffset[] = { 0, 0, 0, 0 };
  float oldViewMatrix[4][4];

  PRBool useIntermediate = (opacity != 1.0 || !mTransform.IsIdentity());

  if (useIntermediate) {
    device()->GetRenderTarget(0, getter_AddRefs(previousRenderTarget));
    device()->CreateTexture(mVisibleRect.width, mVisibleRect.height, 1,
			    D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
			    D3DPOOL_DEFAULT, getter_AddRefs(renderTexture),
                            NULL);
    nsRefPtr<IDirect3DSurface9> renderSurface;
    renderTexture->GetSurfaceLevel(0, getter_AddRefs(renderSurface));
    device()->SetRenderTarget(0, renderSurface);
    device()->GetVertexShaderConstantF(12, previousRenderTargetOffset, 1);
    renderTargetOffset[0] = (float)GetVisibleRect().x;
    renderTargetOffset[1] = (float)GetVisibleRect().y;
    device()->SetVertexShaderConstantF(12, renderTargetOffset, 1);

    float viewMatrix[4][4];
    



    memset(&viewMatrix, 0, sizeof(viewMatrix));
    viewMatrix[0][0] = 2.0f / mVisibleRect.width;
    viewMatrix[1][1] = -2.0f / mVisibleRect.height;
    viewMatrix[2][2] = 1.0f;
    viewMatrix[3][0] = -1.0f;
    viewMatrix[3][1] = 1.0f;
    viewMatrix[3][3] = 1.0f;
    
    device()->GetVertexShaderConstantF(8, &oldViewMatrix[0][0], 4);
    device()->SetVertexShaderConstantF(8, &viewMatrix[0][0], 4);
  }

  


  LayerD3D9 *layerToRender = GetFirstChildD3D9();
  while (layerToRender) {
    const nsIntRect *clipRect = layerToRender->GetLayer()->GetClipRect();
    RECT r;
    if (clipRect) {
      r.left = (LONG)(clipRect->x - renderTargetOffset[0]);
      r.top = (LONG)(clipRect->y - renderTargetOffset[1]);
      r.right = (LONG)(clipRect->x - renderTargetOffset[0] + clipRect->width);
      r.bottom = (LONG)(clipRect->y - renderTargetOffset[1] + clipRect->height);
    } else {
      if (useIntermediate) {
        r.left = 0;
        r.top = 0;
      } else {
        r.left = GetVisibleRect().x;
        r.top = GetVisibleRect().y;
      }
      r.right = r.left + GetVisibleRect().width;
      r.bottom = r.top + GetVisibleRect().height;
    }

    nsRefPtr<IDirect3DSurface9> renderSurface;
    device()->GetRenderTarget(0, getter_AddRefs(renderSurface));

    D3DSURFACE_DESC desc;
    renderSurface->GetDesc(&desc);

    r.left = NS_MAX<LONG>(0, r.left);
    r.top = NS_MAX<LONG>(0, r.top);
    r.bottom = NS_MIN<LONG>(r.bottom, desc.Height);
    r.right = NS_MIN<LONG>(r.right, desc.Width);

    device()->SetScissorRect(&r);

    layerToRender->RenderLayer();
    layerToRender = layerToRender->GetNextSibling();
  }

  if (useIntermediate) {
    device()->SetRenderTarget(0, previousRenderTarget);
    device()->SetVertexShaderConstantF(12, previousRenderTargetOffset, 1);
    device()->SetVertexShaderConstantF(8, &oldViewMatrix[0][0], 4);

    float quadTransform[4][4];
    






    memset(&quadTransform, 0, sizeof(quadTransform));
    quadTransform[0][0] = (float)GetVisibleRect().width;
    quadTransform[1][1] = (float)GetVisibleRect().height;
    quadTransform[2][2] = 1.0f;
    quadTransform[3][0] = (float)GetVisibleRect().x - 0.5f;
    quadTransform[3][1] = (float)GetVisibleRect().y - 0.5f;
    quadTransform[3][3] = 1.0f;

    device()->SetVertexShaderConstantF(0, &quadTransform[0][0], 4);
    device()->SetVertexShaderConstantF(4, &mTransform._11, 4);

    float opacityVector[4];
    



    opacityVector[0] = opacity;
    device()->SetPixelShaderConstantF(0, opacityVector, 1);

    mD3DManager->SetShaderMode(LayerManagerD3D9::RGBLAYER);

    device()->SetTexture(0, renderTexture);
    device()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
  }
}

} 
} 
