





































#include "CanvasLayerD3D9.h"

#include "gfxImageSurface.h"
#include "gfxWindowsSurface.h"
#include "gfxWindowsPlatform.h"

namespace mozilla {
namespace layers {

CanvasLayerD3D9::~CanvasLayerD3D9()
{
  if (mD3DManager) {
    mD3DManager->deviceManager()->mLayersWithResources.RemoveElement(this);
  }
}

void
CanvasLayerD3D9::Initialize(const Data& aData)
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
    void *data = mSurface->GetData(&gKeyD3D9Texture);
    if (data) {
      mTexture = static_cast<IDirect3DTexture9*>(data);
      mIsInteropTexture = true;
      return;
    }
  }

  mIsInteropTexture = false;

  CreateTexture();
}

void
CanvasLayerD3D9::Updated(const nsIntRect& aRect)
{
  if (!mTexture) {
    CreateTexture();
    NS_WARNING("CanvasLayerD3D9::Updated called but no texture present!");
    return;
  }

#ifdef CAIRO_HAS_D2D_SURFACE
  if (mIsInteropTexture) {
    mSurface->Flush();
    cairo_d2d_finish_device(gfxWindowsPlatform::GetPlatform()->GetD2DDevice());
    return;
  }
#endif

  if (mGLContext) {
    
    D3DLOCKED_RECT r;
    HRESULT hr = mTexture->LockRect(0, &r, NULL, 0);

    if (FAILED(hr)) {
      NS_WARNING("Failed to lock CanvasLayer texture.");
      return;
    }

    PRUint8 *destination;
    if (r.Pitch != mBounds.width * 4) {
      destination = new PRUint8[mBounds.width * mBounds.height * 4];
    } else {
      destination = (PRUint8*)r.pBits;
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

    if (r.Pitch != mBounds.width * 4) {
      for (int y = 0; y < mBounds.height; y++) {
        memcpy((PRUint8*)r.pBits + r.Pitch * y,
               destination + mBounds.width * 4 * y,
               mBounds.width * 4);
      }
      delete [] destination;
    }
    mTexture->UnlockRect(0);
  } else if (mSurface) {
    RECT r;
    r.left = aRect.x;
    r.top = aRect.y;
    r.right = aRect.XMost();
    r.bottom = aRect.YMost();

    D3DLOCKED_RECT lockedRect;
    HRESULT hr = mTexture->LockRect(0, &lockedRect, &r, 0);

    if (FAILED(hr)) {
      NS_WARNING("Failed to lock CanvasLayer texture.");
      return;
    }

    PRUint8 *startBits;
    PRUint32 sourceStride;

    nsRefPtr<gfxImageSurface> sourceSurface;

    if (mSurface->GetType() == gfxASurface::SurfaceTypeWin32) {
      sourceSurface = static_cast<gfxWindowsSurface*>(mSurface.get())->GetImageSurface();
      startBits = sourceSurface->Data() + sourceSurface->Stride() * aRect.y +
                  aRect.x * 4;
      sourceStride = sourceSurface->Stride();
    } else if (mSurface->GetType() == gfxASurface::SurfaceTypeImage) {
      sourceSurface = static_cast<gfxImageSurface*>(mSurface.get());
      if (sourceSurface->Format() != gfxASurface::ImageFormatARGB32 &&
          sourceSurface->Format() != gfxASurface::ImageFormatRGB24)
      {
        mTexture->UnlockRect(0);
        return;
      }
      startBits = sourceSurface->Data() + sourceSurface->Stride() * aRect.y +
                  aRect.x * 4;
      sourceStride = sourceSurface->Stride();
    } else {
      sourceSurface = new gfxImageSurface(gfxIntSize(aRect.width, aRect.height),
                                          gfxASurface::ImageFormatARGB32);
      nsRefPtr<gfxContext> ctx = new gfxContext(sourceSurface);
      ctx->Translate(gfxPoint(-aRect.x, -aRect.y));
      ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
      ctx->SetSource(mSurface);
      ctx->Paint();
      startBits = sourceSurface->Data();
      sourceStride = sourceSurface->Stride();
    }

    for (int y = 0; y < aRect.height; y++) {
      memcpy((PRUint8*)lockedRect.pBits + lockedRect.Pitch * y,
             startBits + sourceStride * y,
             aRect.width * 4);
    }

    mTexture->UnlockRect(0);
  }
}

Layer*
CanvasLayerD3D9::GetLayer()
{
  return this;
}

void
CanvasLayerD3D9::RenderLayer(float aOpacity, const gfx3DMatrix &aTransform)
{
  if (!mTexture) {
    Updated(mBounds);
  }

  




  ShaderConstantRect quad(0, 0, mBounds.width, mBounds.height);
  if (mNeedsYFlip) {
    quad.mHeight = (float)-mBounds.height;
    quad.mY = (float)mBounds.height;
  }

  device()->SetVertexShaderConstantF(CBvLayerQuad, quad, 1);

  gfx3DMatrix transform = mTransform * aTransform;
  device()->SetVertexShaderConstantF(CBmLayerTransform, &transform._11, 4);

  float opacity[4];
  



  opacity[0] = GetOpacity();
  device()->SetPixelShaderConstantF(CBfLayerOpacity, opacity, 1);

  mD3DManager->SetShaderMode(DeviceManagerD3D9::RGBALAYER);

  if (!mDataIsPremultiplied) {
    device()->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device()->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
  }
  device()->SetTexture(0, mTexture);
  device()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
  if (!mDataIsPremultiplied) {
    device()->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
    device()->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
  }
}

void
CanvasLayerD3D9::CleanResources()
{
  if (mD3DManager->deviceManager()->HasDynamicTextures()) {
    
    mTexture = nsnull;
  }
}

void
CanvasLayerD3D9::LayerManagerDestroyed()
{
  mD3DManager->deviceManager()->mLayersWithResources.RemoveElement(this);
  mD3DManager = nsnull;
}

void
CanvasLayerD3D9::CreateTexture()
{
  if (mD3DManager->deviceManager()->HasDynamicTextures()) {
    device()->CreateTexture(mBounds.width, mBounds.height, 1, D3DUSAGE_DYNAMIC,
                            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
                            getter_AddRefs(mTexture), NULL);    
  } else {
    
    
    device()->CreateTexture(mBounds.width, mBounds.height, 1, 0,
                            D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                            getter_AddRefs(mTexture), NULL);
  }
}

} 
} 
