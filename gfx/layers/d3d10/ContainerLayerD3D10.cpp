




































#include "ContainerLayerD3D10.h"
#include "nsAlgorithm.h"

namespace mozilla {
namespace layers {

ContainerLayerD3D10::ContainerLayerD3D10(LayerManagerD3D10 *aManager)
  : ContainerLayer(aManager, NULL)
  , LayerD3D10(aManager)
{
  mImplData = static_cast<LayerD3D10*>(this);
}

ContainerLayerD3D10::~ContainerLayerD3D10()
{
  while (mFirstChild) {
    RemoveChild(mFirstChild);
  }
}

void
ContainerLayerD3D10::InsertAfter(Layer* aChild, Layer* aAfter)
{
  aChild->SetParent(this);
  if (!aAfter) {
    Layer *oldFirstChild = GetFirstChild();
    mFirstChild = aChild;
    aChild->SetNextSibling(oldFirstChild);
    aChild->SetPrevSibling(nsnull);
    if (oldFirstChild) {
      oldFirstChild->SetPrevSibling(aChild);
    }
    NS_ADDREF(aChild);
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
      }
      aChild->SetPrevSibling(child);
      NS_ADDREF(aChild);
      return;
    }
  }
  NS_WARNING("Failed to find aAfter layer!");
}

void
ContainerLayerD3D10::RemoveChild(Layer *aChild)
{
  if (GetFirstChild() == aChild) {
    mFirstChild = GetFirstChild()->GetNextSibling();
    if (mFirstChild) {
      mFirstChild->SetPrevSibling(nsnull);
    }
    aChild->SetNextSibling(nsnull);
    aChild->SetPrevSibling(nsnull);
    aChild->SetParent(nsnull);
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
      }
      child->SetNextSibling(nsnull);
      child->SetPrevSibling(nsnull);
      child->SetParent(nsnull);
      NS_RELEASE(aChild);
      return;
    }
    lastChild = child;
  }
}

Layer*
ContainerLayerD3D10::GetLayer()
{
  return this;
}

LayerD3D10*
ContainerLayerD3D10::GetFirstChildD3D10()
{
  if (!mFirstChild) {
    return nsnull;
  }
  return static_cast<LayerD3D10*>(mFirstChild->ImplData());
}

void
ContainerLayerD3D10::RenderLayer()
{
  float renderTargetOffset[] = { 0, 0 };

  nsIntRect visibleRect = mVisibleRegion.GetBounds();
  float opacity = GetEffectiveOpacity();
  PRBool useIntermediate = UseIntermediateSurface();

  nsRefPtr<ID3D10RenderTargetView> previousRTView;
  nsRefPtr<ID3D10Texture2D> renderTexture;
  nsRefPtr<ID3D10RenderTargetView> rtView;
  float previousRenderTargetOffset[2];
  nsIntSize previousViewportSize;

  gfx3DMatrix oldViewMatrix;

  if (useIntermediate) {
    device()->OMGetRenderTargets(1, getter_AddRefs(previousRTView), NULL);
 
    D3D10_TEXTURE2D_DESC desc;
    memset(&desc, 0, sizeof(D3D10_TEXTURE2D_DESC));
    desc.ArraySize = 1;
    desc.MipLevels = 1;
    desc.Width = visibleRect.width;
    desc.Height = visibleRect.height;
    desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
    desc.SampleDesc.Count = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    device()->CreateTexture2D(&desc, NULL, getter_AddRefs(renderTexture));
    
    device()->CreateRenderTargetView(renderTexture, NULL, getter_AddRefs(rtView));

    float black[] = { 0, 0, 0, 0};
    device()->ClearRenderTargetView(rtView, black);

    ID3D10RenderTargetView *rtViewPtr = rtView;
    device()->OMSetRenderTargets(1, &rtViewPtr, NULL);

    effect()->GetVariableByName("vRenderTargetOffset")->
      GetRawValue(previousRenderTargetOffset, 0, 8);

    renderTargetOffset[0] = (float)visibleRect.x;
    renderTargetOffset[1] = (float)visibleRect.y;
    effect()->GetVariableByName("vRenderTargetOffset")->
      SetRawValue(renderTargetOffset, 0, 8);

    previousViewportSize = mD3DManager->GetViewport();
    mD3DManager->SetViewport(nsIntSize(visibleRect.Size()));
  }

  


  LayerD3D10 *layerToRender = GetFirstChildD3D10();
  while (layerToRender) {
    const nsIntRect *clipRect = layerToRender->GetLayer()->GetClipRect();

    D3D10_RECT oldScissor;
    if (clipRect || useIntermediate) {
      UINT numRects = 1;
      device()->RSGetScissorRects(&numRects, &oldScissor);

      RECT r;
      if (clipRect) {
        r.left = (LONG)(clipRect->x - renderTargetOffset[0]);
        r.top = (LONG)(clipRect->y - renderTargetOffset[1]);
        r.right = (LONG)(clipRect->x - renderTargetOffset[0] + clipRect->width);
        r.bottom = (LONG)(clipRect->y - renderTargetOffset[1] + clipRect->height);
      } else {
        
        r.left = 0;
        r.top = 0;
        r.right = visibleRect.width;
        r.bottom = visibleRect.height;
      }

      D3D10_RECT d3drect;
      if (!useIntermediate) {
        
        r.left = NS_MAX<PRInt32>(oldScissor.left, r.left);
        r.right = NS_MIN<PRInt32>(oldScissor.right, r.right);
        r.top = NS_MAX<PRInt32>(oldScissor.top, r.top);
        r.bottom = NS_MIN<PRInt32>(oldScissor.bottom, r.bottom);
      }

      if (r.left >= r.right || r.top >= r.bottom) {
        
        Layer *nextSibling = layerToRender->GetLayer()->GetNextSibling();
        layerToRender = nextSibling ? static_cast<LayerD3D10*>(nextSibling->
                                                              ImplData())
                                    : nsnull;
        continue;
      }

      d3drect.left = NS_MAX<PRInt32>(r.left, 0);
      d3drect.top = NS_MAX<PRInt32>(r.top, 0);
      d3drect.bottom = r.bottom;
      d3drect.right = r.right;

      device()->RSSetScissorRects(1, &d3drect);
    }

    
    layerToRender->RenderLayer();

    if (clipRect || useIntermediate) {
      device()->RSSetScissorRects(1, &oldScissor);
    }

    Layer *nextSibling = layerToRender->GetLayer()->GetNextSibling();
    layerToRender = nextSibling ? static_cast<LayerD3D10*>(nextSibling->
                                                          ImplData())
                                : nsnull;
  }

  if (useIntermediate) {
    mD3DManager->SetViewport(previousViewportSize);
    ID3D10RenderTargetView *rtView = previousRTView;
    device()->OMSetRenderTargets(1, &rtView, NULL);
    effect()->GetVariableByName("vRenderTargetOffset")->
      SetRawValue(previousRenderTargetOffset, 0, 8);

    SetEffectTransformAndOpacity();

    ID3D10EffectTechnique *technique;
    technique = effect()->GetTechniqueByName("RenderRGBALayerPremul");

    effect()->GetVariableByName("vLayerQuad")->AsVector()->SetFloatVector(
      ShaderConstantRectD3D10(
        (float)visibleRect.x,
        (float)visibleRect.y,
        (float)visibleRect.width,
        (float)visibleRect.height)
      );

    technique->GetPassByIndex(0)->Apply(0);

    ID3D10ShaderResourceView *view;
    device()->CreateShaderResourceView(renderTexture, NULL, &view);
    device()->PSSetShaderResources(0, 1, &view);    
    device()->Draw(4, 0);
    view->Release();
  }
}

void
ContainerLayerD3D10::LayerManagerDestroyed()
{
  while (mFirstChild) {
    GetFirstChildD3D10()->LayerManagerDestroyed();
    RemoveChild(mFirstChild);
  }
}

void
ContainerLayerD3D10::Validate()
{
  Layer *layer = GetFirstChild();
  while (layer) {
    static_cast<LayerD3D10*>(layer->ImplData())->Validate();
    layer = layer->GetNextSibling();
  }
}

} 
} 
