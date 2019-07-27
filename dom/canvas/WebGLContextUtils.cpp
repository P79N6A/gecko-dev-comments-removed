




#include "WebGLContext.h"

#include <stdarg.h>

#include "GLContext.h"
#include "jsapi.h"
#include "mozilla/Preferences.h"
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
#include "WebGLContextUtils.h"

#include "mozilla/dom/ScriptSettings.h"

namespace mozilla {

using namespace gl;

bool
IsGLDepthFormat(TexInternalFormat internalformat)
{
    TexInternalFormat unsizedformat = UnsizedInternalFormatFromInternalFormat(internalformat);
    return unsizedformat == LOCAL_GL_DEPTH_COMPONENT;
}

bool
IsGLDepthStencilFormat(TexInternalFormat internalformat)
{
    TexInternalFormat unsizedformat = UnsizedInternalFormatFromInternalFormat(internalformat);
    return unsizedformat == LOCAL_GL_DEPTH_STENCIL;
}

bool
FormatHasAlpha(TexInternalFormat internalformat)
{
    TexInternalFormat unsizedformat = UnsizedInternalFormatFromInternalFormat(internalformat);
    return unsizedformat == LOCAL_GL_RGBA ||
           unsizedformat == LOCAL_GL_LUMINANCE_ALPHA ||
           unsizedformat == LOCAL_GL_ALPHA ||
           unsizedformat == LOCAL_GL_SRGB_ALPHA ||
           unsizedformat == LOCAL_GL_RGBA_INTEGER;
}

TexTarget
TexImageTargetToTexTarget(TexImageTarget texImageTarget)
{
    switch (texImageTarget.get()) {
    case LOCAL_GL_TEXTURE_2D:
    case LOCAL_GL_TEXTURE_3D:
        return texImageTarget.get();
    case LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        return LOCAL_GL_TEXTURE_CUBE_MAP;
    default:
        MOZ_ASSERT(false, "Bad texture target");
        
        return LOCAL_GL_NONE;
    }
}

GLComponents::GLComponents(TexInternalFormat internalformat)
{
    TexInternalFormat unsizedformat = UnsizedInternalFormatFromInternalFormat(internalformat);
    mComponents = 0;

    switch (unsizedformat.get()) {
        case LOCAL_GL_RGBA:
        case LOCAL_GL_RGBA4:
        case LOCAL_GL_RGBA8:
        case LOCAL_GL_RGB5_A1:
        
        
        case LOCAL_GL_LUMINANCE_ALPHA:
            mComponents |= Components::Alpha;
        
        case LOCAL_GL_RGB:
        case LOCAL_GL_RGB565:
        
        case LOCAL_GL_LUMINANCE:
            mComponents |= Components::Red | Components::Green | Components::Blue;
            break;
        case LOCAL_GL_ALPHA:
            mComponents |= Components::Alpha;
            break;
        case LOCAL_GL_DEPTH_COMPONENT:
            mComponents |= Components::Depth;
            break;
        case LOCAL_GL_DEPTH_STENCIL:
            mComponents |= Components::Stencil;
            break;
        default:
            MOZ_ASSERT(false, "Unhandled case - GLComponents");
            break;
    }
}

bool
GLComponents::IsSubsetOf(const GLComponents& other) const
{
    return (mComponents | other.mComponents) == other.mComponents;
}

TexType
TypeFromInternalFormat(TexInternalFormat internalformat)
{
#define HANDLE_WEBGL_INTERNAL_FORMAT(table_effectiveinternalformat, table_internalformat, table_type) \
    if (internalformat == table_effectiveinternalformat) { \
        return table_type; \
    }

#include "WebGLInternalFormatsTable.h"

    
    return LOCAL_GL_NONE; 
}

TexInternalFormat
UnsizedInternalFormatFromInternalFormat(TexInternalFormat internalformat)
{
#define HANDLE_WEBGL_INTERNAL_FORMAT(table_effectiveinternalformat, table_internalformat, table_type) \
    if (internalformat == table_effectiveinternalformat) { \
        return table_internalformat; \
    }

#include "WebGLInternalFormatsTable.h"

    
    
    return internalformat;
}







TexInternalFormat
EffectiveInternalFormatFromUnsizedInternalFormatAndType(TexInternalFormat internalformat,
                                                        TexType type)
{
    MOZ_ASSERT(TypeFromInternalFormat(internalformat) == LOCAL_GL_NONE);

#define HANDLE_WEBGL_INTERNAL_FORMAT(table_effectiveinternalformat, table_internalformat, table_type) \
    if (internalformat == table_internalformat && type == table_type) { \
        return table_effectiveinternalformat; \
    }

#include "WebGLInternalFormatsTable.h"

    
    return LOCAL_GL_NONE;
}

void
UnsizedInternalFormatAndTypeFromEffectiveInternalFormat(TexInternalFormat effectiveinternalformat,
                                                        TexInternalFormat* out_internalformat,
                                                        TexType* out_type)
{
    MOZ_ASSERT(TypeFromInternalFormat(effectiveinternalformat) != LOCAL_GL_NONE);

    MOZ_ASSERT(out_internalformat);
    MOZ_ASSERT(out_type);

    GLenum internalformat = LOCAL_GL_NONE;
    GLenum type = LOCAL_GL_NONE;

    switch (effectiveinternalformat.get()) {

#define HANDLE_WEBGL_INTERNAL_FORMAT(table_effectiveinternalformat, table_internalformat, table_type) \
    case table_effectiveinternalformat: \
        internalformat = table_internalformat; \
        type = table_type; \
        break;

#include "WebGLInternalFormatsTable.h"

        default:
            MOZ_CRASH(); 
    }

    *out_internalformat = internalformat;
    *out_type = type;
}

TexInternalFormat
EffectiveInternalFormatFromInternalFormatAndType(TexInternalFormat internalformat,
                                                 TexType type)
{
    TexType typeOfInternalFormat = TypeFromInternalFormat(internalformat);
    if (typeOfInternalFormat == LOCAL_GL_NONE) {
        return EffectiveInternalFormatFromUnsizedInternalFormatAndType(internalformat, type);
    } else if (typeOfInternalFormat == type) {
        return internalformat;
    } else {
        return LOCAL_GL_NONE;
    }
}





void
DriverFormatsFromEffectiveInternalFormat(gl::GLContext* gl,
                                         TexInternalFormat effectiveinternalformat,
                                         GLenum* out_driverInternalFormat,
                                         GLenum* out_driverFormat,
                                         GLenum* out_driverType)
{
    MOZ_ASSERT(out_driverInternalFormat);
    MOZ_ASSERT(out_driverFormat);
    MOZ_ASSERT(out_driverType);

    TexInternalFormat unsizedinternalformat = LOCAL_GL_NONE;
    TexType type = LOCAL_GL_NONE;

    UnsizedInternalFormatAndTypeFromEffectiveInternalFormat(effectiveinternalformat,
                                                            &unsizedinternalformat, &type);

    
    
    GLenum driverType = type.get();
    if (gl->IsGLES() && type == LOCAL_GL_HALF_FLOAT) {
        driverType = LOCAL_GL_HALF_FLOAT_OES;
    }

    
    GLenum driverFormat = unsizedinternalformat.get();

    
    
    
    GLenum driverInternalFormat = driverFormat;
    if (!gl->IsGLES()) {
        
        if (driverFormat == LOCAL_GL_SRGB) {
            driverFormat = LOCAL_GL_RGB;
        } else if (driverFormat == LOCAL_GL_SRGB_ALPHA) {
            driverFormat = LOCAL_GL_RGBA;
        }

        
        
        if (driverFormat == LOCAL_GL_RED ||
            driverFormat == LOCAL_GL_RG ||
            driverFormat == LOCAL_GL_RED_INTEGER ||
            driverFormat == LOCAL_GL_RG_INTEGER ||
            driverFormat == LOCAL_GL_RGB_INTEGER ||
            driverFormat == LOCAL_GL_RGBA_INTEGER)
        {
            driverInternalFormat = effectiveinternalformat.get();
        }

        
        
        
        
        
        
        
        if (unsizedinternalformat == LOCAL_GL_DEPTH_COMPONENT ||
            unsizedinternalformat == LOCAL_GL_DEPTH_STENCIL ||
            type == LOCAL_GL_FLOAT ||
            type == LOCAL_GL_HALF_FLOAT)
        {
            driverInternalFormat = effectiveinternalformat.get();
        }
    }

    *out_driverInternalFormat = driverInternalFormat;
    *out_driverFormat = driverFormat;
    *out_driverType = driverType;
}






size_t
GetBitsPerTexel(TexInternalFormat effectiveinternalformat)
{
    switch (effectiveinternalformat.get()) {
    case LOCAL_GL_COMPRESSED_RGB_PVRTC_2BPPV1:
    case LOCAL_GL_COMPRESSED_RGBA_PVRTC_2BPPV1:
        return 2;

    case LOCAL_GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
    case LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case LOCAL_GL_ATC_RGB:
    case LOCAL_GL_COMPRESSED_RGB_PVRTC_4BPPV1:
    case LOCAL_GL_COMPRESSED_RGBA_PVRTC_4BPPV1:
    case LOCAL_GL_ETC1_RGB8_OES:
        return 4;

    case LOCAL_GL_ALPHA8:
    case LOCAL_GL_LUMINANCE8:
    case LOCAL_GL_R8:
    case LOCAL_GL_R8I:
    case LOCAL_GL_R8UI:
    case LOCAL_GL_R8_SNORM:
    case LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
    case LOCAL_GL_ATC_RGBA_EXPLICIT_ALPHA:
    case LOCAL_GL_ATC_RGBA_INTERPOLATED_ALPHA:
        return 8;

    case LOCAL_GL_LUMINANCE8_ALPHA8:
    case LOCAL_GL_RGBA4:
    case LOCAL_GL_RGB5_A1:
    case LOCAL_GL_DEPTH_COMPONENT16:
    case LOCAL_GL_RG8:
    case LOCAL_GL_R16I:
    case LOCAL_GL_R16UI:
    case LOCAL_GL_RGB565:
    case LOCAL_GL_R16F:
    case LOCAL_GL_RG8I:
    case LOCAL_GL_RG8UI:
    case LOCAL_GL_RG8_SNORM:
    case LOCAL_GL_ALPHA16F_EXT:
    case LOCAL_GL_LUMINANCE16F_EXT:
        return 16;

    case LOCAL_GL_RGB8:
    case LOCAL_GL_DEPTH_COMPONENT24:
    case LOCAL_GL_SRGB8:
    case LOCAL_GL_RGB8UI:
    case LOCAL_GL_RGB8I:
    case LOCAL_GL_RGB8_SNORM:
        return 24;

    case LOCAL_GL_RGBA8:
    case LOCAL_GL_RGB10_A2:
    case LOCAL_GL_R32F:
    case LOCAL_GL_RG16F:
    case LOCAL_GL_R32I:
    case LOCAL_GL_R32UI:
    case LOCAL_GL_RG16I:
    case LOCAL_GL_RG16UI:
    case LOCAL_GL_DEPTH24_STENCIL8:
    case LOCAL_GL_R11F_G11F_B10F:
    case LOCAL_GL_RGB9_E5:
    case LOCAL_GL_SRGB8_ALPHA8:
    case LOCAL_GL_DEPTH_COMPONENT32F:
    case LOCAL_GL_RGBA8UI:
    case LOCAL_GL_RGBA8I:
    case LOCAL_GL_RGBA8_SNORM:
    case LOCAL_GL_RGB10_A2UI:
    case LOCAL_GL_LUMINANCE_ALPHA16F_EXT:
    case LOCAL_GL_ALPHA32F_EXT:
    case LOCAL_GL_LUMINANCE32F_EXT:
        return 32;

    case LOCAL_GL_DEPTH32F_STENCIL8:
        return 40;

    case LOCAL_GL_RGB16F:
    case LOCAL_GL_RGB16UI:
    case LOCAL_GL_RGB16I:
        return 48;

    case LOCAL_GL_RG32F:
    case LOCAL_GL_RG32I:
    case LOCAL_GL_RG32UI:
    case LOCAL_GL_RGBA16F:
    case LOCAL_GL_RGBA16UI:
    case LOCAL_GL_RGBA16I:
    case LOCAL_GL_LUMINANCE_ALPHA32F_EXT:
        return 64;

    case LOCAL_GL_RGB32F:
    case LOCAL_GL_RGB32UI:
    case LOCAL_GL_RGB32I:
        return 96;

    case LOCAL_GL_RGBA32F:
    case LOCAL_GL_RGBA32UI:
    case LOCAL_GL_RGBA32I:
        return 128;

    default:
        MOZ_ASSERT(false, "Unhandled format");
        return 0;
    }
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
                           GLsizei depth,
                           uint32_t pixelSize,
                           uint32_t packOrUnpackAlignment)
{
    CheckedUint32 checked_plainRowSize = CheckedUint32(width) * pixelSize;

    
    CheckedUint32 checked_alignedRowSize = RoundedToNextMultipleOf(checked_plainRowSize, packOrUnpackAlignment);

    
    CheckedUint32 checked_2dImageSize
        = height <= 0 ? 0 : (height-1) * checked_alignedRowSize + checked_plainRowSize;

    
    CheckedUint32 checked_imageSize = checked_2dImageSize * depth;
    return checked_imageSize;
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

const char*
WebGLContext::EnumName(GLenum glenum)
{
    switch (glenum) {
#define XX(x) case LOCAL_GL_##x: return #x
        XX(ALPHA);
        XX(ATC_RGB);
        XX(ATC_RGBA_EXPLICIT_ALPHA);
        XX(ATC_RGBA_INTERPOLATED_ALPHA);
        XX(COMPRESSED_RGBA_PVRTC_2BPPV1);
        XX(COMPRESSED_RGBA_PVRTC_4BPPV1);
        XX(COMPRESSED_RGBA_S3TC_DXT1_EXT);
        XX(COMPRESSED_RGBA_S3TC_DXT3_EXT);
        XX(COMPRESSED_RGBA_S3TC_DXT5_EXT);
        XX(COMPRESSED_RGB_PVRTC_2BPPV1);
        XX(COMPRESSED_RGB_PVRTC_4BPPV1);
        XX(COMPRESSED_RGB_S3TC_DXT1_EXT);
        XX(DEPTH_COMPONENT);
        XX(DEPTH_COMPONENT16);
        XX(DEPTH_COMPONENT32);
        XX(DEPTH_STENCIL);
        XX(DEPTH24_STENCIL8);
        XX(ETC1_RGB8_OES);
        XX(FLOAT);
        XX(HALF_FLOAT);
        XX(LUMINANCE);
        XX(LUMINANCE_ALPHA);
        XX(RGB);
        XX(RGB16F);
        XX(RGB32F);
        XX(RGBA);
        XX(RGBA16F);
        XX(RGBA32F);
        XX(SRGB);
        XX(SRGB_ALPHA);
        XX(TEXTURE_2D);
        XX(TEXTURE_3D);
        XX(TEXTURE_CUBE_MAP);
        XX(TEXTURE_CUBE_MAP_NEGATIVE_X);
        XX(TEXTURE_CUBE_MAP_NEGATIVE_Y);
        XX(TEXTURE_CUBE_MAP_NEGATIVE_Z);
        XX(TEXTURE_CUBE_MAP_POSITIVE_X);
        XX(TEXTURE_CUBE_MAP_POSITIVE_Y);
        XX(TEXTURE_CUBE_MAP_POSITIVE_Z);
        XX(UNSIGNED_BYTE);
        XX(UNSIGNED_INT);
        XX(UNSIGNED_INT_24_8);
        XX(UNSIGNED_SHORT);
        XX(UNSIGNED_SHORT_4_4_4_4);
        XX(UNSIGNED_SHORT_5_5_5_1);
        XX(UNSIGNED_SHORT_5_6_5);
#undef XX
    }

    return "[Unknown enum name]";
}

bool
WebGLContext::IsCompressedTextureFormat(GLenum format)
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
        case LOCAL_GL_COMPRESSED_R11_EAC:
        case LOCAL_GL_COMPRESSED_SIGNED_R11_EAC:
        case LOCAL_GL_COMPRESSED_RG11_EAC:
        case LOCAL_GL_COMPRESSED_SIGNED_RG11_EAC:
        case LOCAL_GL_COMPRESSED_RGB8_ETC2:
        case LOCAL_GL_COMPRESSED_SRGB8_ETC2:
        case LOCAL_GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        case LOCAL_GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        case LOCAL_GL_COMPRESSED_RGBA8_ETC2_EAC:
        case LOCAL_GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
            return true;
        default:
            return false;
    }
}


bool
WebGLContext::IsTextureFormatCompressed(TexInternalFormat format)
{
    return IsCompressedTextureFormat(format.get());
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

void
AssertUintParamCorrect(gl::GLContext* gl, GLenum pname, GLuint shadow)
{
    GLuint val = 0;
    gl->GetUIntegerv(pname, &val);
    if (val != shadow) {
      printf_stderr("Failed 0x%04x shadow: Cached 0x%x/%u, should be 0x%x/%u.\n",
                    pname, shadow, shadow, val, val);
      MOZ_ASSERT(false, "Bad cached value.");
    }
}

void
AssertMaskedUintParamCorrect(gl::GLContext* gl, GLenum pname, GLuint mask, GLuint shadow)
{
    GLuint val = 0;
    gl->GetUIntegerv(pname, &val);

    const GLuint valMasked = val & mask;
    const GLuint shadowMasked = shadow & mask;

    if (valMasked != shadowMasked) {
      printf_stderr("Failed 0x%04x shadow: Cached 0x%x/%u, should be 0x%x/%u.\n",
                    pname, shadowMasked, shadowMasked, valMasked, valMasked);
      MOZ_ASSERT(false, "Bad cached value.");
    }
}
#else
void
AssertUintParamCorrect(gl::GLContext*, GLenum, GLuint)
{
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
    WebGLBuffer* curBuff = mBoundVertexArray->mElementArrayBuffer;
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

    GLint stencilBits = 0;
    gl->fGetIntegerv(LOCAL_GL_STENCIL_BITS, &stencilBits);
    const GLuint stencilRefMask = (1 << stencilBits) - 1;

    AssertMaskedUintParamCorrect(gl, LOCAL_GL_STENCIL_REF,      stencilRefMask, mStencilRefFront);
    AssertMaskedUintParamCorrect(gl, LOCAL_GL_STENCIL_BACK_REF, stencilRefMask, mStencilRefBack);

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

const char*
InfoFrom(WebGLTexImageFunc func, WebGLTexDimensions dims)
{
    switch (dims) {
    case WebGLTexDimensions::Tex2D:
        switch (func) {
        case WebGLTexImageFunc::TexImage:        return "texImage2D";
        case WebGLTexImageFunc::TexSubImage:     return "texSubImage2D";
        case WebGLTexImageFunc::CopyTexImage:    return "copyTexImage2D";
        case WebGLTexImageFunc::CopyTexSubImage: return "copyTexSubImage2D";
        case WebGLTexImageFunc::CompTexImage:    return "compressedTexImage2D";
        case WebGLTexImageFunc::CompTexSubImage: return "compressedTexSubImage2D";
        default:
            MOZ_CRASH();
        }
    case WebGLTexDimensions::Tex3D:
        switch (func) {
        case WebGLTexImageFunc::TexSubImage:     return "texSubImage3D";
        case WebGLTexImageFunc::CopyTexSubImage: return "copyTexSubImage3D";
        case WebGLTexImageFunc::CompTexSubImage: return "compressedTexSubImage3D";
        default:
            MOZ_CRASH();
        }
    default:
        MOZ_CRASH();
    }
}

} 
