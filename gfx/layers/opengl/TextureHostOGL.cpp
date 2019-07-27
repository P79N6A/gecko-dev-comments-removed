




#include "TextureHostOGL.h"

#include "EGLUtils.h"
#include "GLContext.h"                  
#include "GLLibraryEGL.h"               
#include "GLUploadHelpers.h"
#include "GLReadTexImageHelper.h"
#include "gfx2DGlue.h"                  
#include "gfxReusableSurfaceWrapper.h"  
#include "mozilla/gfx/2D.h"             
#include "mozilla/gfx/BaseSize.h"       
#include "mozilla/gfx/Logging.h"        
#ifdef MOZ_WIDGET_GONK
# include "GrallocImages.h"  
# include "EGLImageHelpers.h"
#endif
#include "mozilla/layers/ISurfaceAllocator.h"
#include "mozilla/layers/YCbCrImageDataSerializer.h"
#include "mozilla/layers/GrallocTextureHost.h"
#include "nsRegion.h"                   
#include "AndroidSurfaceTexture.h"
#include "GfxTexturesReporter.h"        
#include "GLBlitTextureImageHelper.h"
#ifdef XP_MACOSX
#include "SharedSurfaceIO.h"
#include "mozilla/layers/MacIOSurfaceTextureHostOGL.h"
#endif
#include "GeckoProfiler.h"

using namespace mozilla::gl;
using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

class Compositor;

TemporaryRef<TextureHost>
CreateTextureHostOGL(const SurfaceDescriptor& aDesc,
                     ISurfaceAllocator* aDeallocator,
                     TextureFlags aFlags)
{
  RefPtr<TextureHost> result;
  switch (aDesc.type()) {
    case SurfaceDescriptor::TSurfaceDescriptorShmem:
    case SurfaceDescriptor::TSurfaceDescriptorMemory: {
      result = CreateBackendIndependentTextureHost(aDesc,
                                                   aDeallocator, aFlags);
      break;
    }

#ifdef MOZ_WIDGET_ANDROID
    case SurfaceDescriptor::TSurfaceTextureDescriptor: {
      const SurfaceTextureDescriptor& desc = aDesc.get_SurfaceTextureDescriptor();
      result = new SurfaceTextureHost(aFlags,
                                      (AndroidSurfaceTexture*)desc.surfTex(),
                                      desc.size());
      break;
    }
#endif

    case SurfaceDescriptor::TEGLImageDescriptor: {
      const EGLImageDescriptor& desc = aDesc.get_EGLImageDescriptor();
      result = new EGLImageTextureHost(aFlags,
                                       (EGLImage)desc.image(),
                                       (EGLSync)desc.fence(),
                                       desc.size());
      break;
    }

#ifdef XP_MACOSX
    case SurfaceDescriptor::TSurfaceDescriptorMacIOSurface: {
      const SurfaceDescriptorMacIOSurface& desc =
        aDesc.get_SurfaceDescriptorMacIOSurface();
      result = new MacIOSurfaceTextureHostOGL(aFlags, desc);
      break;
    }
#endif

#ifdef MOZ_WIDGET_GONK
    case SurfaceDescriptor::TNewSurfaceDescriptorGralloc: {
      const NewSurfaceDescriptorGralloc& desc =
        aDesc.get_NewSurfaceDescriptorGralloc();
      result = new GrallocTextureHostOGL(aFlags, desc);
      break;
    }
#endif
    default: return nullptr;
  }
  return result.forget();
}

static gl::TextureImage::Flags
FlagsToGLFlags(TextureFlags aFlags)
{
  uint32_t result = TextureImage::NoFlags;

  if (aFlags & TextureFlags::USE_NEAREST_FILTER)
    result |= TextureImage::UseNearestFilter;
  if (aFlags & TextureFlags::ORIGIN_BOTTOM_LEFT)
    result |= TextureImage::OriginBottomLeft;
  if (aFlags & TextureFlags::DISALLOW_BIGIMAGE)
    result |= TextureImage::DisallowBigImage;

  return static_cast<gl::TextureImage::Flags>(result);
}

#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
bool
TextureHostOGL::SetReleaseFence(const android::sp<android::Fence>& aReleaseFence)
{
  if (!aReleaseFence.get() || !aReleaseFence->isValid()) {
    
    
    return false;
  }

  if (!mReleaseFence.get()) {
    mReleaseFence = aReleaseFence;
  } else {
    android::sp<android::Fence> mergedFence = android::Fence::merge(
                  android::String8::format("TextureHostOGL"),
                  mReleaseFence, aReleaseFence);
    if (!mergedFence.get()) {
      
      
      
      mReleaseFence = aReleaseFence;
      return false;
    }
    mReleaseFence = mergedFence;
  }
  return true;
}

android::sp<android::Fence>
TextureHostOGL::GetAndResetReleaseFence()
{
  
  mPrevReleaseFence = mReleaseFence;
  
  mReleaseFence = android::Fence::NO_FENCE;
  return mPrevReleaseFence;
}

void
TextureHostOGL::SetAcquireFence(const android::sp<android::Fence>& aAcquireFence)
{
  mAcquireFence = aAcquireFence;
}

android::sp<android::Fence>
TextureHostOGL::GetAndResetAcquireFence()
{
  android::sp<android::Fence> fence = mAcquireFence;
  
  mAcquireFence = android::Fence::NO_FENCE;
  return fence;
}

void
TextureHostOGL::WaitAcquireFenceSyncComplete()
{
  if (!mAcquireFence.get() || !mAcquireFence->isValid()) {
    return;
  }

  int fenceFd = mAcquireFence->dup();
  if (fenceFd == -1) {
    NS_WARNING("failed to dup fence fd");
    return;
  }

  EGLint attribs[] = {
              LOCAL_EGL_SYNC_NATIVE_FENCE_FD_ANDROID, fenceFd,
              LOCAL_EGL_NONE
          };

  EGLSync sync = sEGLLibrary.fCreateSync(EGL_DISPLAY(),
                                         LOCAL_EGL_SYNC_NATIVE_FENCE_ANDROID,
                                         attribs);
  if (!sync) {
    NS_WARNING("failed to create native fence sync");
    return;
  }

  
  
  
  EGLint status = sEGLLibrary.fClientWaitSync(EGL_DISPLAY(),
                                              sync,
                                              0,
                                              400000000 );
  if (status != LOCAL_EGL_CONDITION_SATISFIED) {
    NS_ERROR("failed to wait native fence sync");
  }
  MOZ_ALWAYS_TRUE( sEGLLibrary.fDestroySync(EGL_DISPLAY(), sync) );
  mAcquireFence = nullptr;
}

#endif

bool
TextureImageTextureSourceOGL::Update(gfx::DataSourceSurface* aSurface,
                                     nsIntRegion* aDestRegion,
                                     gfx::IntPoint* aSrcOffset)
{
  GLContext *gl = mCompositor->gl();
  MOZ_ASSERT(gl);
  if (!gl) {
    NS_WARNING("trying to update TextureImageTextureSourceOGL without a GLContext");
    return false;
  }
  if (!aSurface) {
    gfxCriticalError() << "Invalid surface for OGL update";
    return false;
  }
  MOZ_ASSERT(aSurface);

  IntSize size = aSurface->GetSize();
  if (!mTexImage ||
      (mTexImage->GetSize() != size && !aSrcOffset) ||
      mTexImage->GetContentType() != gfx::ContentForFormat(aSurface->GetFormat())) {
    if (mFlags & TextureFlags::DISALLOW_BIGIMAGE) {
      GLint maxTextureSize;
      gl->fGetIntegerv(LOCAL_GL_MAX_TEXTURE_SIZE, &maxTextureSize);
      if (size.width > maxTextureSize || size.height > maxTextureSize) {
        NS_WARNING("Texture exceeds maximum texture size, refusing upload");
        return false;
      }
      
      
      
      mTexImage = CreateBasicTextureImage(gl, size,
                                          gfx::ContentForFormat(aSurface->GetFormat()),
                                          LOCAL_GL_CLAMP_TO_EDGE,
                                          FlagsToGLFlags(mFlags),
                                          SurfaceFormatToImageFormat(aSurface->GetFormat()));
    } else {
      
      
      
      
      mTexImage = CreateTextureImage(gl,
                                     size,
                                     gfx::ContentForFormat(aSurface->GetFormat()),
                                     LOCAL_GL_CLAMP_TO_EDGE,
                                     FlagsToGLFlags(mFlags),
                                     SurfaceFormatToImageFormat(aSurface->GetFormat()));
    }
    ClearCachedFilter();

    if (aDestRegion &&
        !aSrcOffset &&
        !aDestRegion->IsEqual(gfx::IntRect(0, 0, size.width, size.height))) {
      
      
      
      
      mTexImage->Resize(size);
    }
  }

  mTexImage->UpdateFromDataSource(aSurface, aDestRegion, aSrcOffset);

  if (mTexImage->InUpdate()) {
    mTexImage->EndUpdate();
  }
  return true;
}

void
TextureImageTextureSourceOGL::EnsureBuffer(const nsIntSize& aSize,
                                           gfxContentType aContentType)
{
  if (!mTexImage ||
      mTexImage->GetSize() != aSize ||
      mTexImage->GetContentType() != aContentType) {
    mTexImage = CreateTextureImage(mCompositor->gl(),
                                   aSize,
                                   aContentType,
                                   LOCAL_GL_CLAMP_TO_EDGE,
                                   FlagsToGLFlags(mFlags));
  }
  mTexImage->Resize(aSize);
}

void
TextureImageTextureSourceOGL::CopyTo(const gfx::IntRect& aSourceRect,
                                     DataTextureSource *aDest,
                                     const gfx::IntRect& aDestRect)
{
  MOZ_ASSERT(aDest->AsSourceOGL(), "Incompatible destination type!");
  TextureImageTextureSourceOGL *dest =
    aDest->AsSourceOGL()->AsTextureImageTextureSource();
  MOZ_ASSERT(dest, "Incompatible destination type!");

  mCompositor->BlitTextureImageHelper()->BlitTextureImage(mTexImage, aSourceRect,
                                                  dest->mTexImage, aDestRect);
  dest->mTexImage->MarkValid();
}

void
TextureImageTextureSourceOGL::SetCompositor(Compositor* aCompositor)
{
  MOZ_ASSERT(aCompositor);
  CompositorOGL* glCompositor = static_cast<CompositorOGL*>(aCompositor);

  if (!glCompositor || (mCompositor != glCompositor)) {
    DeallocateDeviceData();
    mCompositor = glCompositor;
  }
}

gfx::IntSize
TextureImageTextureSourceOGL::GetSize() const
{
  if (mTexImage) {
    if (mIterating) {
      return mTexImage->GetTileRect().Size();
    }
    return mTexImage->GetSize();
  }
  NS_WARNING("Trying to query the size of an empty TextureSource.");
  return gfx::IntSize(0, 0);
}

gfx::SurfaceFormat
TextureImageTextureSourceOGL::GetFormat() const
{
  if (mTexImage) {
    return mTexImage->GetTextureFormat();
  }
  NS_WARNING("Trying to query the format of an empty TextureSource.");
  return gfx::SurfaceFormat::UNKNOWN;
}

gfx::IntRect TextureImageTextureSourceOGL::GetTileRect()
{
  return ThebesIntRect(mTexImage->GetTileRect());
}

void
TextureImageTextureSourceOGL::BindTexture(GLenum aTextureUnit, gfx::Filter aFilter)
{
  MOZ_ASSERT(mTexImage,
    "Trying to bind a TextureSource that does not have an underlying GL texture.");
  mTexImage->BindTexture(aTextureUnit);
  SetFilter(mCompositor->gl(), aFilter);
}




GLTextureSource::GLTextureSource(CompositorOGL* aCompositor,
                                 GLuint aTextureHandle,
                                 GLenum aTarget,
                                 gfx::IntSize aSize,
                                 gfx::SurfaceFormat aFormat,
                                 bool aExternallyOwned)
  : mCompositor(aCompositor)
  , mTextureHandle(aTextureHandle)
  , mTextureTarget(aTarget)
  , mSize(aSize)
  , mFormat(aFormat)
  , mExternallyOwned(aExternallyOwned)
{
  MOZ_COUNT_CTOR(GLTextureSource);
}

GLTextureSource::~GLTextureSource()
{
  MOZ_COUNT_DTOR(GLTextureSource);
  if (!mExternallyOwned) {
    DeleteTextureHandle();
  }
}

void
GLTextureSource::DeallocateDeviceData()
{
  if (!mExternallyOwned) {
    DeleteTextureHandle();
  }
}

void
GLTextureSource::DeleteTextureHandle()
{
  if (mTextureHandle != 0 && gl() && gl()->MakeCurrent()) {
    gl()->fDeleteTextures(1, &mTextureHandle);
  }
  mTextureHandle = 0;
}

void
GLTextureSource::BindTexture(GLenum aTextureUnit, gfx::Filter aFilter)
{
  MOZ_ASSERT(gl());
  MOZ_ASSERT(mTextureHandle != 0);
  if (!gl()) {
    return;
  }
  gl()->fActiveTexture(aTextureUnit);
  gl()->fBindTexture(mTextureTarget, mTextureHandle);
  ApplyFilterToBoundTexture(gl(), aFilter, mTextureTarget);
}

void
GLTextureSource::SetCompositor(Compositor* aCompositor)
{
  MOZ_ASSERT(aCompositor);
  mCompositor = static_cast<CompositorOGL*>(aCompositor);
}

bool
GLTextureSource::IsValid() const
{
  return !!gl() && mTextureHandle != 0;
}

gl::GLContext*
GLTextureSource::gl() const
{
  return mCompositor ? mCompositor->gl() : nullptr;
}





#ifdef MOZ_WIDGET_ANDROID

SurfaceTextureSource::SurfaceTextureSource(CompositorOGL* aCompositor,
                                           AndroidSurfaceTexture* aSurfTex,
                                           gfx::SurfaceFormat aFormat,
                                           GLenum aTarget,
                                           GLenum aWrapMode,
                                           gfx::IntSize aSize)
  : mCompositor(aCompositor)
  , mSurfTex(aSurfTex)
  , mFormat(aFormat)
  , mTextureTarget(aTarget)
  , mWrapMode(aWrapMode)
  , mSize(aSize)
{
}

void
SurfaceTextureSource::BindTexture(GLenum aTextureUnit, gfx::Filter aFilter)
{
  if (!gl()) {
    NS_WARNING("Trying to bind a texture without a GLContext");
    return;
  }

  gl()->fActiveTexture(aTextureUnit);

  
  
  gl()->FlushErrors();

  mSurfTex->UpdateTexImage();

  ApplyFilterToBoundTexture(gl(), aFilter, mTextureTarget);
}

void
SurfaceTextureSource::SetCompositor(Compositor* aCompositor)
{
  MOZ_ASSERT(aCompositor);
  if (mCompositor != aCompositor) {
    DeallocateDeviceData();
  }

  mCompositor = static_cast<CompositorOGL*>(aCompositor);
}

bool
SurfaceTextureSource::IsValid() const
{
  return !!gl();
}

gl::GLContext*
SurfaceTextureSource::gl() const
{
  return mCompositor ? mCompositor->gl() : nullptr;
}

gfx::Matrix4x4
SurfaceTextureSource::GetTextureTransform()
{
  gfx::Matrix4x4 ret;
  mSurfTex->GetTransformMatrix(ret);

  return ret;
}



SurfaceTextureHost::SurfaceTextureHost(TextureFlags aFlags,
                                       AndroidSurfaceTexture* aSurfTex,
                                       gfx::IntSize aSize)
  : TextureHost(aFlags)
  , mSurfTex(aSurfTex)
  , mSize(aSize)
  , mCompositor(nullptr)
{
}

SurfaceTextureHost::~SurfaceTextureHost()
{
}

gl::GLContext*
SurfaceTextureHost::gl() const
{
  return mCompositor ? mCompositor->gl() : nullptr;
}

bool
SurfaceTextureHost::Lock()
{
  if (!mCompositor) {
    return false;
  }

  if (!mTextureSource) {
    gfx::SurfaceFormat format = gfx::SurfaceFormat::R8G8B8A8;
    GLenum target = LOCAL_GL_TEXTURE_EXTERNAL;
    GLenum wrapMode = LOCAL_GL_CLAMP_TO_EDGE;
    mTextureSource = new SurfaceTextureSource(mCompositor,
                                              mSurfTex,
                                              format,
                                              target,
                                              wrapMode,
                                              mSize);
  }

  return NS_SUCCEEDED(mSurfTex->Attach(gl()));
}

void
SurfaceTextureHost::Unlock()
{
  mSurfTex->Detach();
}

void
SurfaceTextureHost::SetCompositor(Compositor* aCompositor)
{
  MOZ_ASSERT(aCompositor);
  CompositorOGL* glCompositor = static_cast<CompositorOGL*>(aCompositor);
  mCompositor = glCompositor;
  if (mTextureSource) {
    mTextureSource->SetCompositor(glCompositor);
  }
}

gfx::SurfaceFormat
SurfaceTextureHost::GetFormat() const
{
  MOZ_ASSERT(mTextureSource);
  return mTextureSource->GetFormat();
}

#endif 





EGLImageTextureSource::EGLImageTextureSource(CompositorOGL* aCompositor,
                                             EGLImage aImage,
                                             gfx::SurfaceFormat aFormat,
                                             GLenum aTarget,
                                             GLenum aWrapMode,
                                             gfx::IntSize aSize)
  : mCompositor(aCompositor)
  , mImage(aImage)
  , mFormat(aFormat)
  , mTextureTarget(aTarget)
  , mWrapMode(aWrapMode)
  , mSize(aSize)
{
}

void
EGLImageTextureSource::BindTexture(GLenum aTextureUnit, gfx::Filter aFilter)
{
  if (!gl()) {
    NS_WARNING("Trying to bind a texture without a GLContext");
    return;
  }

  MOZ_ASSERT(DoesEGLContextSupportSharingWithEGLImage(gl()),
             "EGLImage not supported or disabled in runtime");

  GLuint tex = mCompositor->GetTemporaryTexture(GetTextureTarget(), aTextureUnit);

  gl()->fActiveTexture(aTextureUnit);
  gl()->fBindTexture(mTextureTarget, tex);

  MOZ_ASSERT(mTextureTarget == LOCAL_GL_TEXTURE_2D);
  gl()->fEGLImageTargetTexture2D(LOCAL_GL_TEXTURE_2D, mImage);

  ApplyFilterToBoundTexture(gl(), aFilter, mTextureTarget);
}

void
EGLImageTextureSource::SetCompositor(Compositor* aCompositor)
{
  MOZ_ASSERT(aCompositor);
  mCompositor = static_cast<CompositorOGL*>(aCompositor);
}

bool
EGLImageTextureSource::IsValid() const
{
  return !!gl();
}

gl::GLContext*
EGLImageTextureSource::gl() const
{
  return mCompositor ? mCompositor->gl() : nullptr;
}

gfx::Matrix4x4
EGLImageTextureSource::GetTextureTransform()
{
  gfx::Matrix4x4 ret;
  return ret;
}



EGLImageTextureHost::EGLImageTextureHost(TextureFlags aFlags,
                                         EGLImage aImage,
                                         EGLSync aSync,
                                         gfx::IntSize aSize)
  : TextureHost(aFlags)
  , mImage(aImage)
  , mSync(aSync)
  , mSize(aSize)
  , mCompositor(nullptr)
{
}

EGLImageTextureHost::~EGLImageTextureHost()
{
}

gl::GLContext*
EGLImageTextureHost::gl() const
{
  return mCompositor ? mCompositor->gl() : nullptr;
}

bool
EGLImageTextureHost::Lock()
{
  if (!mCompositor) {
    return false;
  }

  EGLint status = sEGLLibrary.fClientWaitSync(EGL_DISPLAY(), mSync, 0, LOCAL_EGL_FOREVER);
  if (status != LOCAL_EGL_CONDITION_SATISFIED) {
    return false;
  }

  if (!mTextureSource) {
    gfx::SurfaceFormat format = gfx::SurfaceFormat::R8G8B8A8;
    GLenum target = LOCAL_GL_TEXTURE_2D;
    GLenum wrapMode = LOCAL_GL_CLAMP_TO_EDGE;
    mTextureSource = new EGLImageTextureSource(mCompositor,
                                               mImage,
                                               format,
                                               target,
                                               wrapMode,
                                               mSize);
  }

  return true;
}

void
EGLImageTextureHost::Unlock()
{
}

void
EGLImageTextureHost::SetCompositor(Compositor* aCompositor)
{
  MOZ_ASSERT(aCompositor);
  CompositorOGL* glCompositor = static_cast<CompositorOGL*>(aCompositor);
  mCompositor = glCompositor;
  if (mTextureSource) {
    mTextureSource->SetCompositor(glCompositor);
  }
}

gfx::SurfaceFormat
EGLImageTextureHost::GetFormat() const
{
  MOZ_ASSERT(mTextureSource);
  return mTextureSource->GetFormat();
}

} 
} 
