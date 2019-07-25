




































#include "ContainerLayerD3D9.h"
#include "gfxUtils.h"
#include "nsRect.h"
#include "ThebesLayerD3D9.h"
#include "ReadbackProcessor.h"

namespace mozilla {
namespace layers {

ContainerLayerD3D9::ContainerLayerD3D9(LayerManagerD3D9 *aManager)
  : ContainerLayer(aManager, NULL)
  , LayerD3D9(aManager)
{
  mImplData = static_cast<LayerD3D9*>(this);
}

ContainerLayerD3D9::~ContainerLayerD3D9()
{
  while (mFirstChild) {
    RemoveChild(mFirstChild);
  }
}

void
ContainerLayerD3D9::InsertAfter(Layer* aChild, Layer* aAfter)
{
  aChild->SetParent(this);
  if (!aAfter) {
    Layer *oldFirstChild = GetFirstChild();
    mFirstChild = aChild;
    aChild->SetNextSibling(oldFirstChild);
    aChild->SetPrevSibling(nsnull);
    if (oldFirstChild) {
      oldFirstChild->SetPrevSibling(aChild);
    } else {
      mLastChild = aChild;
    }
    NS_ADDREF(aChild);
    DidInsertChild(aChild);
    return;
  }
  for (Layer *child = GetFirstChild();
       child; child = child->GetNextSibling()) {
    if (aAfter == child) {
      Layer *oldNextSibling = child->GetNextSibling();
      child->SetNextSibling(aChild);
      aChild->SetNextSibling(oldNextSibling);
      if (oldNextSibling) {
        oldNextSibling->SetPrevSibling(aChild);
      } else {
        mLastChild = aChild;
      }
      aChild->SetPrevSibling(child);
      NS_ADDREF(aChild);
      DidInsertChild(aChild);
      return;
    }
  }
  NS_WARNING("Failed to find aAfter layer!");
}

void
ContainerLayerD3D9::RemoveChild(Layer *aChild)
{
  if (GetFirstChild() == aChild) {
    mFirstChild = GetFirstChild()->GetNextSibling();
    if (mFirstChild) {
      mFirstChild->SetPrevSibling(nsnull);
    } else {
      mLastChild = nsnull;
    }
    aChild->SetNextSibling(nsnull);
    aChild->SetPrevSibling(nsnull);
    aChild->SetParent(nsnull);
    DidRemoveChild(aChild);
    NS_RELEASE(aChild);
    return;
  }
  Layer *lastChild = nsnull;
  for (Layer *child = GetFirstChild(); child;
       child = child->GetNextSibling()) {
    if (child == aChild) {
      
      lastChild->SetNextSibling(child->GetNextSibling());
      if (child->GetNextSibling()) {
        child->GetNextSibling()->SetPrevSibling(lastChild);
      } else {
        mLastChild = lastChild;
      }
      child->SetNextSibling(nsnull);
      child->SetPrevSibling(nsnull);
      child->SetParent(nsnull);
      DidRemoveChild(aChild);
      NS_RELEASE(aChild);
      return;
    }
    lastChild = child;
  }
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

static inline LayerD3D9*
GetNextSiblingD3D9(LayerD3D9* aLayer)
{
   Layer* layer = aLayer->GetLayer()->GetNextSibling();
   return layer ? static_cast<LayerD3D9*>(layer->
                                          ImplData())
                 : nsnull;
}

static PRBool
HasOpaqueAncestorLayer(Layer* aLayer)
{
  for (Layer* l = aLayer->GetParent(); l; l = l->GetParent()) {
    if (l->GetContentFlags() & Layer::CONTENT_OPAQUE)
      return PR_TRUE;
  }
  return PR_FALSE;
}

void
ContainerLayerD3D9::RenderLayer()
{
  nsRefPtr<IDirect3DSurface9> previousRenderTarget;
  nsRefPtr<IDirect3DTexture9> renderTexture;
  float previousRenderTargetOffset[4];
  float renderTargetOffset[] = { 0, 0, 0, 0 };
  float oldViewMatrix[4][4];

  RECT containerD3D9ClipRect; 
  device()->GetScissorRect(&containerD3D9ClipRect);
  
  
  nsIntRect oldScissor(containerD3D9ClipRect.left, 
                       containerD3D9ClipRect.top,
                       containerD3D9ClipRect.right - containerD3D9ClipRect.left,
                       containerD3D9ClipRect.bottom - containerD3D9ClipRect.top);

  ReadbackProcessor readback;
  readback.BuildUpdates(this);

  nsIntRect visibleRect = mVisibleRegion.GetBounds();
  PRBool useIntermediate = UseIntermediateSurface();

  mSupportsComponentAlphaChildren = PR_FALSE;
  if (useIntermediate) {
    device()->GetRenderTarget(0, getter_AddRefs(previousRenderTarget));
    device()->CreateTexture(visibleRect.width, visibleRect.height, 1,
                            D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
                            D3DPOOL_DEFAULT, getter_AddRefs(renderTexture),
                            NULL);
    nsRefPtr<IDirect3DSurface9> renderSurface;
    renderTexture->GetSurfaceLevel(0, getter_AddRefs(renderSurface));
    device()->SetRenderTarget(0, renderSurface);

    if (mVisibleRegion.GetNumRects() == 1 && (GetContentFlags() & CONTENT_OPAQUE)) {
      
      mSupportsComponentAlphaChildren = PR_TRUE;
    } else {
      const gfx3DMatrix& transform3D = GetEffectiveTransform();
      gfxMatrix transform;
      
      
      
      
      HRESULT hr = E_FAIL;
      if (HasOpaqueAncestorLayer(this) &&
          transform3D.Is2D(&transform) && !transform.HasNonIntegerTranslation()) {
        
        RECT dest = { 0, 0, visibleRect.width, visibleRect.height };
        RECT src = dest;
        ::OffsetRect(&src,
                     visibleRect.x + PRInt32(transform.x0),
                     visibleRect.y + PRInt32(transform.y0));
        hr = device()->
          StretchRect(previousRenderTarget, &src, renderSurface, &dest, D3DTEXF_NONE);
      }
      if (hr == S_OK) {
        mSupportsComponentAlphaChildren = PR_TRUE;
      } else {
        device()->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 0), 0, 0);
      }
    }

    device()->GetVertexShaderConstantF(CBvRenderTargetOffset, previousRenderTargetOffset, 1);
    renderTargetOffset[0] = (float)visibleRect.x;
    renderTargetOffset[1] = (float)visibleRect.y;
    device()->SetVertexShaderConstantF(CBvRenderTargetOffset, renderTargetOffset, 1);

    gfx3DMatrix viewMatrix;
    



    viewMatrix._11 = 2.0f / visibleRect.width;
    viewMatrix._22 = -2.0f / visibleRect.height;
    viewMatrix._41 = -1.0f;
    viewMatrix._42 = 1.0f;

    device()->GetVertexShaderConstantF(CBmProjection, &oldViewMatrix[0][0], 4);
    device()->SetVertexShaderConstantF(CBmProjection, &viewMatrix._11, 4);
  } else {
    mSupportsComponentAlphaChildren = (GetContentFlags() & CONTENT_OPAQUE) ||
        (mParent && mParent->SupportsComponentAlphaChildren());
  }

  


  for (LayerD3D9* layerToRender = GetFirstChildD3D9();
       layerToRender != nsnull;
       layerToRender = GetNextSiblingD3D9(layerToRender)) {

    if (layerToRender->GetLayer()->GetEffectiveVisibleRegion().IsEmpty()) {
      continue;
    }
    
    nsIntRect scissorRect =
      layerToRender->GetLayer()->CalculateScissorRect(oldScissor, nsnull);
    if (scissorRect.IsEmpty()) {
      continue;
    }

    RECT d3drect;
    d3drect.left = scissorRect.x;
    d3drect.top = scissorRect.y;
    d3drect.right = scissorRect.x + scissorRect.width;
    d3drect.bottom = scissorRect.y + scissorRect.height;
    device()->SetScissorRect(&d3drect);

    if (layerToRender->GetLayer()->GetType() == TYPE_THEBES) {
      static_cast<ThebesLayerD3D9*>(layerToRender)->RenderThebesLayer(&readback);
    } else {
      layerToRender->RenderLayer();
    }
  }
    
  device()->SetScissorRect(&containerD3D9ClipRect);

  if (useIntermediate) {
    device()->SetRenderTarget(0, previousRenderTarget);
    device()->SetVertexShaderConstantF(CBvRenderTargetOffset, previousRenderTargetOffset, 1);
    device()->SetVertexShaderConstantF(CBmProjection, &oldViewMatrix[0][0], 4);

    device()->SetVertexShaderConstantF(CBvLayerQuad,
                                       ShaderConstantRect(visibleRect.x,
                                                          visibleRect.y,
                                                          visibleRect.width,
                                                          visibleRect.height),
                                       1);

    SetShaderTransformAndOpacity();

    mD3DManager->SetShaderMode(DeviceManagerD3D9::RGBALAYER);

    device()->SetTexture(0, renderTexture);
    device()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
  }
}

void
ContainerLayerD3D9::LayerManagerDestroyed()
{
  while (mFirstChild) {
    GetFirstChildD3D9()->LayerManagerDestroyed();
    RemoveChild(mFirstChild);
  }
}

} 
} 
