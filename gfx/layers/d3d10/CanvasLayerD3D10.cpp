





































#include "CanvasLayerD3D10.h"

#include "gfxImageSurface.h"
#include "gfxWindowsSurface.h"
#include "gfxWindowsPlatform.h"

namespace mozilla {
namespace layers {

CanvasLayerD3D10::~CanvasLayerD3D10()
{
}

void
CanvasLayerD3D10::Initialize(const Data& aData)
{
  NS_ASSERTION(mSurface == nsnull, "BasicCanvasLayer::Initialize called twice!");

  if (aData.mSurface) {
    mSurface = aData.mSurface;
    NS_ASSERTION(aData.mGLContext == nsnull,
                 "CanvasLayer can't have both surface and GLContext");
    mNeedsYFlip = PR_FALSE;
    mDataIsPremultiplied = PR_TRUE;
  } else if (aData.mGLContext) {
    NS_ASSERTION(aData.mGLContext->IsOffscreen(), "canvas gl context isn't offscreen");
    mGLContext = aData.mGLContext;
    mCanvasFramebuffer = mGLContext->GetOffscreenFBO();
    mDataIsPremultiplied = aData.mGLBufferIsPremultiplied;
    mNeedsYFlip = PR_TRUE;
  } else {
    NS_ERROR("CanvasLayer created without mSurface or mGLContext?");
  }

  mBounds.SetRect(0, 0, aData.mSize.width, aData.mSize.height);

  if (mSurface && mSurface->GetType() == gfxASurface::SurfaceTypeD2D) {
    void *data = mSurface->GetData(&gKeyD3D10Texture);
    if (data) {
      mTexture = static_cast<ID3D10Texture2D*>(data);
      mIsD2DTexture = true;
      device()->CreateShaderResourceView(mTexture, NULL, getter_AddRefs(mSRView));
      return;
    }
  }

  mIsD2DTexture = false;

  CD3D10_TEXTURE2D_DESC desc(DXGI_FORMAT_B8G8R8A8_UNORM, mBounds.width, mBounds.height, 1, 1);
  desc.Usage = D3D10_USAGE_DYNAMIC;
  desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;

  HRESULT hr = device()->CreateTexture2D(&desc, NULL, getter_AddRefs(mTexture));
  if (FAILED(hr)) {
    NS_WARNING("Failed to create texture for CanvasLayer!");
    return;
  }
  device()->CreateShaderResourceView(mTexture, NULL, getter_AddRefs(mSRView));
}

void
CanvasLayerD3D10::Updated(const nsIntRect& aRect)
{
  if (mIsD2DTexture) {
    mSurface->Flush();
    return;
  }

  if (mGLContext) {
    
    D3D10_MAPPED_TEXTURE2D map;
    
    HRESULT hr = mTexture->Map(0, D3D10_MAP_WRITE_DISCARD, 0, &map);

    if (FAILED(hr)) {
      NS_WARNING("Failed to map CanvasLayer texture.");
      return;
    }

    PRUint8 *destination;
    if (map.RowPitch != mBounds.width * 4) {
      destination = new PRUint8[mBounds.width * mBounds.height * 4];
    } else {
      destination = (PRUint8*)map.pData;
    }

    
    
    mGLContext->fFlush();

    PRUint32 currentFramebuffer = 0;

    mGLContext->fGetIntegerv(LOCAL_GL_FRAMEBUFFER_BINDING, (GLint*)&currentFramebuffer);

    
    
    if (currentFramebuffer != mCanvasFramebuffer)
      mGLContext->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, mCanvasFramebuffer);

    
    
    
    nsRefPtr<gfxImageSurface> tmpSurface =
      new gfxImageSurface(destination,
                          gfxIntSize(mBounds.width, mBounds.height),
                          mBounds.width * 4,
                          gfxASurface::ImageFormatARGB32);
    mGLContext->ReadPixelsIntoImageSurface(0, 0,
                                           mBounds.width, mBounds.height,
                                           tmpSurface);
    tmpSurface = nsnull;

    
    if (currentFramebuffer != mCanvasFramebuffer)
      mGLContext->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, currentFramebuffer);

    if (map.RowPitch != mBounds.width * 4) {
      for (int y = 0; y < mBounds.height; y++) {
        memcpy((PRUint8*)map.pData + map.RowPitch * y,
               destination + mBounds.width * 4 * y,
               mBounds.width * 4);
      }
      delete [] destination;
    }
    mTexture->Unmap(0);
  } else if (mSurface) {
    RECT r;
    r.left = aRect.x;
    r.top = aRect.y;
    r.right = aRect.XMost();
    r.bottom = aRect.YMost();

    D3D10_MAPPED_TEXTURE2D map;
    HRESULT hr = mTexture->Map(0, D3D10_MAP_WRITE_DISCARD, 0, &map);

    if (FAILED(hr)) {
      NS_WARNING("Failed to lock CanvasLayer texture.");
      return;
    }

    PRUint8 *startBits;
    PRUint32 sourceStride;

    nsRefPtr<gfxImageSurface> dstSurface;

    dstSurface = new gfxImageSurface((unsigned char*)map.pData,
                                     gfxIntSize(aRect.width, aRect.height),
                                     map.RowPitch,
                                     gfxASurface::ImageFormatARGB32);
    nsRefPtr<gfxContext> ctx = new gfxContext(dstSurface);
    ctx->Translate(gfxPoint(-aRect.x, -aRect.y));
    ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
    ctx->SetSource(mSurface);
    ctx->Paint();
    
    mTexture->Unmap(0);
  }
}

Layer*
CanvasLayerD3D10::GetLayer()
{
  return this;
}

void
CanvasLayerD3D10::RenderLayer(float aOpacity, const gfx3DMatrix &aTransform)
{
  if (!mTexture) {
    return;
  }

  nsIntRect visibleRect = mVisibleRegion.GetBounds();

  gfx3DMatrix transform = mTransform * aTransform;
  effect()->GetVariableByName("mLayerTransform")->SetRawValue(&transform._11, 0, 64);
  effect()->GetVariableByName("fLayerOpacity")->AsScalar()->SetFloat(GetOpacity() * aOpacity);

  ID3D10EffectTechnique *technique;

  if (mDataIsPremultiplied) {
    if (mSurface && mSurface->GetContentType() == gfxASurface::CONTENT_COLOR) {
      if (mFilter == gfxPattern::FILTER_NEAREST) {
        technique = effect()->GetTechniqueByName("RenderRGBLayerPremulPoint");
      } else {
        technique = effect()->GetTechniqueByName("RenderRGBLayerPremul");
      }
    } else {
      if (mFilter == gfxPattern::FILTER_NEAREST) {
        technique = effect()->GetTechniqueByName("RenderRGBALayerPremulPoint");
      } else {
        technique = effect()->GetTechniqueByName("RenderRGBALayerPremul");
      }
    }
  } else {
    if (mFilter == gfxPattern::FILTER_NEAREST) {
      technique = effect()->GetTechniqueByName("RenderRGBALayerNonPremulPoint");
    } else {
      technique = effect()->GetTechniqueByName("RenderRGBALayerNonPremul");
    }
  }

  if (mSRView) {
    effect()->GetVariableByName("tRGB")->AsShaderResource()->SetResource(mSRView);
  }

  effect()->GetVariableByName("vLayerQuad")->AsVector()->SetFloatVector(
    ShaderConstantRectD3D10(
      (float)mBounds.x,
      (float)mBounds.y,
      (float)mBounds.width,
      (float)mBounds.height)
    );

  if (mNeedsYFlip) {
    effect()->GetVariableByName("vTextureCoords")->AsVector()->SetFloatVector(
      ShaderConstantRectD3D10(
        0,
        1.0f,
        1.0f,
        -1.0f)
      );
  }

  technique->GetPassByIndex(0)->Apply(0);
  device()->Draw(4, 0);

  if (mNeedsYFlip) {
    effect()->GetVariableByName("vTextureCoords")->AsVector()->
      SetFloatVector(ShaderConstantRectD3D10(0, 0, 1.0f, 1.0f));
  }
}

} 
} 
