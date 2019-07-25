




































#include "CanvasLayerOGL.h"

#include "gfxImageSurface.h"
#include "gfxContext.h"

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
  LayerManagerOGL *glManager = static_cast<LayerManagerOGL*>(mManager);
  GLContext *gl = glManager->gl();
  glManager->MakeCurrent();

  if (mTexture) {
    gl->fDeleteTextures(1, &mTexture);
  }
}

void
CanvasLayerOGL::Initialize(const Data& aData)
{
  NS_ASSERTION(mSurface == nsnull, "BasicCanvasLayer::Initialize called twice!");

  if (aData.mSurface) {
    mSurface = aData.mSurface;
    NS_ASSERTION(aData.mGLContext == nsnull,
                 "CanvasLayerOGL can't have both surface and GLContext");
    mNeedsYFlip = PR_TRUE;
  } else if (aData.mGLContext) {
    
    void *pbuffer = aData.mGLContext->GetNativeData(GLContext::NativePBuffer);
    if (!pbuffer) {
      NS_WARNING("CanvasLayerOGL with GL context without NativePBuffer");
      return;
    }

    mGLContext = aData.mGLContext;
    mGLBufferIsPremultiplied = aData.mGLBufferIsPremultiplied;
    mNeedsYFlip = PR_FALSE;
  } else {
    NS_WARNING("CanvasLayerOGL::Initialize called without surface or GL context!");
    return;
  }

  mBounds.SetRect(0, 0, aData.mSize.width, aData.mSize.height);
}

void
CanvasLayerOGL::Updated(const nsIntRect& aRect)
{
  LayerManagerOGL *glManager = static_cast<LayerManagerOGL*>(mManager);
  GLContext *gl = glManager->gl();
  glManager->MakeCurrent();

  NS_ASSERTION(mUpdatedRect.IsEmpty(), "CanvasLayer::Updated called more than once during a transaction!");

  mUpdatedRect.UnionRect(mUpdatedRect, aRect);

  if (mSurface) {
    if (mTexture == 0) {
      gl->fGenTextures(1, (GLuint*)&mTexture);

      gl->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);

      gl->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, LOCAL_GL_LINEAR);
      gl->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, LOCAL_GL_LINEAR);
      gl->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S, LOCAL_GL_CLAMP_TO_EDGE);
      gl->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T, LOCAL_GL_CLAMP_TO_EDGE);

      mUpdatedRect = mBounds;
    } else {
      gl->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);
    }

    nsRefPtr<gfxImageSurface> updatedAreaImageSurface;
    nsRefPtr<gfxASurface> sourceSurface = mSurface;

#ifdef XP_WIN
    if (sourceSurface->GetType() == gfxASurface::SurfaceTypeWin32) {
      sourceSurface = static_cast<gfxWindowsSurface*>(sourceSurface.get())->GetImageSurface();
      if (!sourceSurface)
        sourceSurface = mSurface;
    }
#endif

#if 0
    
    
    if (mSurface->GetType() == gfxASurface::SurfaceTypeImage) {
      gfxImageSurface *s = static_cast<gfxImageSurface*>(mSurface.get());
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
      updatedAreaImageSurface = new gfxImageSurface(gfxIntSize(mUpdatedRect.width, mUpdatedRect.height),
                                                    gfxASurface::ImageFormatARGB32);
      nsRefPtr<gfxContext> ctx = new gfxContext(updatedAreaImageSurface);
      ctx->Translate(gfxPoint(-mUpdatedRect.x, -mUpdatedRect.y));
      ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
      ctx->SetSource(sourceSurface);
      ctx->Paint();
    }

    if (mUpdatedRect == mBounds) {
      gl->fTexImage2D(LOCAL_GL_TEXTURE_2D,
                      0,
                      LOCAL_GL_RGBA,
                      mUpdatedRect.width,
                      mUpdatedRect.height,
                      0,
                      LOCAL_GL_BGRA,
                      LOCAL_GL_UNSIGNED_BYTE,
                      updatedAreaImageSurface->Data());
    } else {
      gl->fTexSubImage2D(LOCAL_GL_TEXTURE_2D,
                         0,
                         mUpdatedRect.x,
                         mUpdatedRect.y,
                         mUpdatedRect.width,
                         mUpdatedRect.height,
                         LOCAL_GL_BGRA,
                         LOCAL_GL_UNSIGNED_BYTE,
                         updatedAreaImageSurface->Data());
    }
  } else if (mGLContext) {
    
    if (mTexture == 0) {
      gl->fGenTextures(1, (GLuint*)&mTexture);

      gl->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);

      gl->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, LOCAL_GL_LINEAR);
      gl->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, LOCAL_GL_LINEAR);
      gl->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S, LOCAL_GL_CLAMP_TO_EDGE);
      gl->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T, LOCAL_GL_CLAMP_TO_EDGE);

      mUpdatedRect = mBounds;
    }
  }

  
  NS_ASSERTION(mUpdatedRect.IsEmpty() || mBounds.Contains(mUpdatedRect), "CanvasLayer: Updated rect bigger than bounds!");
}

void
CanvasLayerOGL::RenderLayer(int aPreviousDestination,
                            DrawThebesLayerCallback aCallback,
                            void* aCallbackData)
{
  LayerManagerOGL *glManager = static_cast<LayerManagerOGL*>(mManager);
  GLContext *gl = glManager->gl();
  glManager->MakeCurrent();

  float quadTransform[4][4];
  
  memset(&quadTransform, 0, sizeof(quadTransform));
  quadTransform[0][0] = (float)mBounds.width;
  quadTransform[1][1] = (float)mBounds.height;
  quadTransform[2][2] = 1.0f;
  quadTransform[3][3] = 1.0f;

  
  
  

  
  

  RGBLayerProgram *program = glManager->GetRGBLayerProgram();

  program->Activate();

  program->SetLayerQuadTransform(&quadTransform[0][0]);

  gl->fActiveTexture(LOCAL_GL_TEXTURE0);
  gl->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);

  if (mGLContext) {
#if defined(XP_MACOSX)
    CGLError err;
    err = CGLTexImagePBuffer((CGLContextObj) gl->GetNativeData(GLContext::NativeCGLContext),
                             (CGLPBufferObj) mGLContext->GetNativeData(GLContext::NativePBuffer),
                             LOCAL_GL_BACK);
#elif defined(XP_WIN)
    if (!sWGLLibrary.fBindTexImage((HANDLE) mGLContext->GetNativeData(GLContext::NativePBuffer),
                                   LOCAL_WGL_FRONT_LEFT_ARB))
    {
      NS_WARNING("CanvasLayerOGL::RenderLayer wglBindTexImageARB failed");
      return;
    }
#else
    NS_WARNING("CanvasLayerOGL::RenderLayer with GL context, but I don't know how to render on this platform!");
#endif
  }

  program->SetLayerOpacity(GetOpacity());
  program->SetLayerTransform(&mTransform._11);
  program->Apply();

  gl->fDrawArrays(LOCAL_GL_TRIANGLE_STRIP, 0, 4);

  if (mGLContext) {
#if defined(XP_WIN)
    sWGLLibrary.fReleaseTexImage((HANDLE) mGLContext->GetNativeData(GLContext::NativePBuffer),
                                 LOCAL_WGL_FRONT_LEFT_ARB);
#endif
  }
}
