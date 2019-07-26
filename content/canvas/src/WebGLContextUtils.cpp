




#include "WebGLContext.h"

#include <stdarg.h>

#include "GLContext.h"
#include "jsapi.h"
#include "mozilla/Preferences.h"
#include "nsCxPusher.h"
#include "nsIDOMDataContainerEvent.h"
#include "nsIDOMEvent.h"
#include "nsIScriptSecurityManager.h"
#include "nsIVariant.h"
#include "nsServiceManagerUtils.h"
#include "prprf.h"
#include "WebGLBuffer.h"
#include "WebGLExtensions.h"
#include "WebGLFramebuffer.h"
#include "WebGLProgram.h"
#include "WebGLTexture.h"
#include "WebGLVertexArray.h"

namespace mozilla {

using namespace gl;

bool
IsGLDepthFormat(GLenum webGLFormat)
{
    return (webGLFormat == LOCAL_GL_DEPTH_COMPONENT ||
            webGLFormat == LOCAL_GL_DEPTH_COMPONENT16 ||
            webGLFormat == LOCAL_GL_DEPTH_COMPONENT32);
}

bool
IsGLDepthStencilFormat(GLenum webGLFormat)
{
    return (webGLFormat == LOCAL_GL_DEPTH_STENCIL ||
            webGLFormat == LOCAL_GL_DEPTH24_STENCIL8);
}

bool
FormatHasAlpha(GLenum webGLFormat)
{
    return webGLFormat == LOCAL_GL_RGBA ||
           webGLFormat == LOCAL_GL_LUMINANCE_ALPHA ||
           webGLFormat == LOCAL_GL_ALPHA ||
           webGLFormat == LOCAL_GL_RGBA4 ||
           webGLFormat == LOCAL_GL_RGB5_A1 ||
           webGLFormat == LOCAL_GL_SRGB_ALPHA;
}





void
DriverFormatsFromFormatAndType(GLContext* gl, GLenum webGLFormat, GLenum webGLType,
                               GLenum* out_driverInternalFormat, GLenum* out_driverFormat)
{
    MOZ_ASSERT(out_driverInternalFormat, "out_driverInternalFormat can't be nullptr.");
    MOZ_ASSERT(out_driverFormat, "out_driverFormat can't be nullptr.");
    if (!out_driverInternalFormat || !out_driverFormat)
        return;

    
    
    
    if (gl->IsGLES()) {
        *out_driverInternalFormat = webGLFormat;
        *out_driverFormat = webGLFormat;

        return;
    }

    GLenum format = webGLFormat;
    GLenum internalFormat = LOCAL_GL_NONE;

    if (format == LOCAL_GL_DEPTH_COMPONENT) {
        if (webGLType == LOCAL_GL_UNSIGNED_SHORT)
            internalFormat = LOCAL_GL_DEPTH_COMPONENT16;
        else if (webGLType == LOCAL_GL_UNSIGNED_INT)
            internalFormat = LOCAL_GL_DEPTH_COMPONENT32;
    } else if (format == LOCAL_GL_DEPTH_STENCIL) {
        if (webGLType == LOCAL_GL_UNSIGNED_INT_24_8_EXT)
            internalFormat = LOCAL_GL_DEPTH24_STENCIL8;
    } else {
        switch (webGLType) {
        case LOCAL_GL_UNSIGNED_BYTE:
        case LOCAL_GL_UNSIGNED_SHORT_4_4_4_4:
        case LOCAL_GL_UNSIGNED_SHORT_5_5_5_1:
        case LOCAL_GL_UNSIGNED_SHORT_5_6_5:
            internalFormat = format;
            break;

        case LOCAL_GL_FLOAT:
            switch (format) {
            case LOCAL_GL_RGBA:
                internalFormat = LOCAL_GL_RGBA32F;
                break;

            case LOCAL_GL_RGB:
                internalFormat = LOCAL_GL_RGB32F;
                break;

            case LOCAL_GL_ALPHA:
                internalFormat = LOCAL_GL_ALPHA32F_ARB;
                break;

            case LOCAL_GL_LUMINANCE:
                internalFormat = LOCAL_GL_LUMINANCE32F_ARB;
                break;

            case LOCAL_GL_LUMINANCE_ALPHA:
                internalFormat = LOCAL_GL_LUMINANCE_ALPHA32F_ARB;
                break;
            }
            break;

        case LOCAL_GL_HALF_FLOAT_OES:
            switch (format) {
            case LOCAL_GL_RGBA:
                internalFormat = LOCAL_GL_RGBA16F;
                break;

            case LOCAL_GL_RGB:
                internalFormat = LOCAL_GL_RGB16F;
                break;

            case LOCAL_GL_ALPHA:
                internalFormat = LOCAL_GL_ALPHA16F_ARB;
                break;

            case LOCAL_GL_LUMINANCE:
                internalFormat = LOCAL_GL_LUMINANCE16F_ARB;
                break;

            case LOCAL_GL_LUMINANCE_ALPHA:
                internalFormat = LOCAL_GL_LUMINANCE_ALPHA16F_ARB;
                break;
            }
            break;

        default:
            break;
        }

        
        
        
        
        
        
        switch (format) {
        case LOCAL_GL_SRGB:
            internalFormat = format;
            format = LOCAL_GL_RGB;
            break;

        case LOCAL_GL_SRGB_ALPHA:
            internalFormat = format;
            format = LOCAL_GL_RGBA;
            break;
        }
    }

    MOZ_ASSERT(format != LOCAL_GL_NONE && internalFormat != LOCAL_GL_NONE,
               "Coding mistake -- bad format/type passed?");

    *out_driverInternalFormat = internalFormat;
    *out_driverFormat = format;
}

GLenum
DriverTypeFromType(GLContext* gl, GLenum webGLType)
{
    if (gl->IsGLES())
        return webGLType;

    
    GLenum type = webGLType;
    if (type == LOCAL_GL_HALF_FLOAT_OES) {
        if (gl->IsSupported(gl::GLFeature::texture_half_float)) {
            return LOCAL_GL_HALF_FLOAT;
        } else {
            MOZ_ASSERT(gl->IsExtensionSupported(gl::GLContext::OES_texture_half_float));
        }
    }

    return webGLType;
}

void
WebGLContext::GenerateWarning(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    GenerateWarning(fmt, ap);

    va_end(ap);
}

void
WebGLContext::GenerateWarning(const char *fmt, va_list ap)
{
    if (!ShouldGenerateWarnings())
        return;

    mAlreadyGeneratedWarnings++;

    char buf[1024];
    PR_vsnprintf(buf, 1024, fmt, ap);

    

    AutoJSContext cx;
    JS_ReportWarning(cx, "WebGL: %s", buf);
    if (!ShouldGenerateWarnings()) {
        JS_ReportWarning(cx,
            "WebGL: No further warnings will be reported for this WebGL context "
            "(already reported %d warnings)", mAlreadyGeneratedWarnings);
    }
}

bool
WebGLContext::ShouldGenerateWarnings() const
{
    if (mMaxWarnings == -1) {
        return true;
    }

    return mAlreadyGeneratedWarnings < mMaxWarnings;
}

CheckedUint32
WebGLContext::GetImageSize(GLsizei height,
                           GLsizei width,
                           uint32_t pixelSize,
                           uint32_t packOrUnpackAlignment)
{
    CheckedUint32 checked_plainRowSize = CheckedUint32(width) * pixelSize;

    
    CheckedUint32 checked_alignedRowSize = RoundedToNextMultipleOf(checked_plainRowSize, packOrUnpackAlignment);

    
    CheckedUint32 checked_neededByteLength
        = height <= 0 ? 0 : (height-1) * checked_alignedRowSize + checked_plainRowSize;

    return checked_neededByteLength;
}

void
WebGLContext::SynthesizeGLError(GLenum err)
{
    





    if (!mWebGLError)
        mWebGLError = err;
}

void
WebGLContext::SynthesizeGLError(GLenum err, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    GenerateWarning(fmt, va);
    va_end(va);

    return SynthesizeGLError(err);
}

void
WebGLContext::ErrorInvalidEnum(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    GenerateWarning(fmt, va);
    va_end(va);

    return SynthesizeGLError(LOCAL_GL_INVALID_ENUM);
}

void
WebGLContext::ErrorInvalidEnumInfo(const char *info, GLenum enumvalue)
{
    return ErrorInvalidEnum("%s: invalid enum value 0x%x", info, enumvalue);
}

void
WebGLContext::ErrorInvalidOperation(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    GenerateWarning(fmt, va);
    va_end(va);

    return SynthesizeGLError(LOCAL_GL_INVALID_OPERATION);
}

void
WebGLContext::ErrorInvalidValue(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    GenerateWarning(fmt, va);
    va_end(va);

    return SynthesizeGLError(LOCAL_GL_INVALID_VALUE);
}

void
WebGLContext::ErrorInvalidFramebufferOperation(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    GenerateWarning(fmt, va);
    va_end(va);

    return SynthesizeGLError(LOCAL_GL_INVALID_FRAMEBUFFER_OPERATION);
}

void
WebGLContext::ErrorOutOfMemory(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    GenerateWarning(fmt, va);
    va_end(va);

    return SynthesizeGLError(LOCAL_GL_OUT_OF_MEMORY);
}

const char *
WebGLContext::ErrorName(GLenum error)
{
    switch(error) {
        case LOCAL_GL_INVALID_ENUM:
            return "INVALID_ENUM";
        case LOCAL_GL_INVALID_OPERATION:
            return "INVALID_OPERATION";
        case LOCAL_GL_INVALID_VALUE:
            return "INVALID_VALUE";
        case LOCAL_GL_OUT_OF_MEMORY:
            return "OUT_OF_MEMORY";
        case LOCAL_GL_INVALID_FRAMEBUFFER_OPERATION:
            return "INVALID_FRAMEBUFFER_OPERATION";
        case LOCAL_GL_NO_ERROR:
            return "NO_ERROR";
        default:
            MOZ_ASSERT(false);
            return "[unknown WebGL error!]";
    }
}

bool
WebGLContext::IsTextureFormatCompressed(GLenum format)
{
    switch (format) {
        case LOCAL_GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case LOCAL_GL_ATC_RGB:
        case LOCAL_GL_ATC_RGBA_EXPLICIT_ALPHA:
        case LOCAL_GL_ATC_RGBA_INTERPOLATED_ALPHA:
        case LOCAL_GL_COMPRESSED_RGB_PVRTC_4BPPV1:
        case LOCAL_GL_COMPRESSED_RGB_PVRTC_2BPPV1:
        case LOCAL_GL_COMPRESSED_RGBA_PVRTC_4BPPV1:
        case LOCAL_GL_COMPRESSED_RGBA_PVRTC_2BPPV1:
        case LOCAL_GL_ETC1_RGB8_OES:
            return true;
        default:
            return false;
    }
}

GLenum
WebGLContext::GetAndFlushUnderlyingGLErrors()
{
    
    GLenum error = gl->GetAndClearError();

    
    
    if (!mUnderlyingGLError)
        mUnderlyingGLError = error;

    return error;
}

#ifdef DEBUG

static bool
IsCacheCorrect(float cached, float actual)
{
    if (IsNaN(cached)) {
        
        
        return true;
    }

    return cached == actual;
}

static void
AssertUintParamCorrect(gl::GLContext* gl, GLenum pname, GLuint shadow)
{
    GLuint val = 0;
    gl->GetUIntegerv(pname, &val);
    MOZ_ASSERT(val == shadow);
}
#endif

void
WebGLContext::AssertCachedBindings()
{
#ifdef DEBUG
    MakeContextCurrent();

    GetAndFlushUnderlyingGLErrors();

    if (IsExtensionEnabled(WebGLExtensionID::OES_vertex_array_object)) {
        GLuint bound = mBoundVertexArray ? mBoundVertexArray->GLName() : 0;
        AssertUintParamCorrect(gl, LOCAL_GL_VERTEX_ARRAY_BINDING, bound);
    }

    
    GLuint bound = mBoundFramebuffer ? mBoundFramebuffer->GLName() : 0;
    AssertUintParamCorrect(gl, LOCAL_GL_FRAMEBUFFER_BINDING, bound);

    bound = mCurrentProgram ? mCurrentProgram->GLName() : 0;
    AssertUintParamCorrect(gl, LOCAL_GL_CURRENT_PROGRAM, bound);

    
    GLenum activeTexture = mActiveTexture + LOCAL_GL_TEXTURE0;
    AssertUintParamCorrect(gl, LOCAL_GL_ACTIVE_TEXTURE, activeTexture);

    WebGLTexture* curTex = activeBoundTextureForTarget(LOCAL_GL_TEXTURE_2D);
    bound = curTex ? curTex->GLName() : 0;
    AssertUintParamCorrect(gl, LOCAL_GL_TEXTURE_BINDING_2D, bound);

    curTex = activeBoundTextureForTarget(LOCAL_GL_TEXTURE_CUBE_MAP);
    bound = curTex ? curTex->GLName() : 0;
    AssertUintParamCorrect(gl, LOCAL_GL_TEXTURE_BINDING_CUBE_MAP, bound);

    
    bound = mBoundArrayBuffer ? mBoundArrayBuffer->GLName() : 0;
    AssertUintParamCorrect(gl, LOCAL_GL_ARRAY_BUFFER_BINDING, bound);

    MOZ_ASSERT(mBoundVertexArray);
    WebGLBuffer* curBuff = mBoundVertexArray->mBoundElementArrayBuffer;
    bound = curBuff ? curBuff->GLName() : 0;
    AssertUintParamCorrect(gl, LOCAL_GL_ELEMENT_ARRAY_BUFFER_BINDING, bound);

    MOZ_ASSERT(!GetAndFlushUnderlyingGLErrors());
#endif
}

void
WebGLContext::AssertCachedState()
{
#ifdef DEBUG
    MakeContextCurrent();

    GetAndFlushUnderlyingGLErrors();

    
    if (IsExtensionEnabled(WebGLExtensionID::WEBGL_draw_buffers)) {
        AssertUintParamCorrect(gl, LOCAL_GL_MAX_COLOR_ATTACHMENTS, mGLMaxColorAttachments);
        AssertUintParamCorrect(gl, LOCAL_GL_MAX_DRAW_BUFFERS, mGLMaxDrawBuffers);
    }

    
    MOZ_ASSERT(gl->fIsEnabled(LOCAL_GL_SCISSOR_TEST) == mScissorTestEnabled);
    MOZ_ASSERT(gl->fIsEnabled(LOCAL_GL_DITHER) == mDitherEnabled);
    MOZ_ASSERT_IF(IsWebGL2(),
                  gl->fIsEnabled(LOCAL_GL_RASTERIZER_DISCARD) == mRasterizerDiscardEnabled);


    realGLboolean colorWriteMask[4] = {0, 0, 0, 0};
    gl->fGetBooleanv(LOCAL_GL_COLOR_WRITEMASK, colorWriteMask);
    MOZ_ASSERT(colorWriteMask[0] == mColorWriteMask[0] &&
               colorWriteMask[1] == mColorWriteMask[1] &&
               colorWriteMask[2] == mColorWriteMask[2] &&
               colorWriteMask[3] == mColorWriteMask[3]);

    GLfloat colorClearValue[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    gl->fGetFloatv(LOCAL_GL_COLOR_CLEAR_VALUE, colorClearValue);
    MOZ_ASSERT(IsCacheCorrect(mColorClearValue[0], colorClearValue[0]) &&
               IsCacheCorrect(mColorClearValue[1], colorClearValue[1]) &&
               IsCacheCorrect(mColorClearValue[2], colorClearValue[2]) &&
               IsCacheCorrect(mColorClearValue[3], colorClearValue[3]));

    realGLboolean depthWriteMask = 0;
    gl->fGetBooleanv(LOCAL_GL_DEPTH_WRITEMASK, &depthWriteMask);
    MOZ_ASSERT(depthWriteMask == mDepthWriteMask);

    GLfloat depthClearValue = 0.0f;
    gl->fGetFloatv(LOCAL_GL_DEPTH_CLEAR_VALUE, &depthClearValue);
    MOZ_ASSERT(IsCacheCorrect(mDepthClearValue, depthClearValue));

    AssertUintParamCorrect(gl, LOCAL_GL_STENCIL_CLEAR_VALUE, mStencilClearValue);

    AssertUintParamCorrect(gl, LOCAL_GL_STENCIL_REF,      mStencilRefFront);
    AssertUintParamCorrect(gl, LOCAL_GL_STENCIL_BACK_REF, mStencilRefBack);

    AssertUintParamCorrect(gl, LOCAL_GL_STENCIL_VALUE_MASK,      mStencilValueMaskFront);
    AssertUintParamCorrect(gl, LOCAL_GL_STENCIL_BACK_VALUE_MASK, mStencilValueMaskBack);

    AssertUintParamCorrect(gl, LOCAL_GL_STENCIL_WRITEMASK,      mStencilWriteMaskFront);
    AssertUintParamCorrect(gl, LOCAL_GL_STENCIL_BACK_WRITEMASK, mStencilWriteMaskBack);

    
    GLint int4[4] = {0, 0, 0, 0};
    gl->fGetIntegerv(LOCAL_GL_VIEWPORT, int4);
    MOZ_ASSERT(int4[0] == mViewportX &&
               int4[1] == mViewportY &&
               int4[2] == mViewportWidth &&
               int4[3] == mViewportHeight);

    AssertUintParamCorrect(gl, LOCAL_GL_PACK_ALIGNMENT, mPixelStorePackAlignment);
    AssertUintParamCorrect(gl, LOCAL_GL_UNPACK_ALIGNMENT, mPixelStoreUnpackAlignment);

    MOZ_ASSERT(!GetAndFlushUnderlyingGLErrors());
#endif
}

} 
