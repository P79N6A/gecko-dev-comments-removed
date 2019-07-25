




































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

CanvasLayerOGL::~CanvasLayerOGL()
{
  mOGLManager->MakeCurrent();

  if (mTexture) {
    gl()->fDeleteTextures(1, &mTexture);
  }
}

void
CanvasLayerOGL::Initialize(const Data& aData)
{
  NS_ASSERTION(mCanvasSurface == nsnull, "BasicCanvasLayer::Initialize called twice!");

  if (aData.mSurface) {
    mCanvasSurface = aData.mSurface;
    NS_ASSERTION(aData.mGLContext == nsnull,
                 "CanvasLayerOGL can't have both surface and GLContext");
    mNeedsYFlip = PR_FALSE;
    if (mCanvasSurface->GetType() == gfxASurface::SurfaceTypeXlib)
      mCanvasSurfaceAsGLContext = GLContextProvider::CreateForNativePixmapSurface(mCanvasSurface);
  } else if (aData.mGLContext) {
    
    void *pbuffer = aData.mGLContext->GetNativeData(GLContext::NativePBuffer);
    if (!pbuffer) {
      NS_WARNING("CanvasLayerOGL with GL context without NativePBuffer");
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
CanvasLayerOGL::Updated(const nsIntRect& aRect)
{
  NS_ASSERTION(mUpdatedRect.IsEmpty(),
               "CanvasLayer::Updated called more than once during a transaction!");

  mOGLManager->MakeCurrent();

  mUpdatedRect.UnionRect(mUpdatedRect, aRect);

  if (mCanvasSurfaceAsGLContext) {
    PRBool newTexture = mTexture == 0;
    if (newTexture) {
      gl()->fGenTextures(1, &mTexture);

      gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
      gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);

      gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, LOCAL_GL_LINEAR);
      gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, LOCAL_GL_LINEAR);
      gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S, LOCAL_GL_CLAMP_TO_EDGE);
      gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T, LOCAL_GL_CLAMP_TO_EDGE);

      mUpdatedRect = mBounds;
    } else {
      gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
      gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);
    }
    mCanvasSurfaceAsGLContext->BindTexImage();
  } else if (mCanvasSurface) {
    PRBool newTexture = mTexture == 0;
    if (newTexture) {
      gl()->fGenTextures(1, &mTexture);

      gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
      gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);

      gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, LOCAL_GL_LINEAR);
      gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, LOCAL_GL_LINEAR);
      gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S, LOCAL_GL_CLAMP_TO_EDGE);
      gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T, LOCAL_GL_CLAMP_TO_EDGE);

      mUpdatedRect = mBounds;
    } else {
      gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
      gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);
    }

    nsRefPtr<gfxImageSurface> updatedAreaImageSurface;
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
  } else if (mCanvasGLContext) {
    
    PRBool newTexture = mTexture == 0;
    if (newTexture) {
      gl()->fGenTextures(1, (GLuint*)&mTexture);

      gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);

      gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, LOCAL_GL_LINEAR);
      gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, LOCAL_GL_LINEAR);
      gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S, LOCAL_GL_CLAMP_TO_EDGE);
      gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T, LOCAL_GL_CLAMP_TO_EDGE);

      mUpdatedRect = mBounds;
    }

#if defined(XP_MACOSX)
    
    if (newTexture) {
      CGLError err;
      err = CGLTexImagePBuffer((CGLContextObj) gl()->GetNativeData(GLContext::NativeCGLContext),
                               (CGLPBufferObj) mCanvasGLContext->GetNativeData(GLContext::NativePBuffer),
                               LOCAL_GL_BACK);
      if (err) {
        NS_WARNING("CanvasLayerOGL::Updated CGLTexImagePBuffer failed");
      }
    }
#elif defined(XP_WIN)
    
    if (!sWGLLibrary.fBindTexImage((HANDLE) mCanvasGLContext->GetNativeData(GLContext::NativePBuffer),
                                   LOCAL_WGL_FRONT_LEFT_ARB)) {
      NS_WARNING("CanvasLayerOGL::Updated wglBindTexImageARB failed");
    }
#else
    NS_WARNING("CanvasLayerOGL::Updated with GL context, but I don't know how to render on this platform!");
#endif
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
  gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);

  if (mCanvasGLContext || mCanvasSurfaceAsGLContext) {
    program = mOGLManager->GetRGBALayerProgram();
  } else {
    program = mOGLManager->GetBGRALayerProgram();
  }

  program->Activate();
  program->SetLayerQuadRect(mBounds);
  program->SetLayerTransform(mTransform);
  program->SetLayerOpacity(GetOpacity());
  program->SetRenderOffset(aOffset);
  program->SetTextureUnit(0);

  mOGLManager->BindAndDrawQuad(program, mNeedsYFlip ? true : false);

#if defined(XP_WIN)
  
  
  if (mCanvasGLContext) {
    sWGLLibrary.fReleaseTexImage((HANDLE) mCanvasGLContext->GetNativeData(GLContext::NativePBuffer),
                                 LOCAL_WGL_FRONT_LEFT_ARB);
  }
#endif

  mUpdatedRect.Empty();
}
