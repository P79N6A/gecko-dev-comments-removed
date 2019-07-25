




































#include "ThebesLayerD3D10.h"
#include "gfxPlatform.h"

#include "gfxWindowsPlatform.h"
#ifdef CAIRO_HAS_D2D_SURFACE
#include "gfxD2DSurface.h"
#endif

namespace mozilla {
namespace layers {

ThebesLayerD3D10::ThebesLayerD3D10(LayerManagerD3D10 *aManager)
  : ThebesLayer(aManager, NULL)
  , LayerD3D10(aManager)
{
  mImplData = static_cast<LayerD3D10*>(this);
}

ThebesLayerD3D10::~ThebesLayerD3D10()
{
}






#define RETENTION_THRESHOLD 16384

void
ThebesLayerD3D10::SetVisibleRegion(const nsIntRegion &aRegion)
{
  if (aRegion.IsEqual(mVisibleRegion)) {
    return;
  }

  nsIntRegion oldVisibleRegion = mVisibleRegion;
  ThebesLayer::SetVisibleRegion(aRegion);

  if (!mTexture) {
    
    
    
    return;
  }

  VerifyContentType();

  nsRefPtr<ID3D10Texture2D> oldTexture = mTexture;

  nsIntRect oldBounds = oldVisibleRegion.GetBounds();
  nsIntRect newBounds = mVisibleRegion.GetBounds();

  CreateNewTexture(gfxIntSize(newBounds.width, newBounds.height));

  
  
  oldVisibleRegion.And(oldVisibleRegion, mVisibleRegion);
  
  oldVisibleRegion.And(oldVisibleRegion, mValidRegion);

  nsIntRect largeRect = oldVisibleRegion.GetLargestRectangle();

  
  
  
  
  
  
  if (!oldTexture || !mTexture ||
      largeRect.width * largeRect.height < RETENTION_THRESHOLD) {
    mValidRegion.SetEmpty();
    return;
  }

  nsIntRegion retainedRegion;
  nsIntRegionRectIterator iter(oldVisibleRegion);
  const nsIntRect *r;
  while ((r = iter.Next())) {
    if (r->width * r->height > RETENTION_THRESHOLD) {
      
      
      D3D10_BOX box;
      box.left = r->x - oldBounds.x;
      box.top = r->y - oldBounds.y;
      box.right = box.left + r->width;
      box.bottom = box.top + r->height;
      box.back = 1.0f;
      box.front = 0;

      device()->CopySubresourceRegion(mTexture, 0,
                                      r->x - newBounds.x,
                                      r->y - newBounds.y,
                                      0,
                                      oldTexture, 0,
                                      &box);

      retainedRegion.Or(retainedRegion, *r);
    }
  }

  
  mValidRegion.And(mValidRegion, retainedRegion);  
}


void
ThebesLayerD3D10::InvalidateRegion(const nsIntRegion &aRegion)
{
  mValidRegion.Sub(mValidRegion, aRegion);
}

void
ThebesLayerD3D10::RenderLayer()
{
  if (!mTexture) {
    return;
  }

  nsIntRect visibleRect = mVisibleRegion.GetBounds();

  SetEffectTransformAndOpacity();

  ID3D10EffectTechnique *technique;
  if (CanUseOpaqueSurface()) {
    technique = effect()->GetTechniqueByName("RenderRGBLayerPremul");
  } else {
    technique = effect()->GetTechniqueByName("RenderRGBALayerPremul");
  }


  nsIntRegionRectIterator iter(mVisibleRegion);

  const nsIntRect *iterRect;
  if (mSRView) {
    effect()->GetVariableByName("tRGB")->AsShaderResource()->SetResource(mSRView);
  }

  while ((iterRect = iter.Next())) {
    effect()->GetVariableByName("vLayerQuad")->AsVector()->SetFloatVector(
      ShaderConstantRectD3D10(
        (float)iterRect->x,
        (float)iterRect->y,
        (float)iterRect->width,
        (float)iterRect->height)
      );

    effect()->GetVariableByName("vTextureCoords")->AsVector()->SetFloatVector(
      ShaderConstantRectD3D10(
        (float)(iterRect->x - visibleRect.x) / (float)visibleRect.width,
        (float)(iterRect->y - visibleRect.y) / (float)visibleRect.height,
        (float)iterRect->width / (float)visibleRect.width,
        (float)iterRect->height / (float)visibleRect.height)
      );

    technique->GetPassByIndex(0)->Apply(0);
    device()->Draw(4, 0);
  }

  
  effect()->GetVariableByName("vTextureCoords")->AsVector()->
    SetFloatVector(ShaderConstantRectD3D10(0, 0, 1.0f, 1.0f));
}

void
ThebesLayerD3D10::Validate()
{
  if (mVisibleRegion.IsEmpty()) {
    return;
  }

  VerifyContentType();

  nsIntRect visibleRect = mVisibleRegion.GetBounds();

  if (!mTexture) {
    CreateNewTexture(gfxIntSize(visibleRect.width, visibleRect.height));
    mValidRegion.SetEmpty();
  }

  if (!mValidRegion.IsEqual(mVisibleRegion)) {
    





    nsIntRegion region;
    region.Sub(mVisibleRegion, mValidRegion);

    DrawRegion(region);

    mValidRegion = mVisibleRegion;
  }
}

void
ThebesLayerD3D10::LayerManagerDestroyed()
{
  mD3DManager = nsnull;
}

Layer*
ThebesLayerD3D10::GetLayer()
{
  return this;
}

void
ThebesLayerD3D10::VerifyContentType()
{
  if (mD2DSurface) {
    gfxASurface::gfxContentType type = CanUseOpaqueSurface() ?
      gfxASurface::CONTENT_COLOR : gfxASurface::CONTENT_COLOR_ALPHA;

    if (type != mD2DSurface->GetContentType()) {
      mD2DSurface = new gfxD2DSurface(mTexture, type);

      if (!mD2DSurface || mD2DSurface->CairoStatus()) {
        NS_WARNING("Failed to create surface for ThebesLayerD3D10.");
        mD2DSurface = nsnull;
        return;
      }
      mValidRegion.SetEmpty();
    }
  }
}

void
ThebesLayerD3D10::DrawRegion(const nsIntRegion &aRegion)
{
  HRESULT hr;
  nsIntRect visibleRect = mVisibleRegion.GetBounds();

  if (!mD2DSurface) {
    return;
  }

  nsRefPtr<gfxContext> context = new gfxContext(mD2DSurface);

  nsIntRegionRectIterator iter(aRegion);
  context->Translate(gfxPoint(-visibleRect.x, -visibleRect.y));
  context->NewPath();
  const nsIntRect *iterRect;
  while ((iterRect = iter.Next())) {
    context->Rectangle(gfxRect(iterRect->x, iterRect->y, iterRect->width, iterRect->height));      
  }
  context->Clip();

  if (mD2DSurface->GetContentType() != gfxASurface::CONTENT_COLOR) {
    context->SetOperator(gfxContext::OPERATOR_CLEAR);
    context->Paint();
    context->SetOperator(gfxContext::OPERATOR_OVER);
  }

  LayerManagerD3D10::CallbackInfo cbInfo = mD3DManager->GetCallbackInfo();
  cbInfo.Callback(this, context, aRegion, nsIntRegion(), cbInfo.CallbackData);
}

void
ThebesLayerD3D10::CreateNewTexture(const gfxIntSize &aSize)
{
  if (aSize.width == 0 || aSize.height == 0) {
    
    return;
  }

  CD3D10_TEXTURE2D_DESC desc(DXGI_FORMAT_B8G8R8A8_UNORM, aSize.width, aSize.height, 1, 1);
  desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
  desc.MiscFlags = D3D10_RESOURCE_MISC_GDI_COMPATIBLE;

  HRESULT hr = device()->CreateTexture2D(&desc, NULL, getter_AddRefs(mTexture));

  if (FAILED(hr)) {
    NS_WARNING("Failed to create new texture for ThebesLayerD3D10!");
    return;
  }

  hr = device()->CreateShaderResourceView(mTexture, NULL, getter_AddRefs(mSRView));

  if (FAILED(hr)) {
    NS_WARNING("Failed to create shader resource view for ThebesLayerD3D10.");
  }

  mD2DSurface = new gfxD2DSurface(mTexture, CanUseOpaqueSurface() ?
    gfxASurface::CONTENT_COLOR : gfxASurface::CONTENT_COLOR_ALPHA);

  if (!mD2DSurface || mD2DSurface->CairoStatus()) {
    NS_WARNING("Failed to create surface for ThebesLayerD3D10.");
    mD2DSurface = nsnull;
    return;
  }
}

} 
} 
