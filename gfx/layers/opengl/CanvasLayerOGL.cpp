




































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
#if defined(MOZ_WIDGET_GTK2) && !defined(MOZ_PLATFORM_MAEMO)
    if (mPixmap) {
        sGLXLibrary.DestroyPixmap(mPixmap);
        mPixmap = 0;
    }
#endif

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

  mOGLManager->MakeCurrent();

  if (aData.mSurface) {
    mCanvasSurface = aData.mSurface;
    mNeedsYFlip = PR_FALSE;
#if defined(MOZ_WIDGET_GTK2) && !defined(MOZ_PLATFORM_MAEMO)
    mPixmap = sGLXLibrary.CreatePixmap(aData.mSurface);
    if (mPixmap) {
        if (aData.mSurface->GetContentType() == gfxASurface::CONTENT_COLOR_ALPHA) {
            mLayerProgram = gl::RGBALayerProgramType;
        } else {
            mLayerProgram = gl::RGBXLayerProgramType;
        }
        MakeTexture();
    }
#endif
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
      
  
  
  GLint texSize = gl()->GetMaxTextureSize();
  if (mBounds.width > (2 + texSize) || mBounds.height > (2 + texSize)) {
    mDelayedUpdates = PR_TRUE;
    MakeTexture();
    
    
    NS_ABORT_IF_FALSE(mCanvasSurface, 
                      "Invalid texture size when WebGL surface already exists at that size?");
  }
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
CanvasLayerOGL::UpdateSurface()
{
  if (!mDirty)
    return;
  mDirty = PR_FALSE;

  if (mDestroyed || mDelayedUpdates) {
    return;
  }

#if defined(MOZ_WIDGET_GTK2) && !defined(MOZ_PLATFORM_MAEMO)
  if (mPixmap) {
    return;
  }
#endif

  mOGLManager->MakeCurrent();

  if (mCanvasGLContext &&
      mCanvasGLContext->GetContextType() == gl()->GetContextType())
  {
    if (gl()->BindOffscreenNeedsTexture(mCanvasGLContext) &&
        mTexture == 0)
    {
      MakeTexture();
    }
  } else {
    nsRefPtr<gfxASurface> updatedAreaSurface;
    if (mCanvasSurface) {
      updatedAreaSurface = mCanvasSurface;
    } else if (mCanvasGLContext) {
      nsRefPtr<gfxImageSurface> updatedAreaImageSurface =
        new gfxImageSurface(gfxIntSize(mBounds.width, mBounds.height),
                            gfxASurface::ImageFormatARGB32);
      mCanvasGLContext->ReadPixelsIntoImageSurface(0, 0,
                                                   mBounds.width,
                                                   mBounds.height,
                                                   updatedAreaImageSurface);
      updatedAreaSurface = updatedAreaImageSurface;
    }

    mLayerProgram =
      gl()->UploadSurfaceToTexture(updatedAreaSurface,
                                   mBounds,
                                   mTexture,
                                   false,
                                   nsIntPoint(0, 0));
  }
}

void
CanvasLayerOGL::RenderLayer(int aPreviousDestination,
                            const nsIntPoint& aOffset)
{
  UpdateSurface();
  FireDidTransactionCallback();

  mOGLManager->MakeCurrent();

  
  
  

  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);

  if (mTexture) {
    gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);
  }

  ColorTextureLayerProgram *program = nsnull;

  bool useGLContext = mCanvasGLContext &&
    mCanvasGLContext->GetContextType() == gl()->GetContextType();

  nsIntRect drawRect = mBounds;

  if (useGLContext) {
    mCanvasGLContext->MakeCurrent();
    mCanvasGLContext->fFlush();

    gl()->MakeCurrent();
    gl()->BindTex2DOffscreen(mCanvasGLContext);
    program = mOGLManager->GetBasicLayerProgram(CanUseOpaqueSurface(), PR_TRUE);
  } else if (mDelayedUpdates) {
    NS_ABORT_IF_FALSE(mCanvasSurface, "WebGL canvases should always be using full texture upload");
    
    drawRect.IntersectRect(drawRect, GetEffectiveVisibleRegion().GetBounds());

    mLayerProgram =
      gl()->UploadSurfaceToTexture(mCanvasSurface,
                                   nsIntRect(0, 0, drawRect.width, drawRect.height),
                                   mTexture,
                                   true,
                                   drawRect.TopLeft());
  }
  if (!program) { 
    program = mOGLManager->GetColorTextureLayerProgram(mLayerProgram);
  }

#if defined(MOZ_WIDGET_GTK2) && !defined(MOZ_PLATFORM_MAEMO)
  if (mPixmap && !mDelayedUpdates) {
    sGLXLibrary.BindTexImage(mPixmap);
  }
#endif

  ApplyFilter(mFilter);

  program->Activate();
  program->SetLayerQuadRect(drawRect);
  program->SetLayerTransform(GetEffectiveTransform());
  program->SetLayerOpacity(GetEffectiveOpacity());
  program->SetRenderOffset(aOffset);
  program->SetTextureUnit(0);

  mOGLManager->BindAndDrawQuad(program, mNeedsYFlip ? true : false);

#if defined(MOZ_WIDGET_GTK2) && !defined(MOZ_PLATFORM_MAEMO)
  if (mPixmap && !mDelayedUpdates) {
    sGLXLibrary.ReleaseTexImage(mPixmap);
  }
#endif

  if (useGLContext) {
    gl()->UnbindTex2DOffscreen(mCanvasGLContext);
  }
}


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
    mTexImage->DirectUpdate(aNewFront, updateRegion);
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
ShadowCanvasLayerOGL::Disconnect()
{
  Destroy();
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
  ColorTextureLayerProgram *program =
    mOGLManager->GetColorTextureLayerProgram(mTexImage->GetShaderProgramType());

  ApplyFilter(mFilter);

  program->Activate();
  program->SetLayerQuadRect(nsIntRect(nsIntPoint(0, 0), mTexImage->GetSize()));
  program->SetLayerTransform(GetEffectiveTransform());
  program->SetLayerOpacity(GetEffectiveOpacity());
  program->SetRenderOffset(aOffset);
  program->SetTextureUnit(0);

  mOGLManager->BindAndDrawQuad(program);
}
