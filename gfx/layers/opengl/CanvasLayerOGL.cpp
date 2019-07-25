




































#include "gfxSharedImageSurface.h"

#include "CanvasLayerOGL.h"

#include "gfxImageSurface.h"
#include "gfxContext.h"
#include "GLContextProvider.h"

#ifdef XP_WIN
#include "gfxWindowsSurface.h"
#include "WGLLibrary.h"
#endif

#ifdef XP_MACOSX
#include <OpenGL/OpenGL.h>
#endif

using namespace mozilla;
using namespace mozilla::layers;
using namespace mozilla::gl;

void
CanvasLayerOGL::Destroy()
{
  if (!mDestroyed) {
    if (mTexture) {
      GLContext *cx = mOGLManager->glForResources();
      cx->MakeCurrent();
      cx->fDeleteTextures(1, &mTexture);
    }

    mDestroyed = PR_TRUE;
  }
}

void
CanvasLayerOGL::Initialize(const Data& aData)
{
  NS_ASSERTION(mCanvasSurface == nsnull, "BasicCanvasLayer::Initialize called twice!");

  if (aData.mGLContext != nsnull &&
      aData.mSurface != nsnull)
  {
    NS_WARNING("CanvasLayerOGL can't have both surface and GLContext");
    return;
  }

  if (aData.mSurface) {
    mCanvasSurface = aData.mSurface;
    mNeedsYFlip = PR_FALSE;
  } else if (aData.mGLContext) {
    if (!aData.mGLContext->IsOffscreen()) {
      NS_WARNING("CanvasLayerOGL with a non-offscreen GL context given");
      return;
    }

    mCanvasGLContext = aData.mGLContext;
    mGLBufferIsPremultiplied = aData.mGLBufferIsPremultiplied;

    mNeedsYFlip = PR_TRUE;
  } else {
    NS_WARNING("CanvasLayerOGL::Initialize called without surface or GL context!");
    return;
  }

  mBounds.SetRect(0, 0, aData.mSize.width, aData.mSize.height);
}

void
CanvasLayerOGL::MakeTexture()
{
  if (mTexture != 0)
    return;

  gl()->fGenTextures(1, &mTexture);

  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
  gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);

  gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, LOCAL_GL_LINEAR);
  gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, LOCAL_GL_LINEAR);
  gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S, LOCAL_GL_CLAMP_TO_EDGE);
  gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T, LOCAL_GL_CLAMP_TO_EDGE);
}

void
CanvasLayerOGL::Updated(const nsIntRect& aRect)
{
  if (mDestroyed) {
    return;
  }

  NS_ASSERTION(mUpdatedRect.IsEmpty(),
               "CanvasLayer::Updated called more than once during a transaction!");

  mOGLManager->MakeCurrent();

  mUpdatedRect.UnionRect(mUpdatedRect, aRect);

  if (mCanvasGLContext &&
      mCanvasGLContext->GetContextType() == gl()->GetContextType())
  {
    if (gl()->BindOffscreenNeedsTexture(mCanvasGLContext) &&
        mTexture == 0)
    {
      MakeTexture();
    }
  } else {
    PRBool newTexture = mTexture == 0;
    if (newTexture) {
      MakeTexture();
      mUpdatedRect = mBounds;
    } else {
      gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
      gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);
    }

    nsRefPtr<gfxImageSurface> updatedAreaImageSurface;
    if (mCanvasSurface) {
      nsRefPtr<gfxASurface> sourceSurface = mCanvasSurface;

#ifdef XP_WIN
      if (sourceSurface->GetType() == gfxASurface::SurfaceTypeWin32) {
        sourceSurface = static_cast<gfxWindowsSurface*>(sourceSurface.get())->GetImageSurface();
        if (!sourceSurface)
          sourceSurface = mCanvasSurface;
      }
#endif

#if 0
      
      
      if (mCanvasSurface->GetType() == gfxASurface::SurfaceTypeImage) {
        gfxImageSurface *s = static_cast<gfxImageSurface*>(mCanvasSurface.get());
        if (s->Format() == gfxASurface::ImageFormatARGB32 ||
            s->Format() == gfxASurface::ImageFormatRGB24)
        {
          updatedAreaImageSurface = ...;
        } else {
          NS_WARNING("surface with format that we can't handle");
          return;
        }
      } else
#endif
      {
        updatedAreaImageSurface =
          new gfxImageSurface(gfxIntSize(mUpdatedRect.width, mUpdatedRect.height),
                              gfxASurface::ImageFormatARGB32);
        nsRefPtr<gfxContext> ctx = new gfxContext(updatedAreaImageSurface);
        ctx->Translate(gfxPoint(-mUpdatedRect.x, -mUpdatedRect.y));
        ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
        ctx->SetSource(sourceSurface);
        ctx->Paint();
      }
    } else if (mCanvasGLContext) {
      updatedAreaImageSurface =
        new gfxImageSurface(gfxIntSize(mUpdatedRect.width, mUpdatedRect.height),
                            gfxASurface::ImageFormatARGB32);
      mCanvasGLContext->ReadPixelsIntoImageSurface(mUpdatedRect.x, mUpdatedRect.y,
                                                   mUpdatedRect.width,
                                                   mUpdatedRect.height,
                                                   updatedAreaImageSurface);
    }

    if (newTexture) {
      gl()->fTexImage2D(LOCAL_GL_TEXTURE_2D,
                        0,
                        LOCAL_GL_RGBA,
                        mUpdatedRect.width,
                        mUpdatedRect.height,
                        0,
                        LOCAL_GL_RGBA,
                        LOCAL_GL_UNSIGNED_BYTE,
                        updatedAreaImageSurface->Data());
    } else {
      gl()->fTexSubImage2D(LOCAL_GL_TEXTURE_2D,
                           0,
                           mUpdatedRect.x,
                           mUpdatedRect.y,
                           mUpdatedRect.width,
                           mUpdatedRect.height,
                           LOCAL_GL_RGBA,
                           LOCAL_GL_UNSIGNED_BYTE,
                           updatedAreaImageSurface->Data());
    }
  }

  
  NS_ASSERTION(mBounds.Contains(mUpdatedRect),
               "CanvasLayer: Updated rect bigger than bounds!");
}

void
CanvasLayerOGL::RenderLayer(int aPreviousDestination,
                            const nsIntPoint& aOffset)
{
  mOGLManager->MakeCurrent();

  
  
  

  ColorTextureLayerProgram *program = nsnull;

  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);

  if (mTexture) {
    gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);
  }

  bool useGLContext = mCanvasGLContext &&
    mCanvasGLContext->GetContextType() == gl()->GetContextType();

  if (useGLContext) {
    mCanvasGLContext->MakeCurrent();
    mCanvasGLContext->fFlush();

    gl()->MakeCurrent();
    gl()->BindTex2DOffscreen(mCanvasGLContext);
    DEBUG_GL_ERROR_CHECK(gl());
    program = mOGLManager->GetRGBALayerProgram();
  } else {
    program = mOGLManager->GetBGRALayerProgram();
  }

  ApplyFilter(mFilter);

  program->Activate();
  program->SetLayerQuadRect(mBounds);
  program->SetLayerTransform(GetEffectiveTransform());
  program->SetLayerOpacity(GetOpacity());
  program->SetRenderOffset(aOffset);
  program->SetTextureUnit(0);

  mOGLManager->BindAndDrawQuad(program, mNeedsYFlip ? true : false);

  DEBUG_GL_ERROR_CHECK(gl());

  if (useGLContext) {
    gl()->UnbindTex2DOffscreen(mCanvasGLContext);
  }

  mUpdatedRect.Empty();
}


#ifdef MOZ_IPC

ShadowCanvasLayerOGL::ShadowCanvasLayerOGL(LayerManagerOGL* aManager)
  : ShadowCanvasLayer(aManager, nsnull)
  , LayerOGL(aManager)
{
  mImplData = static_cast<LayerOGL*>(this);
}
 
ShadowCanvasLayerOGL::~ShadowCanvasLayerOGL()
{}

void
ShadowCanvasLayerOGL::Initialize(const Data& aData)
{
  mDeadweight = static_cast<gfxSharedImageSurface*>(aData.mSurface);
  gfxSize sz = mDeadweight->GetSize();
  mTexImage = gl()->CreateTextureImage(nsIntSize(sz.width, sz.height),
                                       mDeadweight->GetContentType(),
                                       LOCAL_GL_CLAMP_TO_EDGE);
}

already_AddRefed<gfxSharedImageSurface>
ShadowCanvasLayerOGL::Swap(gfxSharedImageSurface* aNewFront)
{
  if (!mDestroyed && mTexImage) {
    

    gfxSize sz = aNewFront->GetSize();
    nsIntRegion updateRegion(nsIntRect(0, 0, sz.width, sz.height));
    
    nsRefPtr<gfxContext> dest = mTexImage->BeginUpdate(updateRegion);

    dest->SetOperator(gfxContext::OPERATOR_SOURCE);
    dest->DrawSurface(aNewFront, aNewFront->GetSize());

    mTexImage->EndUpdate();
  }

  return aNewFront;
}

void
ShadowCanvasLayerOGL::DestroyFrontBuffer()
{
  mTexImage = nsnull;
  if (mDeadweight) {
    mOGLManager->DestroySharedSurface(mDeadweight, mAllocator);
    mDeadweight = nsnull;
  }
}

void
ShadowCanvasLayerOGL::Destroy()
{
  if (!mDestroyed) {
    mDestroyed = PR_TRUE;
    mTexImage = nsnull;
  }
}

Layer*
ShadowCanvasLayerOGL::GetLayer()
{
  return this;
}

void
ShadowCanvasLayerOGL::RenderLayer(int aPreviousFrameBuffer,
                                  const nsIntPoint& aOffset)
{
  mOGLManager->MakeCurrent();

  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
  gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexImage->Texture());
  ColorTextureLayerProgram *program = mOGLManager->GetBGRALayerProgram();

  ApplyFilter(mFilter);

  program->Activate();
  program->SetLayerQuadRect(nsIntRect(nsIntPoint(0, 0), mTexImage->GetSize()));
  program->SetLayerTransform(mTransform);
  program->SetLayerOpacity(GetOpacity());
  program->SetRenderOffset(aOffset);
  program->SetTextureUnit(0);

  mOGLManager->BindAndDrawQuad(program);

  DEBUG_GL_ERROR_CHECK(gl());
}

#endif  
