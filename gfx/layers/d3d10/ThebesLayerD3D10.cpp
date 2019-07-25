




































#include "ThebesLayerD3D10.h"
#include "gfxPlatform.h"

#include "gfxWindowsPlatform.h"
#ifdef CAIRO_HAS_D2D_SURFACE
#include "gfxD2DSurface.h"
#endif

#include "gfxTeeSurface.h"
#include "gfxUtils.h"
#include "ReadbackLayer.h"
#include "ReadbackProcessor.h"

namespace mozilla {
namespace layers {

ThebesLayerD3D10::ThebesLayerD3D10(LayerManagerD3D10 *aManager)
  : ThebesLayer(aManager, NULL)
  , LayerD3D10(aManager)
  , mCurrentSurfaceMode(SURFACE_OPAQUE)
{
  mImplData = static_cast<LayerD3D10*>(this);
}

ThebesLayerD3D10::~ThebesLayerD3D10()
{
}






#define RETENTION_THRESHOLD 16384

void

ThebesLayerD3D10::InvalidateRegion(const nsIntRegion &aRegion)
{
  mValidRegion.Sub(mValidRegion, aRegion);
}

void ThebesLayerD3D10::CopyRegion(ID3D10Texture2D* aSrc, const nsIntPoint &aSrcOffset,
                                  ID3D10Texture2D* aDest, const nsIntPoint &aDestOffset,
                                  const nsIntRegion &aCopyRegion, nsIntRegion* aValidRegion)
{
  nsIntRegion retainedRegion;
  nsIntRegionRectIterator iter(aCopyRegion);
  const nsIntRect *r;
  while ((r = iter.Next())) {
    if (r->width * r->height > RETENTION_THRESHOLD) {
      
      
      D3D10_BOX box;
      box.left = r->x - aSrcOffset.x;
      box.top = r->y - aSrcOffset.y;
      box.right = box.left + r->width;
      box.bottom = box.top + r->height;
      box.back = 1;
      box.front = 0;

      device()->CopySubresourceRegion(aDest, 0,
                                      r->x - aDestOffset.x,
                                      r->y - aDestOffset.y,
                                      0,
                                      aSrc, 0,
                                      &box);

      retainedRegion.Or(retainedRegion, *r);
    }
  }

  
  aValidRegion->And(*aValidRegion, retainedRegion);  
}

void
ThebesLayerD3D10::RenderLayer()
{
  if (!mTexture) {
    return;
  }

  SetEffectTransformAndOpacity();

  ID3D10EffectTechnique *technique;
  switch (mCurrentSurfaceMode) {
  case SURFACE_COMPONENT_ALPHA:
    technique = effect()->GetTechniqueByName("RenderComponentAlphaLayer");
    break;
  case SURFACE_OPAQUE:
    technique = effect()->GetTechniqueByName("RenderRGBLayerPremul");
    break;
  case SURFACE_SINGLE_CHANNEL_ALPHA:
    technique = effect()->GetTechniqueByName("RenderRGBALayerPremul");
    break;
  default:
    NS_ERROR("Unknown mode");
    return;
  }

  nsIntRegionRectIterator iter(mVisibleRegion);

  const nsIntRect *iterRect;
  if (mSRView) {
    effect()->GetVariableByName("tRGB")->AsShaderResource()->SetResource(mSRView);
  }
  if (mSRViewOnWhite) {
    effect()->GetVariableByName("tRGBWhite")->AsShaderResource()->SetResource(mSRViewOnWhite);
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
        (float)(iterRect->x - mTextureRect.x) / (float)mTextureRect.width,
        (float)(iterRect->y - mTextureRect.y) / (float)mTextureRect.height,
        (float)iterRect->width / (float)mTextureRect.width,
        (float)iterRect->height / (float)mTextureRect.height)
      );

    technique->GetPassByIndex(0)->Apply(0);
    device()->Draw(4, 0);
  }

  
  effect()->GetVariableByName("vTextureCoords")->AsVector()->
    SetFloatVector(ShaderConstantRectD3D10(0, 0, 1.0f, 1.0f));
}

void
ThebesLayerD3D10::Validate(ReadbackProcessor *aReadback)
{
  if (mVisibleRegion.IsEmpty()) {
    return;
  }

  nsIntRect newTextureRect = mVisibleRegion.GetBounds();

  SurfaceMode mode = GetSurfaceMode();
  if (mode == SURFACE_COMPONENT_ALPHA &&
      (!mParent || !mParent->SupportsComponentAlphaChildren())) {
    mode = SURFACE_SINGLE_CHANNEL_ALPHA;
  }
  
  
  
  
  
  nsIntRegion neededRegion = mVisibleRegion;
  if (!neededRegion.GetBounds().IsEqualInterior(newTextureRect) ||
      neededRegion.GetNumRects() > 1) {
    gfxMatrix transform2d;
    if (!GetEffectiveTransform().Is2D(&transform2d) ||
        transform2d.HasNonIntegerTranslation()) {
      neededRegion = newTextureRect;
      if (mode == SURFACE_OPAQUE) {
        
        
        
        mode = SURFACE_SINGLE_CHANNEL_ALPHA;
      }
    }
  }
  mCurrentSurfaceMode = mode;

  VerifyContentType(mode);

  nsTArray<ReadbackProcessor::Update> readbackUpdates;
  nsIntRegion readbackRegion;
  if (aReadback && UsedForReadback()) {
    aReadback->GetThebesLayerUpdates(this, &readbackUpdates, &readbackRegion);
  }

  if (mTexture) {
    if (!mTextureRect.IsEqualInterior(newTextureRect)) {
      nsRefPtr<ID3D10Texture2D> oldTexture = mTexture;
      mTexture = nsnull;
      nsRefPtr<ID3D10Texture2D> oldTextureOnWhite = mTextureOnWhite;
      mTextureOnWhite = nsnull;

      nsIntRegion retainRegion = mTextureRect;
      
      
      retainRegion.And(retainRegion, mVisibleRegion);
      
      retainRegion.And(retainRegion, mValidRegion);

      CreateNewTextures(gfxIntSize(newTextureRect.width, newTextureRect.height), mode);

      nsIntRect largeRect = retainRegion.GetLargestRectangle();

      
      
      
      
      
      
      if (!oldTexture || !mTexture ||
          largeRect.width * largeRect.height < RETENTION_THRESHOLD) {
        mValidRegion.SetEmpty();
      } else {
        CopyRegion(oldTexture, mTextureRect.TopLeft(),
                   mTexture, newTextureRect.TopLeft(),
                   retainRegion, &mValidRegion);
        if (oldTextureOnWhite) {
          CopyRegion(oldTextureOnWhite, mTextureRect.TopLeft(),
                     mTextureOnWhite, newTextureRect.TopLeft(),
                     retainRegion, &mValidRegion);
        }
      }
    }
  }
  mTextureRect = newTextureRect;

  if (!mTexture || (mode == SURFACE_COMPONENT_ALPHA && !mTextureOnWhite)) {
    CreateNewTextures(gfxIntSize(newTextureRect.width, newTextureRect.height), mode);
    mValidRegion.SetEmpty();
  }

  nsIntRegion drawRegion;
  drawRegion.Sub(neededRegion, mValidRegion);

  if (!drawRegion.IsEmpty()) {
    LayerManagerD3D10::CallbackInfo cbInfo = mD3DManager->GetCallbackInfo();
    if (!cbInfo.Callback) {
      NS_ERROR("D3D10 should never need to update ThebesLayers in an empty transaction");
      return;
    }

    DrawRegion(drawRegion, mode);

    if (readbackUpdates.Length() > 0) {
      CD3D10_TEXTURE2D_DESC desc(DXGI_FORMAT_B8G8R8A8_UNORM,
                                 newTextureRect.width, newTextureRect.height,
                                 1, 1, 0, D3D10_USAGE_STAGING,
                                 D3D10_CPU_ACCESS_READ);

      nsRefPtr<ID3D10Texture2D> readbackTexture;
      device()->CreateTexture2D(&desc, NULL, getter_AddRefs(readbackTexture));
      device()->CopyResource(readbackTexture, mTexture);

      for (int i = 0; i < readbackUpdates.Length(); i++) {
        mD3DManager->readbackManager()->PostTask(readbackTexture,
                                                 &readbackUpdates[i],
                                                 gfxPoint(newTextureRect.x, newTextureRect.y));
      }
    }

    mValidRegion = neededRegion;
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
ThebesLayerD3D10::VerifyContentType(SurfaceMode aMode)
{
  if (mD2DSurface) {
    gfxASurface::gfxContentType type = aMode != SURFACE_SINGLE_CHANNEL_ALPHA ?
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
        
    if (aMode != SURFACE_COMPONENT_ALPHA && mTextureOnWhite) {
      
      mD2DSurfaceOnWhite = nsnull;
      mSRViewOnWhite = nsnull;
      mTextureOnWhite = nsnull;
      mValidRegion.SetEmpty();
    }
  }
}

static void
FillSurface(gfxASurface* aSurface, const nsIntRegion& aRegion,
            const nsIntPoint& aOffset, const gfxRGBA& aColor)
{
  nsRefPtr<gfxContext> ctx = new gfxContext(aSurface);
  ctx->Translate(-gfxPoint(aOffset.x, aOffset.y));
  gfxUtils::PathFromRegion(ctx, aRegion);
  ctx->SetColor(aColor);
  ctx->Fill();
}

void
ThebesLayerD3D10::DrawRegion(nsIntRegion &aRegion, SurfaceMode aMode)
{
  nsIntRect visibleRect = mVisibleRegion.GetBounds();

  if (!mD2DSurface) {
    return;
  }

  nsRefPtr<gfxASurface> destinationSurface;
  
  if (aMode == SURFACE_COMPONENT_ALPHA) {
    FillSurface(mD2DSurface, aRegion, visibleRect.TopLeft(), gfxRGBA(0.0, 0.0, 0.0, 1.0));
    FillSurface(mD2DSurfaceOnWhite, aRegion, visibleRect.TopLeft(), gfxRGBA(1.0, 1.0, 1.0, 1.0));
    gfxASurface* surfaces[2] = { mD2DSurface.get(), mD2DSurfaceOnWhite.get() };
    destinationSurface = new gfxTeeSurface(surfaces, NS_ARRAY_LENGTH(surfaces));
    
    
    
    destinationSurface->SetAllowUseAsSource(PR_FALSE);
  } else {
    destinationSurface = mD2DSurface;
  }

  nsRefPtr<gfxContext> context = new gfxContext(destinationSurface);

  nsIntRegionRectIterator iter(aRegion);
  context->Translate(gfxPoint(-visibleRect.x, -visibleRect.y));
  context->NewPath();
  const nsIntRect *iterRect;
  while ((iterRect = iter.Next())) {
    context->Rectangle(gfxRect(iterRect->x, iterRect->y, iterRect->width, iterRect->height));      
  }
  context->Clip();

  if (aMode == SURFACE_SINGLE_CHANNEL_ALPHA) {
    context->SetOperator(gfxContext::OPERATOR_CLEAR);
    context->Paint();
    context->SetOperator(gfxContext::OPERATOR_OVER);
  }

  mD2DSurface->SetSubpixelAntialiasingEnabled(!(mContentFlags & CONTENT_COMPONENT_ALPHA));

  LayerManagerD3D10::CallbackInfo cbInfo = mD3DManager->GetCallbackInfo();
  cbInfo.Callback(this, context, aRegion, nsIntRegion(), cbInfo.CallbackData);
}

void
ThebesLayerD3D10::CreateNewTextures(const gfxIntSize &aSize, SurfaceMode aMode)
{
  if (aSize.width == 0 || aSize.height == 0) {
    
    return;
  }

  CD3D10_TEXTURE2D_DESC desc(DXGI_FORMAT_B8G8R8A8_UNORM, aSize.width, aSize.height, 1, 1);
  desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
  desc.MiscFlags = D3D10_RESOURCE_MISC_GDI_COMPATIBLE;
  HRESULT hr;

  if (!mTexture) {
    hr = device()->CreateTexture2D(&desc, NULL, getter_AddRefs(mTexture));

    if (FAILED(hr)) {
      NS_WARNING("Failed to create new texture for ThebesLayerD3D10!");
      return;
    }

    hr = device()->CreateShaderResourceView(mTexture, NULL, getter_AddRefs(mSRView));

    if (FAILED(hr)) {
      NS_WARNING("Failed to create shader resource view for ThebesLayerD3D10.");
    }

    mD2DSurface = new gfxD2DSurface(mTexture, aMode != SURFACE_SINGLE_CHANNEL_ALPHA ?
      gfxASurface::CONTENT_COLOR : gfxASurface::CONTENT_COLOR_ALPHA);

    if (!mD2DSurface || mD2DSurface->CairoStatus()) {
      NS_WARNING("Failed to create surface for ThebesLayerD3D10.");
      mD2DSurface = nsnull;
      return;
    }

  }

  if (aMode == SURFACE_COMPONENT_ALPHA && !mTextureOnWhite) {
    hr = device()->CreateTexture2D(&desc, NULL, getter_AddRefs(mTextureOnWhite));

    if (FAILED(hr)) {
      NS_WARNING("Failed to create new texture for ThebesLayerD3D10!");
      return;
    }

    hr = device()->CreateShaderResourceView(mTextureOnWhite, NULL, getter_AddRefs(mSRViewOnWhite));

    if (FAILED(hr)) {
      NS_WARNING("Failed to create shader resource view for ThebesLayerD3D10.");
    }

    mD2DSurfaceOnWhite = new gfxD2DSurface(mTextureOnWhite, gfxASurface::CONTENT_COLOR);

    if (!mD2DSurfaceOnWhite || mD2DSurfaceOnWhite->CairoStatus()) {
      NS_WARNING("Failed to create surface for ThebesLayerD3D10.");
      mD2DSurfaceOnWhite = nsnull;
      return;
    }
  }
}

} 
} 
