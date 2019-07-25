






































#include "mozilla/layers/PLayers.h"
#include "mozilla/layers/ShadowLayers.h"
#include "ShadowBufferD3D9.h"

#include "gfxImageSurface.h"
#include "gfxWindowsSurface.h"
#include "gfxWindowsPlatform.h"

#include "CanvasLayerD3D9.h"

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

  CreateTexture();
}

void
CanvasLayerD3D9::UpdateSurface()
{
  if (!mDirty)
    return;
  mDirty = PR_FALSE;

  if (!mTexture) {
    CreateTexture();
    NS_WARNING("CanvasLayerD3D9::Updated called but no texture present!");
    return;
  }

  if (mGLContext) {
    
    LockTextureRectD3D9 textureLock(mTexture);
    if (!textureLock.HasLock()) {
      NS_WARNING("Failed to lock CanvasLayer texture.");
      return;
    }

    D3DLOCKED_RECT r = textureLock.GetLockRect();

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
  } else if (mSurface) {
    RECT r;
    r.left = mBounds.x;
    r.top = mBounds.y;
    r.right = mBounds.XMost();
    r.bottom = mBounds.YMost();

    LockTextureRectD3D9 textureLock(mTexture);
    if (!textureLock.HasLock()) {
      NS_WARNING("Failed to lock CanvasLayer texture.");
      return;
    }

    D3DLOCKED_RECT lockedRect = textureLock.GetLockRect();

    nsRefPtr<gfxImageSurface> sourceSurface;

    if (mSurface->GetType() == gfxASurface::SurfaceTypeWin32) {
      sourceSurface = mSurface->GetAsImageSurface();
    } else if (mSurface->GetType() == gfxASurface::SurfaceTypeImage) {
      sourceSurface = static_cast<gfxImageSurface*>(mSurface.get());
      if (sourceSurface->Format() != gfxASurface::ImageFormatARGB32 &&
          sourceSurface->Format() != gfxASurface::ImageFormatRGB24)
      {
        return;
      }
    } else {
      sourceSurface = new gfxImageSurface(gfxIntSize(mBounds.width, mBounds.height),
                                          gfxASurface::ImageFormatARGB32);
      nsRefPtr<gfxContext> ctx = new gfxContext(sourceSurface);
      ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
      ctx->SetSource(mSurface);
      ctx->Paint();
    }

    PRUint8 *startBits = sourceSurface->Data();
    PRUint32 sourceStride = sourceSurface->Stride();

    if (sourceSurface->Format() != gfxASurface::ImageFormatARGB32) {
      mHasAlpha = false;
    } else {
      mHasAlpha = true;
    }

    for (int y = 0; y < mBounds.height; y++) {
      memcpy((PRUint8*)lockedRect.pBits + lockedRect.Pitch * y,
             startBits + sourceStride * y,
             mBounds.width * 4);
    }

  }
}

Layer*
CanvasLayerD3D9::GetLayer()
{
  return this;
}

void
CanvasLayerD3D9::RenderLayer()
{
  UpdateSurface();
  FireDidTransactionCallback();

  if (!mTexture)
    return;

  




  ShaderConstantRect quad(0, 0, mBounds.width, mBounds.height);
  if (mNeedsYFlip) {
    quad.mHeight = (float)-mBounds.height;
    quad.mY = (float)mBounds.height;
  }

  device()->SetVertexShaderConstantF(CBvLayerQuad, quad, 1);

  SetShaderTransformAndOpacity();

  if (mHasAlpha) {
    mD3DManager->SetShaderMode(DeviceManagerD3D9::RGBALAYER);
  } else {
    mD3DManager->SetShaderMode(DeviceManagerD3D9::RGBLAYER);
  }

  if (mFilter == gfxPattern::FILTER_NEAREST) {
    device()->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    device()->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
  }
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
  if (mFilter == gfxPattern::FILTER_NEAREST) {
    device()->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    device()->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
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

ShadowCanvasLayerD3D9::ShadowCanvasLayerD3D9(LayerManagerD3D9* aManager)
  : ShadowCanvasLayer(aManager, nsnull)
  , LayerD3D9(aManager)
  , mNeedsYFlip(PR_FALSE)
{
  mImplData = static_cast<LayerD3D9*>(this);
}
 
ShadowCanvasLayerD3D9::~ShadowCanvasLayerD3D9()
{}

void
ShadowCanvasLayerD3D9::Initialize(const Data& aData)
{
  NS_RUNTIMEABORT("Non-shadow layer API unexpectedly used for shadow layer");
}

void
ShadowCanvasLayerD3D9::Init(bool needYFlip)
{
  if (!mBuffer) {
    mBuffer = new ShadowBufferD3D9(this);
  }

  mNeedsYFlip = needYFlip;
}

void
ShadowCanvasLayerD3D9::Swap(const CanvasSurface& aNewFront,
                            bool needYFlip,
                            CanvasSurface* aNewBack)
{
  NS_ASSERTION(aNewFront.type() == CanvasSurface::TSurfaceDescriptor, 
    "ShadowCanvasLayerD3D9::Swap expected CanvasSurface surface");

  nsRefPtr<gfxASurface> surf = 
    ShadowLayerForwarder::OpenDescriptor(aNewFront);
  if (!mBuffer) {
    Init(needYFlip);
  }
  mBuffer->Upload(surf, GetVisibleRegion().GetBounds());

  *aNewBack = aNewFront;
}

void
ShadowCanvasLayerD3D9::DestroyFrontBuffer()
{
  Destroy();
}

void
ShadowCanvasLayerD3D9::Disconnect()
{
  Destroy();
}

void
ShadowCanvasLayerD3D9::Destroy()
{
  mBuffer = nsnull;
}

void
ShadowCanvasLayerD3D9::CleanResources()
{
  Destroy();
}

void
ShadowCanvasLayerD3D9::LayerManagerDestroyed()
{
  mD3DManager->deviceManager()->mLayersWithResources.RemoveElement(this);
  mD3DManager = nsnull;
}

Layer*
ShadowCanvasLayerD3D9::GetLayer()
{
  return this;
}

void
ShadowCanvasLayerD3D9::RenderLayer()
{
  if (!mBuffer) {
    return;
  }

  mBuffer->RenderTo(mD3DManager, GetEffectiveVisibleRegion());
}


} 
} 
