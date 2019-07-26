




#include "gfxSharedImageSurface.h"

#include "ImageContainer.h" 
#include "mozilla/layers/ShmemYCbCrImage.h"
#include "ipc/AutoOpenSurface.h"
#include "ImageLayerOGL.h"
#include "gfxImageSurface.h"
#include "gfxUtils.h"
#include "yuv_convert.h"
#include "GLContextProvider.h"
#if defined(GL_PROVIDER_GLX)
# include "GLXLibrary.h"
# include "gfxXlibSurface.h"
#endif
#include "SharedTextureImage.h"

#ifdef MOZ_WIDGET_ANDROID
#include "nsSurfaceTexture.h"
#endif

#ifdef MOZ_WIDGET_GONK
# include "GonkIOSurfaceImage.h"
# include <ui/GraphicBuffer.h>
using namespace android;
#endif

using namespace mozilla::gfx;
using namespace mozilla::gl;

namespace mozilla {
namespace layers {





class TextureDeleter : public nsRunnable {
public:
  TextureDeleter(already_AddRefed<GLContext> aContext,
                 GLuint aTexture)
      : mContext(aContext), mTexture(aTexture)
  {
    NS_ASSERTION(aTexture, "TextureDeleter instantiated with nothing to do");
  }

  NS_IMETHOD Run() {
    mContext->MakeCurrent();
    mContext->fDeleteTextures(1, &mTexture);

    
    mContext = nullptr;
    return NS_OK;
  }

  nsRefPtr<GLContext> mContext;
  GLuint mTexture;
};

void
GLTexture::Allocate(GLContext *aContext)
{
  NS_ASSERTION(aContext->IsGlobalSharedContext() || aContext->IsOwningThreadCurrent(),
               "Can only allocate texture on context's owning thread or with cx sharing");

  Release();

  mContext = aContext;

  mContext->MakeCurrent();
  mContext->fGenTextures(1, &mTexture);
}

void
GLTexture::TakeFrom(GLTexture *aOther)
{
  Release();

  mContext = aOther->mContext.forget();
  mTexture = aOther->mTexture;
  aOther->mTexture = 0;
}

void
GLTexture::Release()
{
  if (!mContext) {
    NS_ASSERTION(!mTexture, "Can't delete texture without a context");
    return;
  }

  if (mContext->IsDestroyed() && !mContext->IsGlobalSharedContext()) {
    mContext = mContext->GetSharedContext();
    if (!mContext) {
      NS_ASSERTION(!mTexture, 
                   "Context has been destroyed and couldn't find a shared context!");
      return;
    }
  }

  if (mTexture) {
    if (mContext->IsOwningThreadCurrent() || mContext->IsGlobalSharedContext()) {
      mContext->MakeCurrent();
      mContext->fDeleteTextures(1, &mTexture);
    } else {
      nsCOMPtr<nsIRunnable> runnable =
        new TextureDeleter(mContext.get(), mTexture);
      mContext->DispatchToOwningThread(runnable);
      mContext.forget();
    }

    mTexture = 0;
  }

  mContext = nullptr;
}

TextureRecycleBin::TextureRecycleBin()
  : mLock("mozilla.layers.TextureRecycleBin.mLock")
{
}

void
TextureRecycleBin::RecycleTexture(GLTexture *aTexture, TextureType aType,
                           const gfxIntSize& aSize)
{
  MutexAutoLock lock(mLock);

  if (!aTexture->IsAllocated())
    return;

  if (!mRecycledTextures[aType].IsEmpty() && aSize != mRecycledTextureSizes[aType]) {
    mRecycledTextures[aType].Clear();
  }
  mRecycledTextureSizes[aType] = aSize;
  mRecycledTextures[aType].AppendElement()->TakeFrom(aTexture);
}

void
TextureRecycleBin::GetTexture(TextureType aType, const gfxIntSize& aSize,
                       GLContext *aContext, GLTexture *aOutTexture)
{
  MutexAutoLock lock(mLock);

  if (mRecycledTextures[aType].IsEmpty() || mRecycledTextureSizes[aType] != aSize) {
    aOutTexture->Allocate(aContext);
    return;
  }
  uint32_t last = mRecycledTextures[aType].Length() - 1;
  aOutTexture->TakeFrom(&mRecycledTextures[aType].ElementAt(last));
  mRecycledTextures[aType].RemoveElementAt(last);
}

struct THEBES_API ImageOGLBackendData : public ImageBackendData
{
  GLTexture mTexture;
};

#ifdef XP_MACOSX
void
AllocateTextureIOSurface(MacIOSurfaceImage *aIOImage, mozilla::gl::GLContext* aGL)
{
  nsAutoPtr<ImageOGLBackendData> backendData(
    new ImageOGLBackendData);

  backendData->mTexture.Allocate(aGL);
  aGL->fBindTexture(LOCAL_GL_TEXTURE_RECTANGLE_ARB, backendData->mTexture.GetTextureID());
  aGL->fTexParameteri(LOCAL_GL_TEXTURE_RECTANGLE_ARB,
                     LOCAL_GL_TEXTURE_MIN_FILTER,
                     LOCAL_GL_NEAREST);
  aGL->fTexParameteri(LOCAL_GL_TEXTURE_RECTANGLE_ARB,
                     LOCAL_GL_TEXTURE_MAG_FILTER,
                     LOCAL_GL_NEAREST);

  void *nativeCtx = aGL->GetNativeData(GLContext::NativeGLContext);

  aIOImage->GetIOSurface()->CGLTexImageIOSurface2D(nativeCtx,
                                     LOCAL_GL_RGBA, LOCAL_GL_BGRA,
                                     LOCAL_GL_UNSIGNED_INT_8_8_8_8_REV, 0);

  aGL->fBindTexture(LOCAL_GL_TEXTURE_RECTANGLE_ARB, 0);

  aIOImage->SetBackendData(LAYERS_OPENGL, backendData.forget());
}
#endif

void
AllocateTextureSharedTexture(SharedTextureImage *aTexImage, mozilla::gl::GLContext* aGL, GLenum aTarget)
{
  nsAutoPtr<ImageOGLBackendData> backendData(
    new ImageOGLBackendData);

  backendData->mTexture.Allocate(aGL);

  aGL->fBindTexture(aTarget, backendData->mTexture.GetTextureID());
  aGL->fTexParameteri(aTarget, LOCAL_GL_TEXTURE_MIN_FILTER, LOCAL_GL_LINEAR);
  aGL->fTexParameteri(aTarget, LOCAL_GL_TEXTURE_MAG_FILTER, LOCAL_GL_LINEAR);
  aGL->fTexParameteri(aTarget, LOCAL_GL_TEXTURE_WRAP_S, LOCAL_GL_CLAMP_TO_EDGE);
  aGL->fTexParameteri(aTarget, LOCAL_GL_TEXTURE_WRAP_T, LOCAL_GL_CLAMP_TO_EDGE);

  aTexImage->SetBackendData(LAYERS_OPENGL, backendData.forget());
}

#ifdef MOZ_WIDGET_GONK
struct THEBES_API GonkIOSurfaceImageOGLBackendData : public ImageBackendData
{
  GLTexture mTexture;
};

void
AllocateTextureIOSurface(GonkIOSurfaceImage *aIOImage, mozilla::gl::GLContext* aGL)
{
  nsAutoPtr<GonkIOSurfaceImageOGLBackendData> backendData(
    new GonkIOSurfaceImageOGLBackendData);

  backendData->mTexture.Allocate(aGL);
  aIOImage->SetBackendData(LAYERS_OPENGL, backendData.forget());
}
#endif

Layer*
ImageLayerOGL::GetLayer()
{
  return this;
}

void
ImageLayerOGL::RenderLayer(int,
                           const nsIntPoint& aOffset)
{
  nsRefPtr<ImageContainer> container = GetContainer();

  if (!container || mOGLManager->CompositingDisabled())
    return;

  mOGLManager->MakeCurrent();

  AutoLockImage autoLock(container);

  Image *image = autoLock.GetImage();
  if (!image) {
    return;
  }

  NS_ASSERTION(image->GetFormat() != REMOTE_IMAGE_BITMAP,
    "Remote images aren't handled yet in OGL layers!");

  if (image->GetFormat() == PLANAR_YCBCR) {
    PlanarYCbCrImage *yuvImage =
      static_cast<PlanarYCbCrImage*>(image);

    if (!yuvImage->IsValid()) {
      return;
    }

    PlanarYCbCrOGLBackendData *data =
      static_cast<PlanarYCbCrOGLBackendData*>(yuvImage->GetBackendData(LAYERS_OPENGL));

    if (data && data->mTextures->GetGLContext() != gl()) {
      
      
      data = nullptr;
      yuvImage->SetBackendData(LAYERS_OPENGL, nullptr);
    }

    if (!data) {
      AllocateTexturesYCbCr(yuvImage);
      data = static_cast<PlanarYCbCrOGLBackendData*>(yuvImage->GetBackendData(LAYERS_OPENGL));
    }

    if (!data || data->mTextures->GetGLContext() != gl()) {
      
      return;
    }

    gl()->MakeCurrent();
    gl()->fActiveTexture(LOCAL_GL_TEXTURE2);
    gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, data->mTextures[2].GetTextureID());
    gl()->ApplyFilterToBoundTexture(mFilter);
    gl()->fActiveTexture(LOCAL_GL_TEXTURE1);
    gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, data->mTextures[1].GetTextureID());
    gl()->ApplyFilterToBoundTexture(mFilter);
    gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
    gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, data->mTextures[0].GetTextureID());
    gl()->ApplyFilterToBoundTexture(mFilter);
    
    ShaderProgramOGL *program = mOGLManager->GetProgram(YCbCrLayerProgramType,
                                                        GetMaskLayer());

    program->Activate();
    program->SetLayerQuadRect(nsIntRect(0, 0,
                                        yuvImage->GetSize().width,
                                        yuvImage->GetSize().height));
    program->SetLayerTransform(GetEffectiveTransform());
    program->SetLayerOpacity(GetEffectiveOpacity());
    program->SetRenderOffset(aOffset);
    program->SetYCbCrTextureUnits(0, 1, 2);
    program->LoadMask(GetMaskLayer());

    mOGLManager->BindAndDrawQuadWithTextureRect(program,
                                                yuvImage->GetData()->GetPictureRect(),
                                                nsIntSize(yuvImage->GetData()->mYSize.width,
                                                          yuvImage->GetData()->mYSize.height));

    
    
    gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
  } else if (image->GetFormat() == CAIRO_SURFACE) {
    CairoImage *cairoImage =
      static_cast<CairoImage*>(image);

    if (!cairoImage->mSurface) {
      return;
    }

    NS_ASSERTION(cairoImage->mSurface->GetContentType() != gfxASurface::CONTENT_ALPHA,
                 "Image layer has alpha image");

    CairoOGLBackendData *data =
      static_cast<CairoOGLBackendData*>(cairoImage->GetBackendData(LAYERS_OPENGL));

    if (data && data->mTexture.GetGLContext() != gl()) {
      
      
      data = nullptr;
      cairoImage->SetBackendData(LAYERS_OPENGL, nullptr);
    }

    if (!data) {
      AllocateTexturesCairo(cairoImage);
      data = static_cast<CairoOGLBackendData*>(cairoImage->GetBackendData(LAYERS_OPENGL));
    }

    if (!data || data->mTexture.GetGLContext() != gl()) {
      
      return;
    }

    gl()->MakeCurrent();

    gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
    gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, data->mTexture.GetTextureID());

    ShaderProgramOGL *program = 
      mOGLManager->GetProgram(data->mLayerProgram, GetMaskLayer());

    gl()->ApplyFilterToBoundTexture(mFilter);

    program->Activate();
    program->SetLayerQuadRect(nsIntRect(0, 0, 
                                        cairoImage->GetSize().width, 
                                        cairoImage->GetSize().height));
    program->SetLayerTransform(GetEffectiveTransform());
    program->SetLayerOpacity(GetEffectiveOpacity());
    program->SetRenderOffset(aOffset);
    program->SetTextureUnit(0);
    program->LoadMask(GetMaskLayer());

    mOGLManager->BindAndDrawQuad(program);
#ifdef XP_MACOSX
  } else if (image->GetFormat() == MAC_IO_SURFACE) {
     MacIOSurfaceImage *ioImage =
       static_cast<MacIOSurfaceImage*>(image);

     gl()->fActiveTexture(LOCAL_GL_TEXTURE0);

     if (!ioImage->GetBackendData(LAYERS_OPENGL)) {
       AllocateTextureIOSurface(ioImage, gl());
     }

     ImageOGLBackendData *data =
      static_cast<ImageOGLBackendData*>(ioImage->GetBackendData(LAYERS_OPENGL));

     gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
     gl()->fBindTexture(LOCAL_GL_TEXTURE_RECTANGLE_ARB, data->mTexture.GetTextureID());

     ShaderProgramOGL *program =
       mOGLManager->GetProgram(gl::RGBARectLayerProgramType, GetMaskLayer());

     program->Activate();
     if (program->GetTexCoordMultiplierUniformLocation() != -1) {
       
       program->SetTexCoordMultiplier(ioImage->GetSize().width, ioImage->GetSize().height);
     } else {
       NS_ASSERTION(0, "no rects?");
     }

     program->SetLayerQuadRect(nsIntRect(0, 0,
                                         ioImage->GetSize().width,
                                         ioImage->GetSize().height));
     program->SetLayerTransform(GetEffectiveTransform());
     program->SetLayerOpacity(GetEffectiveOpacity());
     program->SetRenderOffset(aOffset);
     program->SetTextureUnit(0);
     program->LoadMask(GetMaskLayer());

     mOGLManager->BindAndDrawQuad(program);
     gl()->fBindTexture(LOCAL_GL_TEXTURE_RECTANGLE_ARB, 0);
#endif
  } else if (image->GetFormat() == SHARED_TEXTURE) {
    SharedTextureImage* texImage =
      static_cast<SharedTextureImage*>(image);
    const SharedTextureImage::Data* data = texImage->GetData();
    GLContext::SharedHandleDetails handleDetails;
    if (!gl()->GetSharedHandleDetails(data->mShareType, data->mHandle, handleDetails)) {
      NS_ERROR("Failed to get shared handle details");
      return;
    }

    ShaderProgramOGL* program = mOGLManager->GetProgram(handleDetails.mProgramType, GetMaskLayer());

    program->Activate();
    if (handleDetails.mProgramType == gl::RGBARectLayerProgramType) {
      
      program->SetTexCoordMultiplier(data->mSize.width, data->mSize.height);
    }
    program->SetLayerTransform(GetEffectiveTransform());
    program->SetLayerOpacity(GetEffectiveOpacity());
    program->SetRenderOffset(aOffset);
    program->SetTextureUnit(0);
    program->SetTextureTransform(handleDetails.mTextureTransform);
    program->LoadMask(GetMaskLayer());

    if (!texImage->GetBackendData(LAYERS_OPENGL)) {
      AllocateTextureSharedTexture(texImage, gl(), handleDetails.mTarget);
    }

    ImageOGLBackendData *backendData =
      static_cast<ImageOGLBackendData*>(texImage->GetBackendData(LAYERS_OPENGL));
    gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
    gl()->fBindTexture(handleDetails.mTarget, backendData->mTexture.GetTextureID());

    if (!gl()->AttachSharedHandle(data->mShareType, data->mHandle)) {
      NS_ERROR("Failed to bind shared texture handle");
      return;
    }

    gl()->ApplyFilterToBoundTexture(handleDetails.mTarget, mFilter);
    program->SetLayerQuadRect(nsIntRect(nsIntPoint(0, 0), data->mSize));
    mOGLManager->BindAndDrawQuad(program, data->mInverted);
    gl()->fBindTexture(handleDetails.mTarget, 0);
    gl()->DetachSharedHandle(data->mShareType, data->mHandle);
#ifdef MOZ_WIDGET_GONK
  } else if (image->GetFormat() == GONK_IO_SURFACE) {

    GonkIOSurfaceImage *ioImage = static_cast<GonkIOSurfaceImage*>(image);
    if (!ioImage) {
      return;
    }

    gl()->MakeCurrent();
    gl()->fActiveTexture(LOCAL_GL_TEXTURE0);

    if (!ioImage->GetBackendData(LAYERS_OPENGL)) {
      AllocateTextureIOSurface(ioImage, gl());
    }
    GonkIOSurfaceImageOGLBackendData *data =
      static_cast<GonkIOSurfaceImageOGLBackendData*>(ioImage->GetBackendData(LAYERS_OPENGL));

    gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
    gl()->BindExternalBuffer(data->mTexture.GetTextureID(), ioImage->GetNativeBuffer());

    ShaderProgramOGL *program = mOGLManager->GetProgram(RGBAExternalLayerProgramType, GetMaskLayer());

    gl()->ApplyFilterToBoundTexture(mFilter);

    program->Activate();
    program->SetLayerQuadRect(nsIntRect(0, 0, 
                                        ioImage->GetSize().width, 
                                        ioImage->GetSize().height));
    program->SetLayerTransform(GetEffectiveTransform());
    program->SetLayerOpacity(GetEffectiveOpacity());
    program->SetRenderOffset(aOffset);
    program->SetTextureUnit(0);
    program->LoadMask(GetMaskLayer());

    mOGLManager->BindAndDrawQuadWithTextureRect(program,
                                                GetVisibleRegion().GetBounds(),
                                                nsIntSize(ioImage->GetSize().width,
                                                          ioImage->GetSize().height));
#endif
  }
  GetContainer()->NotifyPaintedImage(image);
}

static void
SetClamping(GLContext* aGL, GLuint aTexture)
{
  aGL->fBindTexture(LOCAL_GL_TEXTURE_2D, aTexture);
  aGL->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S, LOCAL_GL_CLAMP_TO_EDGE);
  aGL->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T, LOCAL_GL_CLAMP_TO_EDGE);
}

static void
UploadYUVToTexture(GLContext* gl, const PlanarYCbCrImage::Data& aData, 
                   GLTexture* aYTexture,
                   GLTexture* aUTexture,
                   GLTexture* aVTexture)
{
  nsIntRect size(0, 0, aData.mYSize.width, aData.mYSize.height);
  GLuint texture = aYTexture->GetTextureID();
  nsRefPtr<gfxASurface> surf = new gfxImageSurface(aData.mYChannel,
                                                   aData.mYSize,
                                                   aData.mYStride,
                                                   gfxASurface::ImageFormatA8);
  gl->UploadSurfaceToTexture(surf, size, texture, true);
  
  size = nsIntRect(0, 0, aData.mCbCrSize.width, aData.mCbCrSize.height);
  texture = aUTexture->GetTextureID();
  surf = new gfxImageSurface(aData.mCbChannel,
                             aData.mCbCrSize,
                             aData.mCbCrStride,
                             gfxASurface::ImageFormatA8);
  gl->UploadSurfaceToTexture(surf, size, texture, true);

  texture = aVTexture->GetTextureID();
  surf = new gfxImageSurface(aData.mCrChannel,
                             aData.mCbCrSize,
                             aData.mCbCrStride,
                             gfxASurface::ImageFormatA8);
  gl->UploadSurfaceToTexture(surf, size, texture, true);
}

ImageLayerOGL::ImageLayerOGL(LayerManagerOGL *aManager)
  : ImageLayer(aManager, NULL)
  , LayerOGL(aManager)
  , mTextureRecycleBin(new TextureRecycleBin())
{ 
  mImplData = static_cast<LayerOGL*>(this);
}

void
ImageLayerOGL::AllocateTexturesYCbCr(PlanarYCbCrImage *aImage)
{
  if (!aImage->IsValid())
    return;

  nsAutoPtr<PlanarYCbCrOGLBackendData> backendData(
    new PlanarYCbCrOGLBackendData);

  const PlanarYCbCrImage::Data *data = aImage->GetData();

  gl()->MakeCurrent();

  mTextureRecycleBin->GetTexture(TextureRecycleBin::TEXTURE_Y, data->mYSize, gl(), &backendData->mTextures[0]);
  SetClamping(gl(), backendData->mTextures[0].GetTextureID());

  mTextureRecycleBin->GetTexture(TextureRecycleBin::TEXTURE_C, data->mCbCrSize, gl(), &backendData->mTextures[1]);
  SetClamping(gl(), backendData->mTextures[1].GetTextureID());

  mTextureRecycleBin->GetTexture(TextureRecycleBin::TEXTURE_C, data->mCbCrSize, gl(), &backendData->mTextures[2]);
  SetClamping(gl(), backendData->mTextures[2].GetTextureID());

  UploadYUVToTexture(gl(), *data,
                     &backendData->mTextures[0],
                     &backendData->mTextures[1],
                     &backendData->mTextures[2]);

  backendData->mYSize = data->mYSize;
  backendData->mCbCrSize = data->mCbCrSize;
  backendData->mTextureRecycleBin = mTextureRecycleBin;

  aImage->SetBackendData(LAYERS_OPENGL, backendData.forget());
}

void
ImageLayerOGL::AllocateTexturesCairo(CairoImage *aImage)
{
  nsAutoPtr<CairoOGLBackendData> backendData(
    new CairoOGLBackendData);

  GLTexture &texture = backendData->mTexture;

  texture.Allocate(gl());

  if (!texture.IsAllocated()) {
    return;
  }

  mozilla::gl::GLContext *gl = texture.GetGLContext();
  gl->MakeCurrent();

  GLuint tex = texture.GetTextureID();
  gl->fActiveTexture(LOCAL_GL_TEXTURE0);

  SetClamping(gl, tex);

#if defined(MOZ_X11) && !defined(MOZ_PLATFORM_MAEMO)
  if (aImage->mSurface->GetType() == gfxASurface::SurfaceTypeXlib) {
    gfxXlibSurface *xsurf =
      static_cast<gfxXlibSurface*>(aImage->mSurface.get());
    GLXPixmap pixmap = xsurf->GetGLXPixmap();
    if (pixmap) {
      if (aImage->mSurface->GetContentType()
          == gfxASurface::CONTENT_COLOR_ALPHA) {
        backendData->mLayerProgram = gl::RGBALayerProgramType;
      } else {
        backendData->mLayerProgram = gl::RGBXLayerProgramType;
      }

      aImage->SetBackendData(LAYERS_OPENGL, backendData.forget());

      sDefGLXLib.BindTexImage(pixmap);

      return;
    }
  }
#endif
  backendData->mLayerProgram =
    gl->UploadSurfaceToTexture(aImage->mSurface,
                               nsIntRect(0,0, aImage->mSize.width, aImage->mSize.height),
                               tex, true);

  aImage->SetBackendData(LAYERS_OPENGL, backendData.forget());
}







static gfxIntSize
CalculatePOTSize(const gfxIntSize& aSize, GLContext* gl)
{
  if (gl->CanUploadNonPowerOfTwo())
    return aSize;

  return gfxIntSize(NextPowerOfTwo(aSize.width), NextPowerOfTwo(aSize.height));
}

bool
ImageLayerOGL::LoadAsTexture(GLuint aTextureUnit, gfxIntSize* aSize)
{
  
  

  if (!GetContainer()) {
    return false;
  }

  AutoLockImage autoLock(GetContainer());

  Image *image = autoLock.GetImage();
  if (!image) {
    return false;
  }

  if (image->GetFormat() != CAIRO_SURFACE) {
    return false;
  }

  CairoImage* cairoImage = static_cast<CairoImage*>(image);

  if (!cairoImage->mSurface) {
    return false;
  }

  CairoOGLBackendData *data = static_cast<CairoOGLBackendData*>(
    cairoImage->GetBackendData(LAYERS_OPENGL));

  if (!data) {
    NS_ASSERTION(cairoImage->mSurface->GetContentType() == gfxASurface::CONTENT_ALPHA,
                 "OpenGL mask layers must be backed by alpha surfaces");

    
    data = new CairoOGLBackendData;
    data->mTextureSize = CalculatePOTSize(cairoImage->mSize, gl());

    GLTexture &texture = data->mTexture;
    texture.Allocate(mOGLManager->gl());

    if (!texture.IsAllocated()) {
      return false;
    }

    mozilla::gl::GLContext *texGL = texture.GetGLContext();
    texGL->MakeCurrent();

    GLuint texID = texture.GetTextureID();

    data->mLayerProgram =
      texGL->UploadSurfaceToTexture(cairoImage->mSurface,
                                    nsIntRect(0,0,
                                              data->mTextureSize.width,
                                              data->mTextureSize.height),
                                    texID, true, nsIntPoint(0,0), false,
                                    aTextureUnit);

    cairoImage->SetBackendData(LAYERS_OPENGL, data);

    gl()->MakeCurrent();
    gl()->fActiveTexture(aTextureUnit);
    gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, texID);
    gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER,
                         LOCAL_GL_LINEAR);
    gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER,
                         LOCAL_GL_LINEAR);
    gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S,
                         LOCAL_GL_CLAMP_TO_EDGE);
    gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T,
                         LOCAL_GL_CLAMP_TO_EDGE);
  } else {
    gl()->fActiveTexture(aTextureUnit);
    gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, data->mTexture.GetTextureID());
  }

  *aSize = data->mTextureSize;
  return true;
}

} 
} 
