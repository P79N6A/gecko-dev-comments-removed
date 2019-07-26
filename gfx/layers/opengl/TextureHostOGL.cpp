




#include "TextureHostOGL.h"
#include "GLContext.h"                  
#include "GLSharedHandleHelpers.h"
#include "GLContextUtils.h"             
#include "SharedSurface.h"              
#include "SharedSurfaceEGL.h"           
#include "SharedSurfaceGL.h"            
#include "SurfaceStream.h"              
#include "SurfaceTypes.h"               
#include "TiledLayerBuffer.h"           
#include "gfx2DGlue.h"                  
#include "gfxImageSurface.h"            
#include "gfxPoint.h"                   
#include "gfxReusableSurfaceWrapper.h"  
#include "ipc/AutoOpenSurface.h"        
#include "mozilla/gfx/2D.h"             
#include "mozilla/gfx/BaseSize.h"       
#include "mozilla/layers/CompositorOGL.h"  
#ifdef MOZ_WIDGET_GONK
# include "GrallocImages.h"  
#endif
#include "mozilla/layers/ISurfaceAllocator.h"
#include "mozilla/layers/YCbCrImageDataSerializer.h"
#include "mozilla/layers/GrallocTextureHost.h"
#include "nsPoint.h"                    
#include "nsRegion.h"                   
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

TemporaryRef<CompositableBackendSpecificData>
CreateCompositableBackendSpecificDataOGL()
{
#ifdef MOZ_WIDGET_GONK
  return new CompositableDataGonkOGL();
#else
  return nullptr;
#endif
}

TemporaryRef<DeprecatedTextureHost>
CreateDeprecatedTextureHostOGL(SurfaceDescriptorType aDescriptorType,
                     uint32_t aDeprecatedTextureHostFlags,
                     uint32_t aTextureFlags)
{
  RefPtr<DeprecatedTextureHost> result = nullptr;

  if (aDescriptorType == SurfaceDescriptor::TYCbCrImage) {
    result = new YCbCrDeprecatedTextureHostOGL();
  } else if (aDescriptorType == SurfaceDescriptor::TSurfaceStreamDescriptor) {
    result = new SurfaceStreamHostOGL();
  } else if (aDescriptorType == SurfaceDescriptor::TSharedTextureDescriptor) {
    result = new SharedDeprecatedTextureHostOGL();
#ifdef MOZ_WIDGET_GONK
  } else if (aDescriptorType == SurfaceDescriptor::TSurfaceDescriptorGralloc) {
    result = new GrallocDeprecatedTextureHostOGL();
#endif
  } else if (aDeprecatedTextureHostFlags & TEXTURE_HOST_TILED) {
    result = new TiledDeprecatedTextureHostOGL();
  } else {
    result = new TextureImageDeprecatedTextureHostOGL();
  }

  NS_ASSERTION(result, "Result should have been created.");

  result->SetFlags(aTextureFlags);
  return result.forget();
}


TemporaryRef<TextureHost>
CreateTextureHostOGL(uint64_t aID,
                     const SurfaceDescriptor& aDesc,
                     ISurfaceAllocator* aDeallocator,
                     TextureFlags aFlags)
{
  RefPtr<TextureHost> result;
  switch (aDesc.type()) {
    case SurfaceDescriptor::TSurfaceDescriptorShmem:
    case SurfaceDescriptor::TSurfaceDescriptorMemory: {
      result = CreateBackendIndependentTextureHost(aID, aDesc,
                                                   aDeallocator, aFlags);
      break;
    }
    case SurfaceDescriptor::TSharedTextureDescriptor: {
      const SharedTextureDescriptor& desc = aDesc.get_SharedTextureDescriptor();
      result = new SharedTextureHostOGL(aID, aFlags,
                                        desc.shareType(),
                                        desc.handle(),
                                        gfx::ToIntSize(desc.size()),
                                        desc.inverted());
      break;
    }
#ifdef XP_MACOSX
    case SurfaceDescriptor::TSurfaceDescriptorMacIOSurface: {
      const SurfaceDescriptorMacIOSurface& desc =
        aDesc.get_SurfaceDescriptorMacIOSurface();
      result = new MacIOSurfaceTextureHostOGL(aID, aFlags, desc);
      break;
    }
#endif
#ifdef MOZ_WIDGET_GONK
    case SurfaceDescriptor::TNewSurfaceDescriptorGralloc: {
      const NewSurfaceDescriptorGralloc& desc =
        aDesc.get_NewSurfaceDescriptorGralloc();
      result = new GrallocTextureHostOGL(aID, aFlags, desc);
      break;
    }
#endif
    default: return nullptr;
  }
  return result.forget();
}

static void
MakeTextureIfNeeded(gl::GLContext* gl, GLenum aTarget, GLuint& aTexture)
{
  if (aTexture != 0)
    return;

  GLenum target = aTarget;
  
  
  if (target == LOCAL_GL_TEXTURE_EXTERNAL) {
    target = LOCAL_GL_TEXTURE_2D;
  }

  gl->fGenTextures(1, &aTexture);

  gl->fBindTexture(target, aTexture);

  gl->fTexParameteri(target, LOCAL_GL_TEXTURE_MIN_FILTER, LOCAL_GL_LINEAR);
  gl->fTexParameteri(target, LOCAL_GL_TEXTURE_MAG_FILTER, LOCAL_GL_LINEAR);
  gl->fTexParameteri(target, LOCAL_GL_TEXTURE_WRAP_S, LOCAL_GL_CLAMP_TO_EDGE);
  gl->fTexParameteri(target, LOCAL_GL_TEXTURE_WRAP_T, LOCAL_GL_CLAMP_TO_EDGE);
}

static gl::TextureImage::Flags
FlagsToGLFlags(TextureFlags aFlags)
{
  uint32_t result = TextureImage::NoFlags;

  if (aFlags & TEXTURE_USE_NEAREST_FILTER)
    result |= TextureImage::UseNearestFilter;
  if (aFlags & TEXTURE_NEEDS_Y_FLIP)
    result |= TextureImage::NeedsYFlip;
  if (aFlags & TEXTURE_DISALLOW_BIGIMAGE)
    result |= TextureImage::DisallowBigImage;

  return static_cast<gl::TextureImage::Flags>(result);
}

GLenum
WrapMode(gl::GLContext *aGl, bool aAllowRepeat)
{
  if (aAllowRepeat &&
      (aGl->IsExtensionSupported(GLContext::ARB_texture_non_power_of_two) ||
       aGl->IsExtensionSupported(GLContext::OES_texture_npot))) {
    return LOCAL_GL_REPEAT;
  }
  return LOCAL_GL_CLAMP_TO_EDGE;
}

CompositableDataGonkOGL::CompositableDataGonkOGL()
 : mTexture(0)
{
}
CompositableDataGonkOGL::~CompositableDataGonkOGL()
{
  DeleteTextureIfPresent();
}

gl::GLContext*
CompositableDataGonkOGL::gl() const
{
  return mCompositor ? mCompositor->gl() : nullptr;
}

void CompositableDataGonkOGL::SetCompositor(Compositor* aCompositor)
{
  mCompositor = static_cast<CompositorOGL*>(aCompositor);
}

void CompositableDataGonkOGL::ClearData()
{
  DeleteTextureIfPresent();
}

GLuint CompositableDataGonkOGL::GetTexture()
{
  if (!mTexture) {
    if (gl()->MakeCurrent()) {
      gl()->fGenTextures(1, &mTexture);
    }
  }
  return mTexture;
}

void
CompositableDataGonkOGL::DeleteTextureIfPresent()
{
  if (mTexture) {
    if (gl()->MakeCurrent()) {
      gl()->fDeleteTextures(1, &mTexture);
    }
  }
}

bool
TextureImageTextureSourceOGL::Update(gfx::DataSourceSurface* aSurface,
                                     TextureFlags aFlags,
                                     nsIntRegion* aDestRegion,
                                     gfx::IntPoint* aSrcOffset)
{
  MOZ_ASSERT(mGL);
  if (!mGL) {
    NS_WARNING("trying to update TextureImageTextureSourceOGL without a GLContext");
    return false;
  }
  MOZ_ASSERT(aSurface);

  nsIntSize size = ThebesIntSize(aSurface->GetSize());
  if (!mTexImage ||
      mTexImage->GetSize() != size ||
      mTexImage->GetContentType() != gfx::ContentForFormat(aSurface->GetFormat())) {
    if (mAllowBigImage) {
      
      
      
      
      mTexImage = CreateTextureImage(mGL,
                                     size,
                                     gfx::ContentForFormat(aSurface->GetFormat()),
                                     WrapMode(mGL, aFlags & TEXTURE_ALLOW_REPEAT),
                                     FlagsToGLFlags(aFlags));
    } else {
      mTexImage = CreateBasicTextureImage(mGL,
                                          size,
                                          gfx::ContentForFormat(aSurface->GetFormat()),
                                          WrapMode(mGL, aFlags & TEXTURE_ALLOW_REPEAT),
                                          FlagsToGLFlags(aFlags));
    }
  }

  mTexImage->UpdateFromDataSource(aSurface, aDestRegion, aSrcOffset);

  if (mTexImage->InUpdate()) {
    mTexImage->EndUpdate();
  }
  return true;
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
  MOZ_ASSERT(mTexImage);
  return mTexImage->GetTextureFormat();
}

nsIntRect TextureImageTextureSourceOGL::GetTileRect()
{
  return ThebesIntRect(mTexImage->GetTileRect());
}

void
TextureImageTextureSourceOGL::BindTexture(GLenum aTextureUnit)
{
  MOZ_ASSERT(mTexImage,
    "Trying to bind a TextureSource that does not have an underlying GL texture.");
  mTexImage->BindTexture(aTextureUnit);
}

SharedTextureSourceOGL::SharedTextureSourceOGL(CompositorOGL* aCompositor,
                                               gl::SharedTextureHandle aHandle,
                                               gfx::SurfaceFormat aFormat,
                                               GLenum aTarget,
                                               GLenum aWrapMode,
                                               SharedTextureShareType aShareType,
                                               gfx::IntSize aSize)
  : mSize(aSize)
  , mCompositor(aCompositor)
  , mSharedHandle(aHandle)
  , mFormat(aFormat)
  , mShareType(aShareType)
  , mTextureTarget(aTarget)
  , mWrapMode(aWrapMode)
{}

void
SharedTextureSourceOGL::BindTexture(GLenum aTextureUnit)
{
  if (!gl()) {
    NS_WARNING("Trying to bind a texture without a GLContext");
    return;
  }
  GLuint tex = mCompositor->GetTemporaryTexture(aTextureUnit);

  gl()->fActiveTexture(aTextureUnit);
  gl()->fBindTexture(mTextureTarget, tex);
  if (!AttachSharedHandle(gl(), mShareType, mSharedHandle)) {
    NS_ERROR("Failed to bind shared texture handle");
    return;
  }
  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
}

void
SharedTextureSourceOGL::DetachSharedHandle()
{
  if (!gl()) {
    return;
  }
  gl::DetachSharedHandle(gl(), mShareType, mSharedHandle);
}

void
SharedTextureSourceOGL::SetCompositor(CompositorOGL* aCompositor)
{
  mCompositor = aCompositor;
}

bool
SharedTextureSourceOGL::IsValid() const
{
  return !!gl();
}

gl::GLContext*
SharedTextureSourceOGL::gl() const
{
  return mCompositor ? mCompositor->gl() : nullptr;
}

gfx3DMatrix
SharedTextureSourceOGL::GetTextureTransform()
{
  SharedHandleDetails handleDetails;
  if (!GetSharedHandleDetails(gl(), mShareType, mSharedHandle, handleDetails)) {
    NS_WARNING("Could not get shared handle details");
    return gfx3DMatrix();
  }

  return handleDetails.mTextureTransform;
}

SharedTextureHostOGL::SharedTextureHostOGL(uint64_t aID,
                                           TextureFlags aFlags,
                                           gl::SharedTextureShareType aShareType,
                                           gl::SharedTextureHandle aSharedHandle,
                                           gfx::IntSize aSize,
                                           bool inverted)
  : TextureHost(aID, aFlags)
  , mSize(aSize)
  , mCompositor(nullptr)
  , mSharedHandle(aSharedHandle)
  , mShareType(aShareType)
{
}

SharedTextureHostOGL::~SharedTextureHostOGL()
{
  
  
}

gl::GLContext*
SharedTextureHostOGL::gl() const
{
  return mCompositor ? mCompositor->gl() : nullptr;
}

bool
SharedTextureHostOGL::Lock()
{
  if (!mCompositor) {
    return false;
  }

  if (!mTextureSource) {
    
    
    SharedHandleDetails handleDetails;
    if (!GetSharedHandleDetails(gl(), mShareType, mSharedHandle, handleDetails)) {
      NS_WARNING("Could not get shared handle details");
      return false;
    }

    GLenum wrapMode = LOCAL_GL_CLAMP_TO_EDGE;
    mTextureSource = new SharedTextureSourceOGL(mCompositor,
                                                mSharedHandle,
                                                handleDetails.mTextureFormat,
                                                handleDetails.mTarget,
                                                wrapMode,
                                                mShareType,
                                                mSize);
  }
  return true;
}

void
SharedTextureHostOGL::Unlock()
{
  if (!mTextureSource) {
    return;
  }
  mTextureSource->DetachSharedHandle();
}

void
SharedTextureHostOGL::SetCompositor(Compositor* aCompositor)
{
  CompositorOGL* glCompositor = static_cast<CompositorOGL*>(aCompositor);
  mCompositor = glCompositor;
  if (mTextureSource) {
    mTextureSource->SetCompositor(glCompositor);
  }
}

gfx::SurfaceFormat
SharedTextureHostOGL::GetFormat() const
{
  MOZ_ASSERT(mTextureSource);
  return mTextureSource->GetFormat();
}

TextureImageDeprecatedTextureHostOGL::~TextureImageDeprecatedTextureHostOGL()
{
  MOZ_COUNT_DTOR(TextureImageDeprecatedTextureHostOGL);
  if (mTexture && mTexture->InUpdate()) {
    mTexture->EndUpdate();
  }
}

gfx::IntSize
TextureImageDeprecatedTextureHostOGL::GetSize() const
{
  if (mTexture) {
    if (mIterating) {
      return mTexture->GetTileRect().Size();
    }
    return mTexture->GetSize();
  }
  return gfx::IntSize(0, 0);
}

nsIntRect TextureImageDeprecatedTextureHostOGL::GetTileRect()
{
  return ThebesIntRect(mTexture->GetTileRect());
}

void
TextureImageDeprecatedTextureHostOGL::SetCompositor(Compositor* aCompositor)
{
  CompositorOGL* glCompositor = static_cast<CompositorOGL*>(aCompositor);
  GLContext* newGL = glCompositor ? glCompositor->gl() : nullptr;
  if (mGL != newGL) {
    mGL = newGL;
    mTexture = nullptr;
    
    
    
    if (newGL && mBuffer && IsSurfaceDescriptorValid(*mBuffer)) {
      UpdateImpl(*mBuffer);
    }
  }
}

void
TextureImageDeprecatedTextureHostOGL::EnsureBuffer(const nsIntSize& aSize,
                                         gfxContentType aContentType)
{
  if (!mTexture ||
      mTexture->GetSize() != aSize ||
      mTexture->GetContentType() != aContentType) {
    mTexture = CreateTextureImage(mGL,
                                  aSize,
                                  aContentType,
                                  WrapMode(mGL, mFlags & TEXTURE_ALLOW_REPEAT),
                                  FlagsToGLFlags(mFlags));
  }
  mTexture->Resize(aSize);
}

void
TextureImageDeprecatedTextureHostOGL::CopyTo(const nsIntRect& aSourceRect,
                                   DeprecatedTextureHost *aDest,
                                   const nsIntRect& aDestRect)
{
  MOZ_ASSERT(aDest->AsSourceOGL(), "Incompatible destination type!");
  TextureImageDeprecatedTextureHostOGL *dest =
    aDest->AsSourceOGL()->AsTextureImageDeprecatedTextureHost();
  MOZ_ASSERT(dest, "Incompatible destination type!");

  mGL->BlitTextureImageHelper()->BlitTextureImage(mTexture, aSourceRect,
                                                  dest->mTexture, aDestRect);
  dest->mTexture->MarkValid();
}

void
TextureImageDeprecatedTextureHostOGL::UpdateImpl(const SurfaceDescriptor& aImage,
                                       nsIntRegion* aRegion,
                                       nsIntPoint* aOffset)
{
  if (!mGL) {
    NS_WARNING("trying to update TextureImageDeprecatedTextureHostOGL without a compositor?");
    return;
  }

#ifdef MOZ_WIDGET_GONK
  if (mCompositableBackendData) {
    
    
    
    
    
    
    
    static_cast<CompositableDataGonkOGL*>(mCompositableBackendData.get())->DeleteTextureIfPresent();
  }
#endif

  AutoOpenSurface surf(OPEN_READ_ONLY, aImage);
  nsIntSize size = surf.Size();
  TextureImage::ImageFormat format = surf.ImageFormat();

  if (!mTexture ||
      (mTexture->GetSize() != size && !aOffset) ||
      mTexture->GetContentType() != surf.ContentType() ||
      (mTexture->GetImageFormat() != format &&
       mTexture->GetImageFormat() != gfxImageFormatUnknown)) {

    mTexture = CreateTextureImage(mGL,
                                  size,
                                  surf.ContentType(),
                                  WrapMode(mGL, mFlags & TEXTURE_ALLOW_REPEAT),
                                  FlagsToGLFlags(mFlags),
                                  format);
  }

  
  nsIntRegion updateRegion;

  if (!aRegion) {
    updateRegion = nsIntRegion(nsIntRect(0, 0, size.width, size.height));
  } else {
    updateRegion = *aRegion;
  }
  nsIntPoint offset;
  if (aOffset) {
    offset = *aOffset;
  }
  mTexture->DirectUpdate(surf.Get(), updateRegion, offset);
  mFormat = mTexture->GetTextureFormat();

  if (mTexture->InUpdate()) {
    mTexture->EndUpdate();
  }
}

bool
TextureImageDeprecatedTextureHostOGL::Lock()
{
  if (!mTexture) {
    NS_WARNING("TextureImageDeprecatedTextureHost to be composited without texture");
    return false;
  }

  mFormat = mTexture->GetTextureFormat();

  return true;
}

void
SharedDeprecatedTextureHostOGL::SetCompositor(Compositor* aCompositor)
{
  CompositorOGL* glCompositor = static_cast<CompositorOGL*>(aCompositor);
  if (mGL && !glCompositor) {
    DeleteTextures();
  }
  mGL = glCompositor ? glCompositor->gl() : nullptr;
}

void
SharedDeprecatedTextureHostOGL::DeleteTextures()
{
  MOZ_ASSERT(mGL);
  if (!mGL->MakeCurrent()) {
    mSharedHandle = 0;
    mTextureHandle = 0;
    return;
  }
  if (mSharedHandle) {
    ReleaseSharedHandle(mGL, mShareType, mSharedHandle);
    mSharedHandle = 0;
  }
  if (mTextureHandle) {
    mGL->fDeleteTextures(1, &mTextureHandle);
    mTextureHandle = 0;
  }
}

void
SharedDeprecatedTextureHostOGL::UpdateImpl(const SurfaceDescriptor& aImage,
                                 nsIntRegion* aRegion,
                                 nsIntPoint* aOffset)
{
  SwapTexturesImpl(aImage, aRegion);
}

void
SharedDeprecatedTextureHostOGL::SwapTexturesImpl(const SurfaceDescriptor& aImage,
                                       nsIntRegion* aRegion)
{
  NS_ASSERTION(aImage.type() == SurfaceDescriptor::TSharedTextureDescriptor,
              "Invalid descriptor");

  SharedTextureDescriptor texture = aImage.get_SharedTextureDescriptor();

  SharedTextureHandle newHandle = texture.handle();
  nsIntSize size = texture.size();
  mSize = gfx::IntSize(size.width, size.height);
  if (texture.inverted()) {
    mFlags |= TEXTURE_NEEDS_Y_FLIP;
  }

  if (mSharedHandle && mSharedHandle != newHandle) {
    ReleaseSharedHandle(mGL, mShareType, mSharedHandle);
  }

  mShareType = texture.shareType();
  mSharedHandle = newHandle;

  SharedHandleDetails handleDetails;
  if (mSharedHandle && GetSharedHandleDetails(mGL, mShareType, mSharedHandle, handleDetails)) {
    mTextureTarget = handleDetails.mTarget;
    mFormat = handleDetails.mTextureFormat;
  }
}

bool
SharedDeprecatedTextureHostOGL::Lock()
{
  MakeTextureIfNeeded(mGL, mTextureTarget, mTextureHandle);

  mGL->fActiveTexture(LOCAL_GL_TEXTURE0);
  mGL->fBindTexture(mTextureTarget, mTextureHandle);
  if (!AttachSharedHandle(mGL, mShareType, mSharedHandle)) {
    NS_ERROR("Failed to bind shared texture handle");
    return false;
  }

  return true;
}

void
SharedDeprecatedTextureHostOGL::Unlock()
{
  DetachSharedHandle(mGL, mShareType, mSharedHandle);
  mGL->fBindTexture(LOCAL_GL_TEXTURE_2D, 0);
}


gfx3DMatrix
SharedDeprecatedTextureHostOGL::GetTextureTransform()
{
  SharedHandleDetails handleDetails;
  
  
  if (mSharedHandle) {
    GetSharedHandleDetails(mGL, mShareType, mSharedHandle, handleDetails);
  }
  return handleDetails.mTextureTransform;
}


void
SurfaceStreamHostOGL::SetCompositor(Compositor* aCompositor)
{
  CompositorOGL* glCompositor = static_cast<CompositorOGL*>(aCompositor);
  if (mGL && !glCompositor) {
    DeleteTextures();
  }
  mGL = glCompositor ? glCompositor->gl() : nullptr;
}

void
SurfaceStreamHostOGL::DeleteTextures()
{
  if (mUploadTexture) {
    MOZ_ASSERT(mGL);
    if (mGL->MakeCurrent()) {
      mGL->fDeleteTextures(1, &mUploadTexture);
    }
    mUploadTexture = 0;
    mTextureHandle = 0;
  }
}

void
SurfaceStreamHostOGL::UpdateImpl(const SurfaceDescriptor& aImage,
                                 nsIntRegion* aRegion,
                                 nsIntPoint* aOffset)
{
  MOZ_ASSERT(aImage.type() == SurfaceDescriptor::TSurfaceStreamDescriptor,
             "Invalid descriptor");

  
  
  
  
  const SurfaceStreamDescriptor& streamDesc =
      aImage.get_SurfaceStreamDescriptor();


  mStream = SurfaceStream::FromHandle(streamDesc.handle());
  MOZ_ASSERT(mStream);
  mStreamGL = dont_AddRef(mStream->GLContext());
}

void
SurfaceStreamHostOGL::Unlock()
{
  
  mFormat = gfx::FORMAT_UNKNOWN;
}

bool
SurfaceStreamHostOGL::Lock()
{
  if (!mGL->MakeCurrent()) {
    return false;
  }

  SharedSurface* sharedSurf = mStream->SwapConsumer();
  if (!sharedSurf) {
    
    return false;
  }

  mGL->MakeCurrent();

  mSize = IntSize(sharedSurf->Size().width, sharedSurf->Size().height);

  gfxImageSurface* toUpload = nullptr;
  switch (sharedSurf->Type()) {
    case SharedSurfaceType::GLTextureShare: {
      SharedSurface_GLTexture* glTexSurf = SharedSurface_GLTexture::Cast(sharedSurf);
      glTexSurf->SetConsumerGL(mGL);
      mTextureHandle = glTexSurf->Texture();
      mTextureTarget = glTexSurf->TextureTarget();
      MOZ_ASSERT(mTextureHandle);
      mFormat = sharedSurf->HasAlpha() ? FORMAT_R8G8B8A8
                                       : FORMAT_R8G8B8X8;
      break;
    }
    case SharedSurfaceType::EGLImageShare: {
      SharedSurface_EGLImage* eglImageSurf =
          SharedSurface_EGLImage::Cast(sharedSurf);

      mTextureHandle = eglImageSurf->AcquireConsumerTexture(mGL);
      mTextureTarget = eglImageSurf->TextureTarget();
      if (!mTextureHandle) {
        toUpload = eglImageSurf->GetPixels();
        MOZ_ASSERT(toUpload);
      } else {
        mFormat = sharedSurf->HasAlpha() ? FORMAT_R8G8B8A8
                                         : FORMAT_R8G8B8X8;
      }
      break;
    }
#ifdef XP_MACOSX
    case SharedSurfaceType::IOSurface: {
      SharedSurface_IOSurface* glTexSurf = SharedSurface_IOSurface::Cast(sharedSurf);
      mTextureHandle = glTexSurf->Texture();
      mTextureTarget = glTexSurf->TextureTarget();
      MOZ_ASSERT(mTextureHandle);
      mFormat = sharedSurf->HasAlpha() ? FORMAT_R8G8B8A8
                                       : FORMAT_R8G8B8X8;
      break;
    }
#endif
    case SharedSurfaceType::Basic: {
      toUpload = SharedSurface_Basic::Cast(sharedSurf)->GetData();
      MOZ_ASSERT(toUpload);
      break;
    }
    default:
      MOZ_CRASH("Invalid SharedSurface type.");
  }

  if (toUpload) {
    
    nsIntSize size(toUpload->GetSize());
    nsIntRect rect(nsIntPoint(0,0), size);
    nsIntRegion bounds(rect);
    mFormat = mGL->UploadSurfaceToTexture(toUpload,
                                          bounds,
                                          mUploadTexture,
                                          true);
    mTextureHandle = mUploadTexture;
    mTextureTarget = LOCAL_GL_TEXTURE_2D;
  }

  MOZ_ASSERT(mTextureHandle);
  mGL->fBindTexture(mTextureTarget, mTextureHandle);
  mGL->fTexParameteri(mTextureTarget,
                      LOCAL_GL_TEXTURE_WRAP_S,
                      LOCAL_GL_CLAMP_TO_EDGE);
  mGL->fTexParameteri(mTextureTarget,
                      LOCAL_GL_TEXTURE_WRAP_T,
                      LOCAL_GL_CLAMP_TO_EDGE);
  return true;
}

void
SurfaceStreamHostOGL::BindTexture(GLenum activetex)
{
  MOZ_ASSERT(mGL);
  mGL->fActiveTexture(activetex);
  mGL->fBindTexture(mTextureTarget, mTextureHandle);
}


void
YCbCrDeprecatedTextureHostOGL::SetCompositor(Compositor* aCompositor)
{
  CompositorOGL* glCompositor = static_cast<CompositorOGL*>(aCompositor);
  GLContext* newGL = glCompositor ? glCompositor->gl() : nullptr;
  if (mGL != newGL) {
    mGL = newGL;
    mYTexture->mTexImage = nullptr;
    mCbTexture->mTexImage = nullptr;
    mCrTexture->mTexImage = nullptr;
    
    if (newGL && mBuffer && mBuffer->type() == SurfaceDescriptor::TYCbCrImage) {
      UpdateImpl(*mBuffer);
    }
  }
}

void
YCbCrDeprecatedTextureHostOGL::UpdateImpl(const SurfaceDescriptor& aImage,
                                nsIntRegion* aRegion,
                                nsIntPoint* aOffset)
{
  if (!mGL) {
    return;
  }
  NS_ASSERTION(aImage.type() == SurfaceDescriptor::TYCbCrImage, "SurfaceDescriptor mismatch");

  YCbCrImageDataDeserializer deserializer(aImage.get_YCbCrImage().data().get<uint8_t>());

  gfxIntSize gfxSize = deserializer.GetYSize();
  gfxIntSize gfxCbCrSize = deserializer.GetCbCrSize();

  if (!mYTexture->mTexImage || mYTexture->mTexImage->GetSize() != gfxSize) {
    mYTexture->mTexImage = CreateBasicTextureImage(mGL,
                                                   gfxSize,
                                                   GFX_CONTENT_ALPHA,
                                                   WrapMode(mGL, mFlags & TEXTURE_ALLOW_REPEAT),
                                                   FlagsToGLFlags(mFlags));
  }
  if (!mCbTexture->mTexImage || mCbTexture->mTexImage->GetSize() != gfxCbCrSize) {
    mCbTexture->mTexImage = CreateBasicTextureImage(mGL,
                                                    gfxCbCrSize,
                                                    GFX_CONTENT_ALPHA,
                                                    WrapMode(mGL, mFlags & TEXTURE_ALLOW_REPEAT),
                                                    FlagsToGLFlags(mFlags));
  }
  if (!mCrTexture->mTexImage || mCrTexture->mTexImage->GetSize() != gfxCbCrSize) {
    mCrTexture->mTexImage = CreateBasicTextureImage(mGL,
                                                    gfxCbCrSize,
                                                    GFX_CONTENT_ALPHA,
                                                    WrapMode(mGL, mFlags & TEXTURE_ALLOW_REPEAT),
                                                    FlagsToGLFlags(mFlags));
  }

  RefPtr<gfxImageSurface> tempY = new gfxImageSurface(deserializer.GetYData(),
                                       gfxSize, deserializer.GetYStride(),
                                       gfxImageFormatA8);
  RefPtr<gfxImageSurface> tempCb = new gfxImageSurface(deserializer.GetCbData(),
                                       gfxCbCrSize, deserializer.GetCbCrStride(),
                                       gfxImageFormatA8);
  RefPtr<gfxImageSurface> tempCr = new gfxImageSurface(deserializer.GetCrData(),
                                       gfxCbCrSize, deserializer.GetCbCrStride(),
                                       gfxImageFormatA8);

  nsIntRegion yRegion(nsIntRect(0, 0, gfxSize.width, gfxSize.height));
  nsIntRegion cbCrRegion(nsIntRect(0, 0, gfxCbCrSize.width, gfxCbCrSize.height));

  mYTexture->mTexImage->DirectUpdate(tempY, yRegion);
  mCbTexture->mTexImage->DirectUpdate(tempCb, cbCrRegion);
  mCrTexture->mTexImage->DirectUpdate(tempCr, cbCrRegion);
}

bool
YCbCrDeprecatedTextureHostOGL::Lock()
{
  return true;
}


TiledDeprecatedTextureHostOGL::~TiledDeprecatedTextureHostOGL()
{
  DeleteTextures();
}

void
TiledDeprecatedTextureHostOGL::BindTexture(GLenum aTextureUnit)
{
  mGL->fActiveTexture(aTextureUnit);
  mGL->fBindTexture(LOCAL_GL_TEXTURE_2D, mTextureHandle);
}

static void
GetFormatAndTileForImageFormat(gfxImageFormat aFormat,
                               GLenum& aOutFormat,
                               GLenum& aOutType)
{
  if (aFormat == gfxImageFormatRGB16_565) {
    aOutFormat = LOCAL_GL_RGB;
    aOutType = LOCAL_GL_UNSIGNED_SHORT_5_6_5;
  } else {
    aOutFormat = LOCAL_GL_RGBA;
    aOutType = LOCAL_GL_UNSIGNED_BYTE;
  }
}

void
TiledDeprecatedTextureHostOGL::SetCompositor(Compositor* aCompositor)
{
  CompositorOGL* glCompositor = static_cast<CompositorOGL*>(aCompositor);
  if (mGL && !glCompositor) {
    DeleteTextures();
  }
  mGL = glCompositor ? glCompositor->gl() : nullptr;
}

void
TiledDeprecatedTextureHostOGL::DeleteTextures()
{
  if (mTextureHandle) {
    if (mGL->MakeCurrent()) {
      mGL->fDeleteTextures(1, &mTextureHandle);

      gl::GfxTexturesReporter::UpdateAmount(gl::GfxTexturesReporter::MemoryFreed,
                                            mGLFormat, GetTileType(),
                                            TILEDLAYERBUFFER_TILE_SIZE);
    } else if (mGL->IsDestroyed()) {
      
      
      
      gl::GfxTexturesReporter::UpdateAmount(gl::GfxTexturesReporter::MemoryFreed,
                                            mGLFormat, GetTileType(),
                                            TILEDLAYERBUFFER_TILE_SIZE);
    }

    mTextureHandle = 0;
  }
}

void
TiledDeprecatedTextureHostOGL::Update(gfxReusableSurfaceWrapper* aReusableSurface, TextureFlags aFlags, const gfx::IntSize& aSize)
{
  mSize = aSize;
  if (!mGL->MakeCurrent()) {
    return;
  }

  if (aFlags & TEXTURE_NEW_TILE) {
    SetFlags(aFlags);
    mGL->fGenTextures(1, &mTextureHandle);
    mGL->fBindTexture(LOCAL_GL_TEXTURE_2D, mTextureHandle);
    mGL->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, LOCAL_GL_LINEAR);
    mGL->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, LOCAL_GL_LINEAR);
    mGL->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S, LOCAL_GL_CLAMP_TO_EDGE);
    mGL->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T, LOCAL_GL_CLAMP_TO_EDGE);
  } else {
    mGL->fBindTexture(LOCAL_GL_TEXTURE_2D, mTextureHandle);
    
    
    gl::GfxTexturesReporter::UpdateAmount(gl::GfxTexturesReporter::MemoryFreed,
                                          mGLFormat, GetTileType(),
                                          TILEDLAYERBUFFER_TILE_SIZE);
  }

  GLenum type;
  GetFormatAndTileForImageFormat(aReusableSurface->Format(), mGLFormat, type);

  const unsigned char* buf = aReusableSurface->GetReadOnlyData();
  mGL->fTexImage2D(LOCAL_GL_TEXTURE_2D, 0, mGLFormat,
                   TILEDLAYERBUFFER_TILE_SIZE, TILEDLAYERBUFFER_TILE_SIZE, 0,
                   mGLFormat, type, buf);

  gl::GfxTexturesReporter::UpdateAmount(gl::GfxTexturesReporter::MemoryAllocated,
                                        mGLFormat, type,
                                        TILEDLAYERBUFFER_TILE_SIZE);

  if (mGLFormat == LOCAL_GL_RGB) {
    mFormat = FORMAT_R8G8B8X8;
  } else {
    mFormat = FORMAT_B8G8R8A8;
  }
}

bool
TiledDeprecatedTextureHostOGL::Lock()
{
  if (!mTextureHandle) {
    NS_WARNING("TiledDeprecatedTextureHostOGL not ready to be composited");
    return false;
  }

  if (!mGL->MakeCurrent()) {
    return false;
  }
  mGL->fActiveTexture(LOCAL_GL_TEXTURE0);

  return true;
}

#ifdef MOZ_WIDGET_GONK
static gfx::SurfaceFormat
Deprecated_SurfaceFormatForAndroidPixelFormat(android::PixelFormat aFormat,
                                   bool swapRB = false)
{
  switch (aFormat) {
  case android::PIXEL_FORMAT_BGRA_8888:
    return swapRB ? FORMAT_R8G8B8A8 : FORMAT_B8G8R8A8;
  case android::PIXEL_FORMAT_RGBA_8888:
    return swapRB ? FORMAT_B8G8R8A8 : FORMAT_R8G8B8A8;
  case android::PIXEL_FORMAT_RGBX_8888:
    return swapRB ? FORMAT_B8G8R8X8 : FORMAT_R8G8B8X8;
  case android::PIXEL_FORMAT_RGB_565:
    return FORMAT_R5G6B5;
  case android::PIXEL_FORMAT_A_8:
    return FORMAT_A8;
  case HAL_PIXEL_FORMAT_YCbCr_422_SP:
  case HAL_PIXEL_FORMAT_YCrCb_420_SP:
  case HAL_PIXEL_FORMAT_YCbCr_422_I:
  case GrallocImage::HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED:
  case GrallocImage::HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS:
  case HAL_PIXEL_FORMAT_YV12:
    return FORMAT_B8G8R8A8; 
  default:
    if (aFormat >= 0x100 && aFormat <= 0x1FF) {
      
      return FORMAT_B8G8R8A8;
    } else {
      
      
      
      
      
      MOZ_ASSERT(false, "Unknown Android pixel format.");
      return FORMAT_UNKNOWN;
    }
  }
}

static GLenum
Deprecated_TextureTargetForAndroidPixelFormat(android::PixelFormat aFormat)
{
  switch (aFormat) {
  case HAL_PIXEL_FORMAT_YCbCr_422_SP:
  case HAL_PIXEL_FORMAT_YCrCb_420_SP:
  case HAL_PIXEL_FORMAT_YCbCr_422_I:
  case GrallocImage::HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED:
  case GrallocImage::HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS:
  case HAL_PIXEL_FORMAT_YV12:
    return LOCAL_GL_TEXTURE_EXTERNAL;
  case android::PIXEL_FORMAT_RGBA_8888:
  case android::PIXEL_FORMAT_RGBX_8888:
  case android::PIXEL_FORMAT_RGB_565:
  case android::PIXEL_FORMAT_A_8:
    return LOCAL_GL_TEXTURE_2D;
  default:
    if (aFormat >= 0x100 && aFormat <= 0x1FF) {
      
      return LOCAL_GL_TEXTURE_EXTERNAL;
    } else {
      
      
      
      
      
      MOZ_ASSERT(false, "Unknown Android pixel format.");
      return LOCAL_GL_TEXTURE_EXTERNAL;
    }
  }
}

GrallocDeprecatedTextureHostOGL::GrallocDeprecatedTextureHostOGL()
: mCompositor(nullptr)
, mTextureTarget(0)
, mEGLImage(0)
, mIsRBSwapped(false)
{
}

void GrallocDeprecatedTextureHostOGL::SetCompositor(Compositor* aCompositor)
{
  CompositorOGL* glCompositor = static_cast<CompositorOGL*>(aCompositor);
  if (mCompositor && !glCompositor) {
    DeleteTextures();
  }
  mCompositor = glCompositor;
}

gfx::SurfaceFormat
GrallocDeprecatedTextureHostOGL::GetFormat() const
{
  switch (mTextureTarget) {
  case LOCAL_GL_TEXTURE_EXTERNAL: return gfx::FORMAT_R8G8B8A8;
  case LOCAL_GL_TEXTURE_2D: return mFormat;
  default: return gfx::FORMAT_UNKNOWN;
  }
}

void
GrallocDeprecatedTextureHostOGL::DeleteTextures()
{
  if (mEGLImage) {
    if (gl()->MakeCurrent()) {
      gl()->DestroyEGLImage(mEGLImage);
    }
    mEGLImage = 0;
  }
}


static void
AddDeprecatedTextureHostToGrallocBufferActor(DeprecatedTextureHost* aDeprecatedTextureHost, const SurfaceDescriptor* aSurfaceDescriptor)
{
  if (aSurfaceDescriptor && IsSurfaceDescriptorValid(*aSurfaceDescriptor)) {
    GrallocBufferActor* actor = static_cast<GrallocBufferActor*>(aSurfaceDescriptor->get_SurfaceDescriptorGralloc().bufferParent());
    actor->AddDeprecatedTextureHost(aDeprecatedTextureHost);
  }
}

static void
RemoveDeprecatedTextureHostFromGrallocBufferActor(DeprecatedTextureHost* aDeprecatedTextureHost, const SurfaceDescriptor* aSurfaceDescriptor)
{
  if (aSurfaceDescriptor && IsSurfaceDescriptorValid(*aSurfaceDescriptor)) {
    GrallocBufferActor* actor = static_cast<GrallocBufferActor*>(aSurfaceDescriptor->get_SurfaceDescriptorGralloc().bufferParent());
    actor->RemoveDeprecatedTextureHost(aDeprecatedTextureHost);
  }
}

void
GrallocDeprecatedTextureHostOGL::UpdateImpl(const SurfaceDescriptor& aImage,
                                 nsIntRegion* aRegion,
                                 nsIntPoint* aOffset)
{
  SwapTexturesImpl(aImage, aRegion);
}

void
GrallocDeprecatedTextureHostOGL::SwapTexturesImpl(const SurfaceDescriptor& aImage,
                                        nsIntRegion*)
{
  MOZ_ASSERT(aImage.type() == SurfaceDescriptor::TSurfaceDescriptorGralloc);

  const SurfaceDescriptorGralloc& desc = aImage.get_SurfaceDescriptorGralloc();
  mGraphicBuffer = GrallocBufferActor::GetFrom(desc);
  mIsRBSwapped = desc.isRBSwapped();
  mFormat = Deprecated_SurfaceFormatForAndroidPixelFormat(mGraphicBuffer->getPixelFormat(),
                                               mIsRBSwapped);

  mTextureTarget = Deprecated_TextureTargetForAndroidPixelFormat(mGraphicBuffer->getPixelFormat());
  GLuint tex = GetGLTexture();
  
  DeleteTextures();

  if (!gl()->MakeCurrent()) {
    return;
  }
  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
  gl()->fBindTexture(mTextureTarget, tex);
  
  
  
  mEGLImage = gl()->CreateEGLImageForNativeBuffer(mGraphicBuffer->getNativeBuffer());
  gl()->fEGLImageTargetTexture2D(mTextureTarget, mEGLImage);

}

gl::GLContext*
GrallocDeprecatedTextureHostOGL::gl() const
{
  return mCompositor ? mCompositor->gl() : nullptr;
}

void GrallocDeprecatedTextureHostOGL::BindTexture(GLenum aTextureUnit)
{
  PROFILER_LABEL("Gralloc", "BindTexture");
  










  MOZ_ASSERT(gl());
  if (!gl()->MakeCurrent()) {
    return;
  }

  GLuint tex = GetGLTexture();

  gl()->fActiveTexture(aTextureUnit);
  gl()->fBindTexture(mTextureTarget, tex);
  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
}

bool
GrallocDeprecatedTextureHostOGL::IsValid() const
{
  return !!gl() && !!mGraphicBuffer.get();
}

GrallocDeprecatedTextureHostOGL::~GrallocDeprecatedTextureHostOGL()
{
  DeleteTextures();

  
  
  
  RemoveDeprecatedTextureHostFromGrallocBufferActor(this, mBuffer);
}

bool
GrallocDeprecatedTextureHostOGL::Lock()
{
  
  return IsValid();
}

void
GrallocDeprecatedTextureHostOGL::Unlock()
{
  
}

void
GrallocDeprecatedTextureHostOGL::SetBuffer(SurfaceDescriptor* aBuffer, ISurfaceAllocator* aAllocator)
{
  MOZ_ASSERT(!mBuffer, "Will leak the old mBuffer");

  if (aBuffer != mBuffer) {
    
    
    RemoveDeprecatedTextureHostFromGrallocBufferActor(this, mBuffer);
    AddDeprecatedTextureHostToGrallocBufferActor(this, aBuffer);
  }

  mBuffer = aBuffer;
  mDeAllocator = aAllocator;
}

LayerRenderState
GrallocDeprecatedTextureHostOGL::GetRenderState()
{
  if (mGraphicBuffer.get()) {

    uint32_t flags = mFlags & TEXTURE_NEEDS_Y_FLIP ? LAYER_RENDER_STATE_Y_FLIPPED : 0;

    








    if (mIsRBSwapped) {
      flags |= LAYER_RENDER_STATE_FORMAT_RB_SWAP;
    }

    nsIntSize bufferSize(mGraphicBuffer->getWidth(), mGraphicBuffer->getHeight());

    return LayerRenderState(mGraphicBuffer.get(),
                            bufferSize,
                            flags);
  }

  return LayerRenderState();
}

GLuint
GrallocDeprecatedTextureHostOGL::GetGLTexture()
{
  mCompositableBackendData->SetCompositor(mCompositor);
  return static_cast<CompositableDataGonkOGL*>(mCompositableBackendData.get())->GetTexture();
}

#endif 

TemporaryRef<gfx::DataSourceSurface>
TextureImageDeprecatedTextureHostOGL::GetAsSurface() {
  RefPtr<gfx::DataSourceSurface> surf =
    IsValid() ? ReadBackSurface(mGL, mTexture->GetTextureID(),
                                false, mTexture->GetTextureFormat())
              : nullptr;
  return surf.forget();
}

TemporaryRef<gfx::DataSourceSurface>
YCbCrDeprecatedTextureHostOGL::GetAsSurface() {
  RefPtr<gfx::DataSourceSurface> surf =
    IsValid() ? ReadBackSurface(mGL, mYTexture->mTexImage->GetTextureID(),
                                false, mYTexture->mTexImage->GetTextureFormat())
              : nullptr;
  return surf.forget();
}

TemporaryRef<gfx::DataSourceSurface>
SharedDeprecatedTextureHostOGL::GetAsSurface() {
  RefPtr<gfx::DataSourceSurface> surf =
    IsValid() ? ReadBackSurface(mGL, GetTextureHandle(), false, GetFormat())
              : nullptr;
  return surf.forget();
}

TemporaryRef<gfx::DataSourceSurface>
SurfaceStreamHostOGL::GetAsSurface() {
  RefPtr<gfx::DataSourceSurface> surf =
    IsValid() ? ReadBackSurface(mGL, mTextureHandle, false, GetFormat())
              : nullptr;
  return surf.forget();
}

TemporaryRef<gfx::DataSourceSurface>
TiledDeprecatedTextureHostOGL::GetAsSurface() {
  RefPtr<DataSourceSurface> surf =
    IsValid() ? ReadBackSurface(mGL, mTextureHandle, false, GetFormat())
              : nullptr;
  return surf.forget();
}

#ifdef MOZ_WIDGET_GONK
TemporaryRef<gfx::DataSourceSurface>
GrallocDeprecatedTextureHostOGL::GetAsSurface() {
  if (!gl()->MakeCurrent()) {
    return nullptr;
  }
  GLuint tex = GetGLTexture();
  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
  gl()->fBindTexture(mTextureTarget, tex);
  if (!mEGLImage) {
    mEGLImage = gl()->CreateEGLImageForNativeBuffer(mGraphicBuffer->getNativeBuffer());
  }
  gl()->fEGLImageTargetTexture2D(mTextureTarget, mEGLImage);

  RefPtr<gfx::DataSourceSurface> surf =
    IsValid() ? ReadBackSurface(gl(), tex, false, GetFormat())
              : nullptr;
  return surf.forget();
}
#endif 

} 
} 
