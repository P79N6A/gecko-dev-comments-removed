




#include "WebGLTexture.h"

#include "GLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "mozilla/Scoped.h"
#include "ScopedGLHelpers.h"
#include "WebGLContext.h"
#include "WebGLContextUtils.h"
#include "WebGLTexelConversions.h"

#include <algorithm>

using namespace mozilla;

JSObject*
WebGLTexture::WrapObject(JSContext *cx) {
    return dom::WebGLTextureBinding::Wrap(cx, this);
}

WebGLTexture::WebGLTexture(WebGLContext *context)
    : WebGLBindableName<TexTarget>()
    , WebGLContextBoundObject(context)
    , mMinFilter(LOCAL_GL_NEAREST_MIPMAP_LINEAR)
    , mMagFilter(LOCAL_GL_LINEAR)
    , mWrapS(LOCAL_GL_REPEAT)
    , mWrapT(LOCAL_GL_REPEAT)
    , mFacesCount(0)
    , mMaxLevelWithCustomImages(0)
    , mHaveGeneratedMipmap(false)
    , mFakeBlackStatus(WebGLTextureFakeBlackStatus::IncompleteTexture)
{
    SetIsDOMBinding();
    mContext->MakeContextCurrent();
    mContext->gl->fGenTextures(1, &mGLName);
    mContext->mTextures.insertBack(this);
}

void
WebGLTexture::Delete() {
    mImageInfos.Clear();
    mContext->MakeContextCurrent();
    mContext->gl->fDeleteTextures(1, &mGLName);
    LinkedListElement<WebGLTexture>::removeFrom(mContext->mTextures);
}

int64_t
WebGLTexture::ImageInfo::MemoryUsage() const {
    if (mImageDataStatus == WebGLImageDataStatus::NoImageData)
        return 0;
    int64_t bitsPerTexel = WebGLContext::GetBitsPerTexel(mWebGLFormat, mWebGLType);
    return int64_t(mWidth) * int64_t(mHeight) * bitsPerTexel/8;
}

int64_t
WebGLTexture::MemoryUsage() const {
    if (IsDeleted())
        return 0;
    int64_t result = 0;
    for(size_t face = 0; face < mFacesCount; face++) {
        if (mHaveGeneratedMipmap) {
            
            
            
            result += ImageInfoAtFace(face, 0).MemoryUsage() * 4 / 3;
        } else {
            for(size_t level = 0; level <= mMaxLevelWithCustomImages; level++)
                result += ImageInfoAtFace(face, level).MemoryUsage();
        }
    }
    return result;
}

bool
WebGLTexture::DoesTexture2DMipmapHaveAllLevelsConsistentlyDefined(TexImageTarget texImageTarget) const {
    if (mHaveGeneratedMipmap)
        return true;

    
    ImageInfo expected = ImageInfoAt(texImageTarget, 0);

    
    
    for (size_t level = 0; level <= mMaxLevelWithCustomImages; ++level) {
        const ImageInfo& actual = ImageInfoAt(texImageTarget, level);
        if (actual != expected)
            return false;
        expected.mWidth = std::max(1, expected.mWidth >> 1);
        expected.mHeight = std::max(1, expected.mHeight >> 1);

        
        
        if (actual.mWidth == 1 && actual.mHeight == 1)
            return true;
    }

    
    return false;
}

void
WebGLTexture::Bind(TexTarget aTexTarget) {
    
    

    bool firstTimeThisTextureIsBound = !HasEverBeenBound();

    if (firstTimeThisTextureIsBound) {
        BindTo(aTexTarget);
    } else if (aTexTarget != Target()) {
        mContext->ErrorInvalidOperation("bindTexture: this texture has already been bound to a different target");
        
        
        return;
    }

    GLuint name = GLName();

    mContext->gl->fBindTexture(aTexTarget.get(), name);

    if (firstTimeThisTextureIsBound) {
        mFacesCount = (aTexTarget == LOCAL_GL_TEXTURE_2D) ? 1 : 6;
        EnsureMaxLevelWithCustomImagesAtLeast(0);
        SetFakeBlackStatus(WebGLTextureFakeBlackStatus::Unknown);

        
        
        
        if (mTarget == LOCAL_GL_TEXTURE_CUBE_MAP && !mContext->gl->IsGLES())
            mContext->gl->fTexParameteri(aTexTarget.get(), LOCAL_GL_TEXTURE_WRAP_R, LOCAL_GL_CLAMP_TO_EDGE);
    }
}

void
WebGLTexture::SetImageInfo(TexImageTarget aTexImageTarget, GLint aLevel,
                  GLsizei aWidth, GLsizei aHeight,
                  GLenum aFormat, GLenum aType, WebGLImageDataStatus aStatus)
{
    MOZ_ASSERT(TexImageTargetToTexTarget(aTexImageTarget) == mTarget);
    if (TexImageTargetToTexTarget(aTexImageTarget) != mTarget)
        return;

    EnsureMaxLevelWithCustomImagesAtLeast(aLevel);

    ImageInfoAt(aTexImageTarget, aLevel) = ImageInfo(aWidth, aHeight, aFormat, aType, aStatus);

    if (aLevel > 0)
        SetCustomMipmap();

    
    NotifyFBsStatusChanged();

    SetFakeBlackStatus(WebGLTextureFakeBlackStatus::Unknown);
}

void
WebGLTexture::SetGeneratedMipmap() {
    if (!mHaveGeneratedMipmap) {
        mHaveGeneratedMipmap = true;
        SetFakeBlackStatus(WebGLTextureFakeBlackStatus::Unknown);
    }
}

void
WebGLTexture::SetCustomMipmap() {
    if (mHaveGeneratedMipmap) {
        
        

        
        
        ImageInfo imageInfo = ImageInfoAtFace(0, 0);
        NS_ASSERTION(imageInfo.IsPowerOfTwo(), "this texture is NPOT, so how could GenerateMipmap() ever accept it?");

        GLsizei size = std::max(imageInfo.mWidth, imageInfo.mHeight);

        
        size_t maxLevel = 0;
        for (GLsizei n = size; n > 1; n >>= 1)
            ++maxLevel;

        EnsureMaxLevelWithCustomImagesAtLeast(maxLevel);

        for (size_t level = 1; level <= maxLevel; ++level) {
            
            imageInfo.mWidth >>= 1;
            imageInfo.mHeight >>= 1;
            for(size_t face = 0; face < mFacesCount; ++face)
                ImageInfoAtFace(face, level) = imageInfo;
        }
    }
    mHaveGeneratedMipmap = false;
}

bool
WebGLTexture::AreAllLevel0ImageInfosEqual() const {
    for (size_t face = 1; face < mFacesCount; ++face) {
        if (ImageInfoAtFace(face, 0) != ImageInfoAtFace(0, 0))
            return false;
    }
    return true;
}

bool
WebGLTexture::IsMipmapTexture2DComplete() const {
    if (mTarget != LOCAL_GL_TEXTURE_2D)
        return false;
    if (!ImageInfoAt(LOCAL_GL_TEXTURE_2D, 0).IsPositive())
        return false;
    if (mHaveGeneratedMipmap)
        return true;
    return DoesTexture2DMipmapHaveAllLevelsConsistentlyDefined(LOCAL_GL_TEXTURE_2D);
}

bool
WebGLTexture::IsCubeComplete() const {
    if (mTarget != LOCAL_GL_TEXTURE_CUBE_MAP)
        return false;
    const ImageInfo &first = ImageInfoAt(LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0);
    if (!first.IsPositive() || !first.IsSquare())
        return false;
    return AreAllLevel0ImageInfosEqual();
}

static TexImageTarget
GLCubeMapFaceById(int id)
{
    
    return TexImageTarget(LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X + id);
}

bool
WebGLTexture::IsMipmapCubeComplete() const {
    if (!IsCubeComplete()) 
        return false;
    for (int i = 0; i < 6; i++) {
        const TexImageTarget face = GLCubeMapFaceById(i);
        if (!DoesTexture2DMipmapHaveAllLevelsConsistentlyDefined(face))
            return false;
    }
    return true;
}

WebGLTextureFakeBlackStatus
WebGLTexture::ResolvedFakeBlackStatus() {
    if (MOZ_LIKELY(mFakeBlackStatus != WebGLTextureFakeBlackStatus::Unknown)) {
        return mFakeBlackStatus;
    }

    
    

    for (size_t face = 0; face < mFacesCount; ++face) {
        if (ImageInfoAtFace(face, 0).mImageDataStatus == WebGLImageDataStatus::NoImageData) {
            
            
            mFakeBlackStatus = WebGLTextureFakeBlackStatus::IncompleteTexture;
            return mFakeBlackStatus;
        }
    }

    const char *msg_rendering_as_black
        = "A texture is going to be rendered as if it were black, as per the OpenGL ES 2.0.24 spec section 3.8.2, "
          "because it";

    if (mTarget == LOCAL_GL_TEXTURE_2D)
    {
        if (DoesMinFilterRequireMipmap())
        {
            if (!IsMipmapTexture2DComplete()) {
                mContext->GenerateWarning
                    ("%s is a 2D texture, with a minification filter requiring a mipmap, "
                      "and is not mipmap complete (as defined in section 3.7.10).", msg_rendering_as_black);
                mFakeBlackStatus = WebGLTextureFakeBlackStatus::IncompleteTexture;
            } else if (!ImageInfoBase().IsPowerOfTwo()) {
                mContext->GenerateWarning
                    ("%s is a 2D texture, with a minification filter requiring a mipmap, "
                      "and either its width or height is not a power of two.", msg_rendering_as_black);
                mFakeBlackStatus = WebGLTextureFakeBlackStatus::IncompleteTexture;
            }
        }
        else 
        {
            if (!ImageInfoBase().IsPositive()) {
                mContext->GenerateWarning
                    ("%s is a 2D texture and its width or height is equal to zero.",
                      msg_rendering_as_black);
                mFakeBlackStatus = WebGLTextureFakeBlackStatus::IncompleteTexture;
            } else if (!AreBothWrapModesClampToEdge() && !ImageInfoBase().IsPowerOfTwo()) {
                mContext->GenerateWarning
                    ("%s is a 2D texture, with a minification filter not requiring a mipmap, "
                      "with its width or height not a power of two, and with a wrap mode "
                      "different from CLAMP_TO_EDGE.", msg_rendering_as_black);
                mFakeBlackStatus = WebGLTextureFakeBlackStatus::IncompleteTexture;
            }
        }
    }
    else 
    {
        bool areAllLevel0ImagesPOT = true;
        for (size_t face = 0; face < mFacesCount; ++face)
            areAllLevel0ImagesPOT &= ImageInfoAtFace(face, 0).IsPowerOfTwo();

        if (DoesMinFilterRequireMipmap())
        {
            if (!IsMipmapCubeComplete()) {
                mContext->GenerateWarning("%s is a cube map texture, with a minification filter requiring a mipmap, "
                            "and is not mipmap cube complete (as defined in section 3.7.10).",
                            msg_rendering_as_black);
                mFakeBlackStatus = WebGLTextureFakeBlackStatus::IncompleteTexture;
            } else if (!areAllLevel0ImagesPOT) {
                mContext->GenerateWarning("%s is a cube map texture, with a minification filter requiring a mipmap, "
                            "and either the width or the height of some level 0 image is not a power of two.",
                            msg_rendering_as_black);
                mFakeBlackStatus = WebGLTextureFakeBlackStatus::IncompleteTexture;
            }
        }
        else 
        {
            if (!IsCubeComplete()) {
                mContext->GenerateWarning("%s is a cube map texture, with a minification filter not requiring a mipmap, "
                            "and is not cube complete (as defined in section 3.7.10).",
                            msg_rendering_as_black);
                mFakeBlackStatus = WebGLTextureFakeBlackStatus::IncompleteTexture;
            } else if (!AreBothWrapModesClampToEdge() && !areAllLevel0ImagesPOT) {
                mContext->GenerateWarning("%s is a cube map texture, with a minification filter not requiring a mipmap, "
                            "with some level 0 image having width or height not a power of two, and with a wrap mode "
                            "different from CLAMP_TO_EDGE.", msg_rendering_as_black);
                mFakeBlackStatus = WebGLTextureFakeBlackStatus::IncompleteTexture;
            }
        }
    }

    if (ImageInfoBase().mWebGLType == LOCAL_GL_FLOAT &&
        !Context()->IsExtensionEnabled(WebGLExtensionID::OES_texture_float_linear))
    {
        if (mMinFilter == LOCAL_GL_LINEAR ||
            mMinFilter == LOCAL_GL_LINEAR_MIPMAP_LINEAR ||
            mMinFilter == LOCAL_GL_LINEAR_MIPMAP_NEAREST ||
            mMinFilter == LOCAL_GL_NEAREST_MIPMAP_LINEAR)
        {
            mContext->GenerateWarning("%s is a texture with a linear minification filter, "
                                      "which is not compatible with gl.FLOAT by default. "
                                      "Try enabling the OES_texture_float_linear extension if supported.", msg_rendering_as_black);
            mFakeBlackStatus = WebGLTextureFakeBlackStatus::IncompleteTexture;
        }
        else if (mMagFilter == LOCAL_GL_LINEAR)
        {
            mContext->GenerateWarning("%s is a texture with a linear magnification filter, "
                                      "which is not compatible with gl.FLOAT by default. "
                                      "Try enabling the OES_texture_float_linear extension if supported.", msg_rendering_as_black);
            mFakeBlackStatus = WebGLTextureFakeBlackStatus::IncompleteTexture;
        }
    } else if (ImageInfoBase().mWebGLType == LOCAL_GL_HALF_FLOAT_OES &&
               !Context()->IsExtensionEnabled(WebGLExtensionID::OES_texture_half_float_linear))
    {
        if (mMinFilter == LOCAL_GL_LINEAR ||
            mMinFilter == LOCAL_GL_LINEAR_MIPMAP_LINEAR ||
            mMinFilter == LOCAL_GL_LINEAR_MIPMAP_NEAREST ||
            mMinFilter == LOCAL_GL_NEAREST_MIPMAP_LINEAR)
        {
            mContext->GenerateWarning("%s is a texture with a linear minification filter, "
                                      "which is not compatible with gl.HALF_FLOAT by default. "
                                      "Try enabling the OES_texture_half_float_linear extension if supported.", msg_rendering_as_black);
            mFakeBlackStatus = WebGLTextureFakeBlackStatus::IncompleteTexture;
        }
        else if (mMagFilter == LOCAL_GL_LINEAR)
        {
            mContext->GenerateWarning("%s is a texture with a linear magnification filter, "
                                      "which is not compatible with gl.HALF_FLOAT by default. "
                                      "Try enabling the OES_texture_half_float_linear extension if supported.", msg_rendering_as_black);
            mFakeBlackStatus = WebGLTextureFakeBlackStatus::IncompleteTexture;
        }
    }

    
    
    bool hasUninitializedImageData = false;
    for (size_t level = 0; level <= mMaxLevelWithCustomImages; ++level) {
        for (size_t face = 0; face < mFacesCount; ++face) {
            hasUninitializedImageData |= (ImageInfoAtFace(face, level).mImageDataStatus == WebGLImageDataStatus::UninitializedImageData);
        }
    }

    if (hasUninitializedImageData) {
        bool hasAnyInitializedImageData = false;
        for (size_t level = 0; level <= mMaxLevelWithCustomImages; ++level) {
            for (size_t face = 0; face < mFacesCount; ++face) {
                if (ImageInfoAtFace(face, level).mImageDataStatus == WebGLImageDataStatus::InitializedImageData) {
                    hasAnyInitializedImageData = true;
                    break;
                }
            }
            if (hasAnyInitializedImageData) {
                break;
            }
        }

        if (hasAnyInitializedImageData) {
            
            
            
            
            
            for (size_t level = 0; level <= mMaxLevelWithCustomImages; ++level) {
                for (size_t face = 0; face < mFacesCount; ++face) {
                    TexImageTarget imageTarget = mTarget == LOCAL_GL_TEXTURE_2D
                                                 ? LOCAL_GL_TEXTURE_2D
                                                 : LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
                    const ImageInfo& imageInfo = ImageInfoAt(imageTarget, level);
                    if (imageInfo.mImageDataStatus == WebGLImageDataStatus::UninitializedImageData) {
                        DoDeferredImageInitialization(imageTarget, level);
                    }
                }
            }
            mFakeBlackStatus = WebGLTextureFakeBlackStatus::NotNeeded;
        } else {
            
            
            mFakeBlackStatus = WebGLTextureFakeBlackStatus::UninitializedImageData;
        }
    }

    
    
    if (mFakeBlackStatus == WebGLTextureFakeBlackStatus::Unknown) {
        mFakeBlackStatus = WebGLTextureFakeBlackStatus::NotNeeded;
    }

    MOZ_ASSERT(mFakeBlackStatus != WebGLTextureFakeBlackStatus::Unknown);
    return mFakeBlackStatus;
}


static bool
ClearByMask(WebGLContext* context, GLbitfield mask)
{
    gl::GLContext* gl = context->GL();
    MOZ_ASSERT(gl->IsCurrent());

    GLenum status = gl->fCheckFramebufferStatus(LOCAL_GL_FRAMEBUFFER);
    if (status != LOCAL_GL_FRAMEBUFFER_COMPLETE)
        return false;

    bool colorAttachmentsMask[WebGLContext::kMaxColorAttachments] = {false};
    if (mask & LOCAL_GL_COLOR_BUFFER_BIT) {
        colorAttachmentsMask[0] = true;
    }

    context->ForceClearFramebufferWithDefaultValues(mask, colorAttachmentsMask);
    return true;
}


static bool
ClearWithTempFB(WebGLContext* context, GLuint tex,
                TexImageTarget texImageTarget, GLint level,
                GLenum baseInternalFormat,
                GLsizei width, GLsizei height)
{
    if (texImageTarget != LOCAL_GL_TEXTURE_2D)
        return false;

    gl::GLContext* gl = context->GL();
    MOZ_ASSERT(gl->IsCurrent());

    gl::ScopedFramebuffer fb(gl);
    gl::ScopedBindFramebuffer autoFB(gl, fb.FB());
    GLbitfield mask = 0;

    switch (baseInternalFormat) {
    case LOCAL_GL_LUMINANCE:
    case LOCAL_GL_LUMINANCE_ALPHA:
    case LOCAL_GL_ALPHA:
    case LOCAL_GL_RGB:
    case LOCAL_GL_RGBA:
    case LOCAL_GL_BGR:
    case LOCAL_GL_BGRA:
        mask = LOCAL_GL_COLOR_BUFFER_BIT;
        gl->fFramebufferTexture2D(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_COLOR_ATTACHMENT0,
                                  texImageTarget.get(), tex, level);
        break;

    case LOCAL_GL_DEPTH_COMPONENT:
        mask = LOCAL_GL_DEPTH_BUFFER_BIT;
        gl->fFramebufferTexture2D(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_DEPTH_ATTACHMENT,
                                  texImageTarget.get(), tex, level);
        break;

    case LOCAL_GL_DEPTH_STENCIL:
        mask = LOCAL_GL_DEPTH_BUFFER_BIT |
               LOCAL_GL_STENCIL_BUFFER_BIT;
        gl->fFramebufferTexture2D(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_DEPTH_ATTACHMENT,
                                  texImageTarget.get(), tex, level);
        gl->fFramebufferTexture2D(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_STENCIL_ATTACHMENT,
                                  texImageTarget.get(), tex, level);
        break;

    default:
        return false;
    }
    MOZ_ASSERT(mask);

    if (ClearByMask(context, mask))
        return true;

    
    

    if (mask & LOCAL_GL_COLOR_BUFFER_BIT) {
        
        return false;
    }

    gl::ScopedRenderbuffer rb(gl);
    {
        gl::ScopedBindRenderbuffer(gl, rb.RB());
        gl->fRenderbufferStorage(LOCAL_GL_RENDERBUFFER,
                                 LOCAL_GL_RGBA4,
                                 width, height);
    }

    gl->fFramebufferRenderbuffer(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_COLOR_ATTACHMENT0,
                                 LOCAL_GL_RENDERBUFFER, rb.RB());
    mask |= LOCAL_GL_COLOR_BUFFER_BIT;

    
    return ClearByMask(context, mask);
}


void
WebGLTexture::DoDeferredImageInitialization(TexImageTarget imageTarget, GLint level)
{
    const ImageInfo& imageInfo = ImageInfoAt(imageTarget, level);
    MOZ_ASSERT(imageInfo.mImageDataStatus == WebGLImageDataStatus::UninitializedImageData);

    mContext->MakeContextCurrent();

    
    GLenum format = imageInfo.mWebGLFormat;
    GLenum type = imageInfo.mWebGLType;
    WebGLTexelFormat texelformat = GetWebGLTexelFormat(format, type);

    bool cleared = ClearWithTempFB(mContext, GLName(),
                                   imageTarget, level,
                                   format, imageInfo.mHeight, imageInfo.mWidth);
    if (cleared) {
        SetImageDataStatus(imageTarget, level, WebGLImageDataStatus::InitializedImageData);
        return;
    }

    
    gl::ScopedBindTexture autoBindTex(mContext->gl, GLName(), mTarget.get());

    uint32_t texelsize = WebGLTexelConversions::TexelBytesForFormat(texelformat);
    CheckedUint32 checked_byteLength
        = WebGLContext::GetImageSize(
                        imageInfo.mHeight,
                        imageInfo.mWidth,
                        texelsize,
                        mContext->mPixelStoreUnpackAlignment);
    MOZ_ASSERT(checked_byteLength.isValid()); 
    ScopedFreePtr<void> zeros;
    zeros = calloc(1, checked_byteLength.value());

    gl::GLContext* gl = mContext->gl;
    GLenum driverType = DriverTypeFromType(gl, type);
    GLenum driverInternalFormat = LOCAL_GL_NONE;
    GLenum driverFormat = LOCAL_GL_NONE;
    DriverFormatsFromFormatAndType(gl, format, type, &driverInternalFormat, &driverFormat);

    mContext->GetAndFlushUnderlyingGLErrors();
    gl->fTexImage2D(imageTarget.get(), level, driverInternalFormat,
                    imageInfo.mWidth, imageInfo.mHeight,
                    0, driverFormat, driverType,
                    zeros);
    GLenum error = mContext->GetAndFlushUnderlyingGLErrors();
    if (error) {
        
        printf_stderr("Error: 0x%4x\n", error);
        MOZ_CRASH(); 
    }

    SetImageDataStatus(imageTarget, level, WebGLImageDataStatus::InitializedImageData);
}

void
WebGLTexture::SetFakeBlackStatus(WebGLTextureFakeBlackStatus x)
{
    mFakeBlackStatus = x;
    mContext->SetFakeBlackStatus(WebGLContextFakeBlackStatus::Unknown);
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLTexture)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WebGLTexture, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WebGLTexture, Release)
