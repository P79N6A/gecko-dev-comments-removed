




#include "WebGLContext.h"

#include <algorithm>
#include "angle/ShaderLang.h"
#include "CanvasUtils.h"
#include "GLContext.h"
#include "jsfriendapi.h"
#include "mozilla/CheckedInt.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "nsIObserverService.h"
#include "WebGLBuffer.h"
#include "WebGLContextUtils.h"
#include "WebGLFramebuffer.h"
#include "WebGLProgram.h"
#include "WebGLRenderbuffer.h"
#include "WebGLShader.h"
#include "WebGLTexture.h"
#include "WebGLUniformLocation.h"
#include "WebGLValidateStrings.h"
#include "WebGLVertexArray.h"
#include "WebGLVertexAttribData.h"

#if defined(MOZ_WIDGET_COCOA)
#include "nsCocoaFeatures.h"
#endif

namespace mozilla {




static void
BlockSizeFor(GLenum format, GLint* const out_blockWidth,
             GLint* const out_blockHeight)
{
    MOZ_ASSERT(out_blockWidth && out_blockHeight);

    switch (format) {
    case LOCAL_GL_ATC_RGB:
    case LOCAL_GL_ATC_RGBA_EXPLICIT_ALPHA:
    case LOCAL_GL_ATC_RGBA_INTERPOLATED_ALPHA:
    case LOCAL_GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
    case LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        *out_blockWidth = 4;
        *out_blockHeight = 4;
        break;

    case LOCAL_GL_ETC1_RGB8_OES:
        
        break;

    default:
        break;
    }
}

static bool
IsCompressedFunc(WebGLTexImageFunc func)
{
    return func == WebGLTexImageFunc::CompTexImage ||
           func == WebGLTexImageFunc::CompTexSubImage;
}





static void
ErrorInvalidEnumWithName(WebGLContext* ctx, const char* msg, GLenum glenum,
                         WebGLTexImageFunc func, WebGLTexDimensions dims)
{
    const char* name = WebGLContext::EnumName(glenum);
    if (name) {
        ctx->ErrorInvalidEnum("%s: %s %s", InfoFrom(func, dims), msg, name);
    } else {
        ctx->ErrorInvalidEnum("%s: %s 0x%04x", InfoFrom(func, dims), msg,
                              glenum);
    }
}




static bool
IsAllowedFromSource(GLenum format, WebGLTexImageFunc func)
{
    switch (format) {
    case LOCAL_GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
    case LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
    case LOCAL_GL_COMPRESSED_RGB_PVRTC_2BPPV1:
    case LOCAL_GL_COMPRESSED_RGB_PVRTC_4BPPV1:
    case LOCAL_GL_COMPRESSED_RGBA_PVRTC_2BPPV1:
    case LOCAL_GL_COMPRESSED_RGBA_PVRTC_4BPPV1:
        return (func == WebGLTexImageFunc::CompTexImage ||
                func == WebGLTexImageFunc::CompTexSubImage);

    case LOCAL_GL_ATC_RGB:
    case LOCAL_GL_ATC_RGBA_EXPLICIT_ALPHA:
    case LOCAL_GL_ATC_RGBA_INTERPOLATED_ALPHA:
    case LOCAL_GL_ETC1_RGB8_OES:
        return func == WebGLTexImageFunc::CompTexImage;
    }

    return true;
}




static bool
IsCopyFunc(WebGLTexImageFunc func)
{
    return (func == WebGLTexImageFunc::CopyTexImage ||
            func == WebGLTexImageFunc::CopyTexSubImage);
}




static bool
IsSubFunc(WebGLTexImageFunc func)
{
    return (func == WebGLTexImageFunc::TexSubImage ||
            func == WebGLTexImageFunc::CopyTexSubImage ||
            func == WebGLTexImageFunc::CompTexSubImage);
}




static bool
IsTexImageCubemapTarget(GLenum texImageTarget)
{
    return (texImageTarget >= LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
            texImageTarget <= LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
}

bool
WebGLContext::ValidateBlendEquationEnum(GLenum mode, const char* info)
{
    switch (mode) {
    case LOCAL_GL_FUNC_ADD:
    case LOCAL_GL_FUNC_SUBTRACT:
    case LOCAL_GL_FUNC_REVERSE_SUBTRACT:
        return true;

    case LOCAL_GL_MIN:
    case LOCAL_GL_MAX:
        if (IsExtensionEnabled(WebGLExtensionID::EXT_blend_minmax))
            return true;

        break;

    default:
        break;
    }

    ErrorInvalidEnumInfo(info, mode);
    return false;
}

bool
WebGLContext::ValidateBlendFuncDstEnum(GLenum factor, const char* info)
{
    switch (factor) {
    case LOCAL_GL_ZERO:
    case LOCAL_GL_ONE:
    case LOCAL_GL_SRC_COLOR:
    case LOCAL_GL_ONE_MINUS_SRC_COLOR:
    case LOCAL_GL_DST_COLOR:
    case LOCAL_GL_ONE_MINUS_DST_COLOR:
    case LOCAL_GL_SRC_ALPHA:
    case LOCAL_GL_ONE_MINUS_SRC_ALPHA:
    case LOCAL_GL_DST_ALPHA:
    case LOCAL_GL_ONE_MINUS_DST_ALPHA:
    case LOCAL_GL_CONSTANT_COLOR:
    case LOCAL_GL_ONE_MINUS_CONSTANT_COLOR:
    case LOCAL_GL_CONSTANT_ALPHA:
    case LOCAL_GL_ONE_MINUS_CONSTANT_ALPHA:
        return true;

    default:
        ErrorInvalidEnumInfo(info, factor);
        return false;
    }
}

bool
WebGLContext::ValidateBlendFuncSrcEnum(GLenum factor, const char* info)
{
    if (factor == LOCAL_GL_SRC_ALPHA_SATURATE)
        return true;

    return ValidateBlendFuncDstEnum(factor, info);
}

bool
WebGLContext::ValidateBlendFuncEnumsCompatibility(GLenum sfactor,
                                                  GLenum dfactor,
                                                  const char* info)
{
    bool sfactorIsConstantColor = sfactor == LOCAL_GL_CONSTANT_COLOR ||
                                  sfactor == LOCAL_GL_ONE_MINUS_CONSTANT_COLOR;
    bool sfactorIsConstantAlpha = sfactor == LOCAL_GL_CONSTANT_ALPHA ||
                                  sfactor == LOCAL_GL_ONE_MINUS_CONSTANT_ALPHA;
    bool dfactorIsConstantColor = dfactor == LOCAL_GL_CONSTANT_COLOR ||
                                  dfactor == LOCAL_GL_ONE_MINUS_CONSTANT_COLOR;
    bool dfactorIsConstantAlpha = dfactor == LOCAL_GL_CONSTANT_ALPHA ||
                                  dfactor == LOCAL_GL_ONE_MINUS_CONSTANT_ALPHA;
    if ( (sfactorIsConstantColor && dfactorIsConstantAlpha) ||
         (dfactorIsConstantColor && sfactorIsConstantAlpha) )
    {
        ErrorInvalidOperation("%s are mutually incompatible, see section 6.8 in"
                              " the WebGL 1.0 spec", info);
        return false;
    }

    return true;
}

bool
WebGLContext::ValidateDataOffsetSize(WebGLintptr offset, WebGLsizeiptr size, WebGLsizeiptr bufferSize, const char* info)
{
    if (offset < 0) {
        ErrorInvalidValue("%s: offset must be positive", info);
        return false;
    }

    if (size < 0) {
        ErrorInvalidValue("%s: size must be positive", info);
        return false;
    }

    
    
    CheckedInt<GLsizeiptr> neededBytes = CheckedInt<GLsizeiptr>(offset) + size;
    if (!neededBytes.isValid() || neededBytes.value() > bufferSize) {
        ErrorInvalidValue("%s: invalid range", info);
        return false;
    }

    return true;
}








bool
WebGLContext::ValidateDataRanges(WebGLintptr readOffset, WebGLintptr writeOffset, WebGLsizeiptr size, const char* info)
{
    MOZ_ASSERT((CheckedInt<WebGLsizeiptr>(readOffset) + size).isValid());
    MOZ_ASSERT((CheckedInt<WebGLsizeiptr>(writeOffset) + size).isValid());

    bool separate = (readOffset + size < writeOffset || writeOffset + size < readOffset);
    if (!separate)
        ErrorInvalidValue("%s: ranges [readOffset, readOffset + size) and [writeOffset, writeOffset + size) overlap");

    return separate;
}

bool
WebGLContext::ValidateTextureTargetEnum(GLenum target, const char* info)
{
    switch (target) {
    case LOCAL_GL_TEXTURE_2D:
    case LOCAL_GL_TEXTURE_CUBE_MAP:
        return true;

    case LOCAL_GL_TEXTURE_3D:
        if (IsWebGL2())
            return true;

        break;

    default:
        break;
    }

    ErrorInvalidEnumInfo(info, target);
    return false;
}

bool
WebGLContext::ValidateComparisonEnum(GLenum target, const char* info)
{
    switch (target) {
    case LOCAL_GL_NEVER:
    case LOCAL_GL_LESS:
    case LOCAL_GL_LEQUAL:
    case LOCAL_GL_GREATER:
    case LOCAL_GL_GEQUAL:
    case LOCAL_GL_EQUAL:
    case LOCAL_GL_NOTEQUAL:
    case LOCAL_GL_ALWAYS:
        return true;

    default:
        ErrorInvalidEnumInfo(info, target);
        return false;
    }
}

bool
WebGLContext::ValidateStencilOpEnum(GLenum action, const char* info)
{
    switch (action) {
    case LOCAL_GL_KEEP:
    case LOCAL_GL_ZERO:
    case LOCAL_GL_REPLACE:
    case LOCAL_GL_INCR:
    case LOCAL_GL_INCR_WRAP:
    case LOCAL_GL_DECR:
    case LOCAL_GL_DECR_WRAP:
    case LOCAL_GL_INVERT:
        return true;

    default:
        ErrorInvalidEnumInfo(info, action);
        return false;
    }
}

bool
WebGLContext::ValidateFaceEnum(GLenum face, const char* info)
{
    switch (face) {
    case LOCAL_GL_FRONT:
    case LOCAL_GL_BACK:
    case LOCAL_GL_FRONT_AND_BACK:
        return true;

    default:
        ErrorInvalidEnumInfo(info, face);
        return false;
    }
}

bool
WebGLContext::ValidateDrawModeEnum(GLenum mode, const char* info)
{
    switch (mode) {
    case LOCAL_GL_TRIANGLES:
    case LOCAL_GL_TRIANGLE_STRIP:
    case LOCAL_GL_TRIANGLE_FAN:
    case LOCAL_GL_POINTS:
    case LOCAL_GL_LINE_STRIP:
    case LOCAL_GL_LINE_LOOP:
    case LOCAL_GL_LINES:
        return true;

    default:
        ErrorInvalidEnumInfo(info, mode);
        return false;
    }
}





bool
WebGLContext::ValidateFramebufferAttachment(const WebGLFramebuffer* fb, GLenum attachment,
                                            const char* funcName)
{
    if (!fb) {
        switch (attachment) {
        case LOCAL_GL_COLOR:
        case LOCAL_GL_DEPTH:
        case LOCAL_GL_STENCIL:
            return true;

        default:
            ErrorInvalidEnum("%s: attachment: invalid enum value 0x%x.",
                             funcName, attachment);
            return false;
        }
    }

    if (attachment == LOCAL_GL_DEPTH_ATTACHMENT ||
        attachment == LOCAL_GL_STENCIL_ATTACHMENT ||
        attachment == LOCAL_GL_DEPTH_STENCIL_ATTACHMENT)
    {
        return true;
    }

    GLenum colorAttachCount = 1;
    if (IsExtensionEnabled(WebGLExtensionID::WEBGL_draw_buffers))
        colorAttachCount = mGLMaxColorAttachments;

    if (attachment >= LOCAL_GL_COLOR_ATTACHMENT0 &&
        attachment < GLenum(LOCAL_GL_COLOR_ATTACHMENT0 + colorAttachCount))
    {
        return true;
    }

    ErrorInvalidEnum("%s: attachment: invalid enum value 0x%x.", funcName,
                     attachment);
    return false;
}




bool
WebGLContext::ValidateSamplerParameterName(GLenum pname, const char* info)
{
    switch (pname) {
    case LOCAL_GL_TEXTURE_MIN_FILTER:
    case LOCAL_GL_TEXTURE_MAG_FILTER:
    case LOCAL_GL_TEXTURE_WRAP_S:
    case LOCAL_GL_TEXTURE_WRAP_T:
    case LOCAL_GL_TEXTURE_WRAP_R:
    case LOCAL_GL_TEXTURE_MIN_LOD:
    case LOCAL_GL_TEXTURE_MAX_LOD:
    case LOCAL_GL_TEXTURE_COMPARE_MODE:
    case LOCAL_GL_TEXTURE_COMPARE_FUNC:
        return true;

    default:
        ErrorInvalidEnum("%s: invalid pname: %s", info, EnumName(pname));
        return false;
    }
}




bool
WebGLContext::ValidateSamplerParameterParams(GLenum pname, const WebGLIntOrFloat& param, const char* info)
{
    const GLenum p = param.AsInt();

    switch (pname) {
    case LOCAL_GL_TEXTURE_MIN_FILTER:
        switch (p) {
        case LOCAL_GL_NEAREST:
        case LOCAL_GL_LINEAR:
        case LOCAL_GL_NEAREST_MIPMAP_NEAREST:
        case LOCAL_GL_NEAREST_MIPMAP_LINEAR:
        case LOCAL_GL_LINEAR_MIPMAP_NEAREST:
        case LOCAL_GL_LINEAR_MIPMAP_LINEAR:
            return true;

        default:
            ErrorInvalidEnum("%s: invalid param: %s", info, EnumName(p));
            return false;
        }

    case LOCAL_GL_TEXTURE_MAG_FILTER:
        switch (p) {
        case LOCAL_GL_NEAREST:
        case LOCAL_GL_LINEAR:
            return true;

        default:
            ErrorInvalidEnum("%s: invalid param: %s", info, EnumName(p));
            return false;
        }

    case LOCAL_GL_TEXTURE_WRAP_S:
    case LOCAL_GL_TEXTURE_WRAP_T:
    case LOCAL_GL_TEXTURE_WRAP_R:
        switch (p) {
        case LOCAL_GL_CLAMP_TO_EDGE:
        case LOCAL_GL_REPEAT:
        case LOCAL_GL_MIRRORED_REPEAT:
            return true;

        default:
            ErrorInvalidEnum("%s: invalid param: %s", info, EnumName(p));
            return false;
        }

    case LOCAL_GL_TEXTURE_MIN_LOD:
    case LOCAL_GL_TEXTURE_MAX_LOD:
        return true;

    case LOCAL_GL_TEXTURE_COMPARE_MODE:
        switch (param.AsInt()) {
        case LOCAL_GL_NONE:
        case LOCAL_GL_COMPARE_REF_TO_TEXTURE:
            return true;

        default:
            ErrorInvalidEnum("%s: invalid param: %s", info, EnumName(p));
            return false;
        }

    case LOCAL_GL_TEXTURE_COMPARE_FUNC:
        switch (p) {
        case LOCAL_GL_LEQUAL:
        case LOCAL_GL_GEQUAL:
        case LOCAL_GL_LESS:
        case LOCAL_GL_GREATER:
        case LOCAL_GL_EQUAL:
        case LOCAL_GL_NOTEQUAL:
        case LOCAL_GL_ALWAYS:
        case LOCAL_GL_NEVER:
            return true;

        default:
            ErrorInvalidEnum("%s: invalid param: %s", info, EnumName(p));
            return false;
        }

    default:
        ErrorInvalidEnum("%s: invalid pname: %s", info, EnumName(pname));
        return false;
    }
}






bool
WebGLContext::ValidateTexImageFormat(GLenum format, WebGLTexImageFunc func,
                                     WebGLTexDimensions dims)
{
    
    if (format == LOCAL_GL_ALPHA ||
        format == LOCAL_GL_RGB ||
        format == LOCAL_GL_RGBA ||
        format == LOCAL_GL_LUMINANCE ||
        format == LOCAL_GL_LUMINANCE_ALPHA)
    {
        return true;
    }

    
    if (format == LOCAL_GL_RED ||
        format == LOCAL_GL_RG ||
        format == LOCAL_GL_RED_INTEGER ||
        format == LOCAL_GL_RG_INTEGER ||
        format == LOCAL_GL_RGB_INTEGER ||
        format == LOCAL_GL_RGBA_INTEGER)
    {
        if (IsWebGL2())
            return true;

        ErrorInvalidEnum("%s: Invalid format %s: Requires WebGL version 2.0 or"
                         " newer.", InfoFrom(func, dims), EnumName(format));
        return false;
    }

    
    if (format == LOCAL_GL_DEPTH_COMPONENT ||
        format == LOCAL_GL_DEPTH_STENCIL)
    {
        if (!IsExtensionEnabled(WebGLExtensionID::WEBGL_depth_texture)) {
            ErrorInvalidEnum("%s: Invalid format %s: Requires that"
                             " WEBGL_depth_texture is enabled.",
                             InfoFrom(func, dims), EnumName(format));
            return false;
        }

        
        
        
        if ((func == WebGLTexImageFunc::TexSubImage && !IsWebGL2()) ||
            func == WebGLTexImageFunc::CopyTexImage ||
            func == WebGLTexImageFunc::CopyTexSubImage)
        {
            ErrorInvalidOperation("%s: format %s is not supported",
                                  InfoFrom(func, dims), EnumName(format));
            return false;
        }

        return true;
    }

    
    
    
    
    if (IsCopyFunc(func)) {
        ErrorInvalidEnumWithName(this, "invalid format", format, func, dims);
        return false;
    }

    
    if (format == LOCAL_GL_SRGB ||
        format == LOCAL_GL_SRGB_ALPHA)
    {
        if (IsExtensionEnabled(WebGLExtensionID::EXT_sRGB))
            return true;

        ErrorInvalidEnum("%s: Invalid format %s: Requires that EXT_sRGB is"
                         " enabled.", InfoFrom(func, dims),
                         WebGLContext::EnumName(format));
        return false;
    }

    ErrorInvalidEnumWithName(this, "invalid format", format, func, dims);
    return false;
}




bool
WebGLContext::ValidateTexImageTarget(GLenum target, WebGLTexImageFunc func,
                                     WebGLTexDimensions dims)
{
    switch (dims) {
    case WebGLTexDimensions::Tex2D:
        if (target == LOCAL_GL_TEXTURE_2D ||
            IsTexImageCubemapTarget(target))
        {
            return true;
        }

        ErrorInvalidEnumWithName(this, "invalid target", target, func, dims);
        return false;

    case WebGLTexDimensions::Tex3D:
        if (target == LOCAL_GL_TEXTURE_3D)
        {
            return true;
        }

        ErrorInvalidEnumWithName(this, "invalid target", target, func, dims);
        return false;

    default:
        MOZ_ASSERT(false, "ValidateTexImageTarget: Invalid dims");
    }

    return false;
}





bool
WebGLContext::ValidateTexImageType(GLenum type, WebGLTexImageFunc func,
                                   WebGLTexDimensions dims)
{
    
    if (type == LOCAL_GL_UNSIGNED_BYTE ||
        type == LOCAL_GL_UNSIGNED_SHORT_5_6_5 ||
        type == LOCAL_GL_UNSIGNED_SHORT_4_4_4_4 ||
        type == LOCAL_GL_UNSIGNED_SHORT_5_5_5_1)
    {
        return true;
    }

    
    if (type == LOCAL_GL_BYTE ||
        type == LOCAL_GL_SHORT ||
        type == LOCAL_GL_INT ||
        type == LOCAL_GL_FLOAT_32_UNSIGNED_INT_24_8_REV ||
        type == LOCAL_GL_UNSIGNED_INT_2_10_10_10_REV ||
        type == LOCAL_GL_UNSIGNED_INT_10F_11F_11F_REV ||
        type == LOCAL_GL_UNSIGNED_INT_5_9_9_9_REV)
    {
        if (IsWebGL2())
            return true;

        ErrorInvalidEnum("%s: Invalid type %s: Requires WebGL version 2.0 or"
                         " newer.", InfoFrom(func, dims),
                         WebGLContext::EnumName(type));
        return false;
    }

    
    if (type == LOCAL_GL_FLOAT) {
        if (IsExtensionEnabled(WebGLExtensionID::OES_texture_float))
            return true;

        ErrorInvalidEnum("%s: Invalid type %s: Requires that OES_texture_float"
                         " is enabled.",
                         InfoFrom(func, dims), WebGLContext::EnumName(type));
        return false;
    }

    
    if (type == LOCAL_GL_HALF_FLOAT) {
        if (IsExtensionEnabled(WebGLExtensionID::OES_texture_half_float))
            return true;

        ErrorInvalidEnum("%s: Invalid type %s: Requires that"
                         " OES_texture_half_float is enabled.",
                         InfoFrom(func, dims), WebGLContext::EnumName(type));
        return false;
    }

    
    if (type == LOCAL_GL_UNSIGNED_SHORT ||
        type == LOCAL_GL_UNSIGNED_INT ||
        type == LOCAL_GL_UNSIGNED_INT_24_8)
    {
        if (IsExtensionEnabled(WebGLExtensionID::WEBGL_depth_texture))
            return true;

        ErrorInvalidEnum("%s: Invalid type %s: Requires that"
                         " WEBGL_depth_texture is enabled.",
                         InfoFrom(func, dims), WebGLContext::EnumName(type));
        return false;
    }

    ErrorInvalidEnumWithName(this, "invalid type", type, func, dims);
    return false;
}






bool
WebGLContext::ValidateCompTexImageSize(GLint level, GLenum format,
                                       GLint xoffset, GLint yoffset,
                                       GLsizei width, GLsizei height,
                                       GLsizei levelWidth, GLsizei levelHeight,
                                       WebGLTexImageFunc func,
                                       WebGLTexDimensions dims)
{
    
    MOZ_ASSERT(xoffset >= 0 && yoffset >= 0 &&
               width >= 0 && height >= 0);

    if (xoffset + width > (GLint) levelWidth) {
        ErrorInvalidValue("%s: xoffset + width must be <= levelWidth.",
                          InfoFrom(func, dims));
        return false;
    }

    if (yoffset + height > (GLint) levelHeight) {
        ErrorInvalidValue("%s: yoffset + height must be <= levelHeight.",
                          InfoFrom(func, dims));
        return false;
    }

    GLint blockWidth = 1;
    GLint blockHeight = 1;
    BlockSizeFor(format, &blockWidth, &blockHeight);

    
    
    
    if (blockWidth != 1 || blockHeight != 1) {
        
        if (xoffset % blockWidth != 0) {
            ErrorInvalidOperation("%s: xoffset must be multiple of %d.",
                                  InfoFrom(func, dims), blockWidth);
            return false;
        }

        if (yoffset % blockHeight != 0) {
            ErrorInvalidOperation("%s: yoffset must be multiple of %d.",
                                  InfoFrom(func, dims), blockHeight);
            return false;
        }

        









        if (level == 0) {
            if (width % blockWidth != 0) {
                ErrorInvalidOperation("%s: Width of level 0 must be a multiple"
                                      " of %d.", InfoFrom(func, dims),
                                      blockWidth);
                return false;
            }

            if (height % blockHeight != 0) {
                ErrorInvalidOperation("%s: Height of level 0 must be a multiple"
                                      " of %d.", InfoFrom(func, dims),
                                      blockHeight);
                return false;
            }
        } else if (level > 0) {
            if (width % blockWidth != 0 && width > 2) {
                ErrorInvalidOperation("%s: Width of level %d must be a multiple"
                                      " of %d, or be 0, 1, or 2.",
                                      InfoFrom(func, dims), level, blockWidth);
                return false;
            }

            if (height % blockHeight != 0 && height > 2) {
                ErrorInvalidOperation("%s: Height of level %d must be a"
                                      " multiple of %d, or be 0, 1, or 2.",
                                      InfoFrom(func, dims), level, blockHeight);
                return false;
            }
        }

        if (IsSubFunc(func)) {
            if ((xoffset % blockWidth) != 0) {
                ErrorInvalidOperation("%s: xoffset must be a multiple of %d.",
                                      InfoFrom(func, dims), blockWidth);
                return false;
            }

            if (yoffset % blockHeight != 0) {
                ErrorInvalidOperation("%s: yoffset must be a multiple of %d.",
                                      InfoFrom(func, dims), blockHeight);
                return false;
            }
        }
    }

    switch (format) {
    case LOCAL_GL_COMPRESSED_RGB_PVRTC_4BPPV1:
    case LOCAL_GL_COMPRESSED_RGB_PVRTC_2BPPV1:
    case LOCAL_GL_COMPRESSED_RGBA_PVRTC_4BPPV1:
    case LOCAL_GL_COMPRESSED_RGBA_PVRTC_2BPPV1:
        if (!IsPOTAssumingNonnegative(width) ||
            !IsPOTAssumingNonnegative(height))
        {
            ErrorInvalidValue("%s: Width and height must be powers of two.",
                              InfoFrom(func, dims));
            return false;
        }
    }

    return true;
}





bool
WebGLContext::ValidateCompTexImageDataSize(GLint level, GLenum format,
                                           GLsizei width, GLsizei height,
                                           uint32_t byteLength,
                                           WebGLTexImageFunc func,
                                           WebGLTexDimensions dims)
{
    
    MOZ_ASSERT(width >= 0 && height >= 0);

    CheckedUint32 required_byteLength = 0;

    switch (format) {
    case LOCAL_GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
    case LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case LOCAL_GL_ATC_RGB:
    case LOCAL_GL_ETC1_RGB8_OES:
        required_byteLength = ((CheckedUint32(width) + 3) / 4) *
                              ((CheckedUint32(height) + 3) / 4) * 8;
        break;

    case LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
    case LOCAL_GL_ATC_RGBA_EXPLICIT_ALPHA:
    case LOCAL_GL_ATC_RGBA_INTERPOLATED_ALPHA:
        required_byteLength = ((CheckedUint32(width) + 3) / 4) *
                              ((CheckedUint32(height) + 3) / 4) * 16;
        break;

    case LOCAL_GL_COMPRESSED_RGB_PVRTC_4BPPV1:
    case LOCAL_GL_COMPRESSED_RGBA_PVRTC_4BPPV1:
        required_byteLength = CheckedUint32(std::max(width, 8)) *
                              CheckedUint32(std::max(height, 8)) / 2;
        break;

    case LOCAL_GL_COMPRESSED_RGB_PVRTC_2BPPV1:
    case LOCAL_GL_COMPRESSED_RGBA_PVRTC_2BPPV1:
        required_byteLength = CheckedUint32(std::max(width, 16)) *
                              CheckedUint32(std::max(height, 8)) / 4;
        break;
    }

    if (!required_byteLength.isValid() ||
        required_byteLength.value() != byteLength)
    {
        ErrorInvalidValue("%s: Data size does not match dimensions.",
                          InfoFrom(func, dims));
        return false;
    }

    return true;
}







bool
WebGLContext::ValidateTexImageSize(TexImageTarget texImageTarget, GLint level,
                                   GLint width, GLint height, GLint depth,
                                   WebGLTexImageFunc func,
                                   WebGLTexDimensions dims)
{
    MOZ_ASSERT(level >= 0, "level should already be validated");

    









    if (level > 31)
        level = 31;

    auto texTarget = TexImageTargetToTexTarget(texImageTarget);
    const GLuint maxTexImageSize = MaxTextureSizeForTarget(texTarget) >> level;

    const bool isCubemapTarget = IsTexImageCubemapTarget(texImageTarget.get());
    const bool isSub = IsSubFunc(func);

    if (!isSub && isCubemapTarget && (width != height)) {
        





        ErrorInvalidValue("%s: For cube maps, width must equal height.",
                          InfoFrom(func, dims));
        return false;
    }

    if (texImageTarget == LOCAL_GL_TEXTURE_2D || isCubemapTarget) {
        




        if (width < 0) {
            ErrorInvalidValue("%s: Width must be >= 0.", InfoFrom(func, dims));
            return false;
        }

        if (height < 0) {
            ErrorInvalidValue("%s: Height must be >= 0.", InfoFrom(func, dims));
            return false;
        }

        









        if (width > (int) maxTexImageSize) {
            ErrorInvalidValue("%s: The maximum width for level %d is %u.",
                              InfoFrom(func, dims), level, maxTexImageSize);
            return false;
        }

        if (height > (int) maxTexImageSize) {
            ErrorInvalidValue("%s: The maximum height for level %d is %u.",
                              InfoFrom(func, dims), level, maxTexImageSize);
            return false;
        }

        






        if (!IsWebGL2() && level > 0) {
            if (!IsPOTAssumingNonnegative(width)) {
                ErrorInvalidValue("%s: For level > 0, width of %d must be a"
                                  " power of two.", InfoFrom(func, dims),
                                  width);
                return false;
            }

            if (!IsPOTAssumingNonnegative(height)) {
                ErrorInvalidValue("%s: For level > 0, height of %d must be a"
                                  " power of two.", InfoFrom(func, dims),
                                  height);
                return false;
            }
        }
    }

    
    if (texImageTarget == LOCAL_GL_TEXTURE_3D) {
        if (depth < 0) {
            ErrorInvalidValue("%s: Depth must be >= 0.", InfoFrom(func, dims));
            return false;
        }

        if (!IsWebGL2() && !IsPOTAssumingNonnegative(depth)) {
            ErrorInvalidValue("%s: Depth of %d must be a power of two.",
                              InfoFrom(func, dims), depth);
            return false;
        }
    }

    return true;
}





bool
WebGLContext::ValidateTexSubImageSize(GLint xoffset, GLint yoffset,
                                      GLint , GLsizei width,
                                      GLsizei height, GLsizei ,
                                      GLsizei baseWidth, GLsizei baseHeight,
                                      GLsizei ,
                                      WebGLTexImageFunc func,
                                      WebGLTexDimensions dims)
{
    










    if (xoffset < 0) {
        ErrorInvalidValue("%s: xoffset must be >= 0.", InfoFrom(func, dims));
        return false;
    }

    if (yoffset < 0) {
        ErrorInvalidValue("%s: yoffset must be >= 0.", InfoFrom(func, dims));
        return false;
    }

    if (!CanvasUtils::CheckSaneSubrectSize(xoffset, yoffset, width, height,
                                           baseWidth, baseHeight))
    {
        ErrorInvalidValue("%s: Subtexture rectangle out-of-bounds.",
                          InfoFrom(func, dims));
        return false;
    }

    return true;
}





bool
WebGLContext::ValidateTexImageFormatAndType(GLenum format, GLenum type,
                                            WebGLTexImageFunc func,
                                            WebGLTexDimensions dims)
{
    if (IsCompressedFunc(func) || IsCopyFunc(func)) {
        MOZ_ASSERT(type == LOCAL_GL_NONE && format == LOCAL_GL_NONE);
        return true;
    }

    if (!ValidateTexImageFormat(format, func, dims) ||
        !ValidateTexImageType(type, func, dims))
    {
        return false;
    }

    
    
    
    TexInternalFormat effective =
        EffectiveInternalFormatFromInternalFormatAndType(format, type);

    if (effective != LOCAL_GL_NONE)
        return true;

    ErrorInvalidOperation("%s: Invalid combination of format %s and type %s.",
                          InfoFrom(func, dims), WebGLContext::EnumName(format),
                          WebGLContext::EnumName(type));
    return false;
}

bool
WebGLContext::ValidateCompTexImageInternalFormat(GLenum format,
                                                 WebGLTexImageFunc func,
                                                 WebGLTexDimensions dims)
{
    if (!IsCompressedTextureFormat(format)) {
        ErrorInvalidEnum("%s: Invalid compressed texture format: %s",
                         InfoFrom(func, dims), WebGLContext::EnumName(format));
        return false;
    }

    
    if (format == LOCAL_GL_ATC_RGB ||
        format == LOCAL_GL_ATC_RGBA_EXPLICIT_ALPHA ||
        format == LOCAL_GL_ATC_RGBA_INTERPOLATED_ALPHA)
    {
        if (IsExtensionEnabled(WebGLExtensionID::WEBGL_compressed_texture_atc))
            return true;

        ErrorInvalidEnum("%s: Invalid format %s: Requires that"
                         " WEBGL_compressed_texture_atc is enabled.",
                         InfoFrom(func, dims), WebGLContext::EnumName(format));
        return false;
    }

    
    if (format == LOCAL_GL_ETC1_RGB8_OES) {
        if (IsExtensionEnabled(WebGLExtensionID::WEBGL_compressed_texture_etc1))
            return true;

        ErrorInvalidEnum("%s: Invalid format %s: Requires that"
                         " WEBGL_compressed_texture_etc1 is enabled.",
                         InfoFrom(func, dims), WebGLContext::EnumName(format));
        return false;
    }


    if (format == LOCAL_GL_COMPRESSED_RGB_PVRTC_2BPPV1 ||
        format == LOCAL_GL_COMPRESSED_RGB_PVRTC_4BPPV1 ||
        format == LOCAL_GL_COMPRESSED_RGBA_PVRTC_2BPPV1 ||
        format == LOCAL_GL_COMPRESSED_RGBA_PVRTC_4BPPV1)
    {
        if (IsExtensionEnabled(WebGLExtensionID::WEBGL_compressed_texture_pvrtc))
            return true;

        ErrorInvalidEnum("%s: Invalid format %s: Requires that"
                         " WEBGL_compressed_texture_pvrtc is enabled.",
                         InfoFrom(func, dims), WebGLContext::EnumName(format));
        return false;
    }


    if (format == LOCAL_GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
        format == LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
        format == LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
        format == LOCAL_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
    {
        if (IsExtensionEnabled(WebGLExtensionID::WEBGL_compressed_texture_s3tc))
            return true;

        ErrorInvalidEnum("%s: Invalid format %s: Requires that"
                         " WEBGL_compressed_texture_s3tc is enabled.",
                         InfoFrom(func, dims), WebGLContext::EnumName(format));
        return false;
    }

    MOZ_ASSERT(false);
    return false;
}

bool
WebGLContext::ValidateCopyTexImageInternalFormat(GLenum format,
                                                 WebGLTexImageFunc func,
                                                 WebGLTexDimensions dims)
{
    switch (format) {
    case LOCAL_GL_RGBA:
    case LOCAL_GL_RGB:
    case LOCAL_GL_LUMINANCE_ALPHA:
    case LOCAL_GL_LUMINANCE:
    case LOCAL_GL_ALPHA:
        return true;
    }
    
    
    
    
    GenerateWarning("%s: Invalid texture internal format: %s",
                    InfoFrom(func, dims), WebGLContext::EnumName(format));

    GLenum error;
    if (func == WebGLTexImageFunc::CopyTexImage)
        error = LOCAL_GL_INVALID_ENUM;
    else
        error = LOCAL_GL_INVALID_OPERATION;

    SynthesizeGLError(error);
    return false;
}







bool
WebGLContext::ValidateTexInputData(GLenum type, js::Scalar::Type jsArrayType,
                                   WebGLTexImageFunc func,
                                   WebGLTexDimensions dims)
{
    
    
    if (jsArrayType == js::Scalar::MaxTypedArrayViewType)
        return true;

    const char invalidTypedArray[] = "%s: Invalid typed array type for given"
                                     " texture data type.";

    bool validInput = false;
    switch (type) {
    case LOCAL_GL_UNSIGNED_BYTE:
        validInput = jsArrayType == js::Scalar::Uint8;
        break;

    case LOCAL_GL_BYTE:
        validInput = jsArrayType == js::Scalar::Int8;
        break;

    case LOCAL_GL_HALF_FLOAT:
    case LOCAL_GL_UNSIGNED_SHORT:
    case LOCAL_GL_UNSIGNED_SHORT_4_4_4_4:
    case LOCAL_GL_UNSIGNED_SHORT_5_5_5_1:
    case LOCAL_GL_UNSIGNED_SHORT_5_6_5:
        validInput = jsArrayType == js::Scalar::Uint16;
        break;

    case LOCAL_GL_SHORT:
        validInput = jsArrayType == js::Scalar::Int16;
        break;

    case LOCAL_GL_UNSIGNED_INT:
    case LOCAL_GL_UNSIGNED_INT_24_8:
    case LOCAL_GL_UNSIGNED_INT_2_10_10_10_REV:
    case LOCAL_GL_UNSIGNED_INT_10F_11F_11F_REV:
    case LOCAL_GL_UNSIGNED_INT_5_9_9_9_REV:
        validInput = jsArrayType == js::Scalar::Uint32;
        break;

    case LOCAL_GL_INT:
        validInput = jsArrayType == js::Scalar::Int32;
        break;

    case LOCAL_GL_FLOAT:
        validInput = jsArrayType == js::Scalar::Float32;
        break;

    default:
        break;
    }

    if (!validInput)
        ErrorInvalidOperation(invalidTypedArray, InfoFrom(func, dims));

    return validInput;
}








bool
WebGLContext::ValidateCopyTexImage(GLenum format, WebGLTexImageFunc func,
                                   WebGLTexDimensions dims)
{
    MOZ_ASSERT(IsCopyFunc(func));

    
    GLenum fboFormat = mOptions.alpha ? LOCAL_GL_RGBA : LOCAL_GL_RGB;

    if (mBoundReadFramebuffer) {
        TexInternalFormat srcFormat;
        if (!mBoundReadFramebuffer->ValidateForRead(InfoFrom(func, dims), &srcFormat))
            return false;

        fboFormat = srcFormat.get();
    }

    
    
    const GLComponents formatComps = GLComponents(format);
    const GLComponents fboComps = GLComponents(fboFormat);
    if (!formatComps.IsSubsetOf(fboComps)) {
        ErrorInvalidOperation("%s: Format %s is not a subset of the current"
                              " framebuffer format, which is %s.",
                              InfoFrom(func, dims), EnumName(format),
                              EnumName(fboFormat));
        return false;
    }

    return true;
}







bool
WebGLContext::ValidateTexImage(TexImageTarget texImageTarget, GLint level,
                               GLenum internalFormat, GLint xoffset,
                               GLint yoffset, GLint zoffset, GLint width,
                               GLint height, GLint depth, GLint border,
                               GLenum format, GLenum type,
                               WebGLTexImageFunc func,
                               WebGLTexDimensions dims)
{
    const char* info = InfoFrom(func, dims);

    
    if (level < 0) {
        ErrorInvalidValue("%s: `level` must be >= 0.", info);
        return false;
    }

    
    if (border != 0) {
        ErrorInvalidValue("%s: `border` must be 0.", info);
        return false;
    }

    
    if (!ValidateTexImageFormatAndType(format, type, func, dims))
        return false;

    if (!TexInternalFormat::IsValueLegal(internalFormat)) {
        ErrorInvalidEnum("%s: Invalid `internalformat` enum %s.", info,
                         EnumName(internalFormat));
        return false;
    }
    TexInternalFormat unsizedInternalFormat =
        UnsizedInternalFormatFromInternalFormat(internalFormat);

    if (IsCompressedFunc(func)) {
        if (!ValidateCompTexImageInternalFormat(internalFormat, func, dims))
            return false;

    } else if (IsCopyFunc(func)) {
        if (!ValidateCopyTexImageInternalFormat(unsizedInternalFormat.get(),
                                                func, dims))
        {
            return false;
        }
    } else if (format != unsizedInternalFormat) {
        if (IsWebGL2()) {
            
            
            
            
            auto effectiveFormat = EffectiveInternalFormatFromInternalFormatAndType(format,
                                                                                    type);
            if (internalFormat != effectiveFormat) {
                bool exceptionallyAllowed = false;
                if (internalFormat == LOCAL_GL_SRGB8_ALPHA8 &&
                    format == LOCAL_GL_RGBA &&
                    type == LOCAL_GL_UNSIGNED_BYTE)
                {
                    exceptionallyAllowed = true;
                }
                else if (internalFormat == LOCAL_GL_SRGB8 &&
                         format == LOCAL_GL_RGB &&
                         type == LOCAL_GL_UNSIGNED_BYTE)
                {
                    exceptionallyAllowed = true;
                }
                if (!exceptionallyAllowed) {
                    ErrorInvalidOperation("%s: `internalformat` does not match"
                                          " `format` and `type`.", info);
                    return false;
                }
            }
        } else {
            
            ErrorInvalidOperation("%s: `internalformat` does not match"
                                  " `format`.", info);
            return false;
        }
    }

    
    if (!ValidateTexImageSize(texImageTarget, level, width, height, 0, func,
                              dims))
    {
        return false;
    }

    




    WebGLTexture* tex = ActiveBoundTextureForTexImageTarget(texImageTarget);
    if (!tex) {
        ErrorInvalidOperation("%s: No texture is bound to target %s.", info,
                              WebGLContext::EnumName(texImageTarget.get()));
        return false;
    }

    if (IsSubFunc(func)) {
        if (!tex->HasImageInfoAt(texImageTarget, level)) {
            ErrorInvalidOperation("%s: No texture image previously defined for"
                                  " target %s at level %d.", info,
                                  WebGLContext::EnumName(texImageTarget.get()),
                                                         level);
            return false;
        }

        const auto& imageInfo = tex->ImageInfoAt(texImageTarget, level);
        if (!ValidateTexSubImageSize(xoffset, yoffset, zoffset, width, height,
                                     depth, imageInfo.Width(),
                                     imageInfo.Height(), 0, func, dims))
        {
            return false;
        }
    }

    
    if (texImageTarget != LOCAL_GL_TEXTURE_2D &&
        (format == LOCAL_GL_DEPTH_COMPONENT ||
         format == LOCAL_GL_DEPTH_STENCIL))
    {
        ErrorInvalidOperation("%s: With format of %s, target must be"
                              " TEXTURE_2D.", info,
                              WebGLContext::EnumName(format));
        return false;
    }

    
    if (!IsAllowedFromSource(internalFormat, func)) {
        ErrorInvalidOperation("%s: Invalid format %s for this operation.",
                              info, WebGLContext::EnumName(format));
        return false;
    }

    
    return true;
}

bool
WebGLContext::ValidateUniformLocation(WebGLUniformLocation* loc, const char* funcName)
{
    




    if (!loc)
        return false;

    if (!ValidateObject(funcName, loc))
        return false;

    if (!mCurrentProgram) {
        ErrorInvalidOperation("%s: No program is currently bound.", funcName);
        return false;
    }

    return loc->ValidateForProgram(mCurrentProgram, this, funcName);
}

bool
WebGLContext::ValidateAttribArraySetter(const char* name, uint32_t setterElemSize,
                                        uint32_t arrayLength)
{
    if (IsContextLost())
        return false;

    if (arrayLength < setterElemSize) {
        ErrorInvalidOperation("%s: Array must have >= %d elements.", name,
                              setterElemSize);
        return false;
    }

    return true;
}

bool
WebGLContext::ValidateUniformSetter(WebGLUniformLocation* loc,
                                    uint8_t setterElemSize, GLenum setterType,
                                    const char* funcName, GLuint* out_rawLoc)
{
    if (IsContextLost())
        return false;

    if (!ValidateUniformLocation(loc, funcName))
        return false;

    if (!loc->ValidateSizeAndType(setterElemSize, setterType, this, funcName))
        return false;

    *out_rawLoc = loc->mLoc;
    return true;
}

bool
WebGLContext::ValidateUniformArraySetter(WebGLUniformLocation* loc,
                                         uint8_t setterElemSize,
                                         GLenum setterType,
                                         size_t setterArraySize,
                                         const char* funcName,
                                         GLuint* const out_rawLoc,
                                         GLsizei* const out_numElementsToUpload)
{
    if (IsContextLost())
        return false;

    if (!ValidateUniformLocation(loc, funcName))
        return false;

    if (!loc->ValidateSizeAndType(setterElemSize, setterType, this, funcName))
        return false;

    if (!loc->ValidateArrayLength(setterElemSize, setterArraySize, this, funcName))
        return false;

    *out_rawLoc = loc->mLoc;
    *out_numElementsToUpload = std::min((size_t)loc->mActiveInfo->mElemCount,
                                        setterArraySize / setterElemSize);
    return true;
}

bool
WebGLContext::ValidateUniformMatrixArraySetter(WebGLUniformLocation* loc,
                                               uint8_t setterCols,
                                               uint8_t setterRows,
                                               GLenum setterType,
                                               size_t setterArraySize,
                                               bool setterTranspose,
                                               const char* funcName,
                                               GLuint* const out_rawLoc,
                                               GLsizei* const out_numElementsToUpload)
{
    uint8_t setterElemSize = setterCols * setterRows;

    if (IsContextLost())
        return false;

    if (!ValidateUniformLocation(loc, funcName))
        return false;

    if (!loc->ValidateSizeAndType(setterElemSize, setterType, this, funcName))
        return false;

    if (!loc->ValidateArrayLength(setterElemSize, setterArraySize, this, funcName))
        return false;

    if (!ValidateUniformMatrixTranspose(setterTranspose, funcName))
        return false;

    *out_rawLoc = loc->mLoc;
    *out_numElementsToUpload = std::min((size_t)loc->mActiveInfo->mElemCount,
                                        setterArraySize / setterElemSize);
    return true;
}

bool
WebGLContext::ValidateAttribIndex(GLuint index, const char* info)
{
    bool valid = (index < MaxVertexAttribs());

    if (!valid) {
        if (index == GLuint(-1)) {
            ErrorInvalidValue("%s: -1 is not a valid `index`. This value"
                              " probably comes from a getAttribLocation()"
                              " call, where this return value -1 means"
                              " that the passed name didn't correspond to"
                              " an active attribute in the specified"
                              " program.", info);
        } else {
            ErrorInvalidValue("%s: `index` must be less than"
                              " MAX_VERTEX_ATTRIBS.", info);
        }
    }

    return valid;
}

bool
WebGLContext::ValidateAttribPointer(bool integerMode, GLuint index, GLint size, GLenum type,
                                    WebGLboolean normalized, GLsizei stride,
                                    WebGLintptr byteOffset, const char* info)
{
    WebGLBuffer* buffer = mBoundArrayBuffer;
    if (!buffer) {
        ErrorInvalidOperation("%s: must have valid GL_ARRAY_BUFFER binding", info);
        return false;
    }

    GLsizei requiredAlignment = 0;
    if (!ValidateAttribPointerType(integerMode, type, &requiredAlignment, info))
        return false;

    
    MOZ_ASSERT(IsPOTAssumingNonnegative(requiredAlignment));
    GLsizei requiredAlignmentMask = requiredAlignment - 1;

    if (size < 1 || size > 4) {
        ErrorInvalidValue("%s: invalid element size", info);
        return false;
    }

    
    if (stride < 0 || stride > 255) {
        ErrorInvalidValue("%s: negative or too large stride", info);
        return false;
    }

    if (byteOffset < 0) {
        ErrorInvalidValue("%s: negative offset", info);
        return false;
    }

    if (stride & requiredAlignmentMask) {
        ErrorInvalidOperation("%s: stride doesn't satisfy the alignment "
                              "requirement of given type", info);
        return false;
    }

    if (byteOffset & requiredAlignmentMask) {
        ErrorInvalidOperation("%s: byteOffset doesn't satisfy the alignment "
                              "requirement of given type", info);
        return false;
    }

    return true;
}

bool
WebGLContext::ValidateStencilParamsForDrawCall()
{
    const char msg[] = "%s set different front and back stencil %s. Drawing in"
                       " this configuration is not allowed.";

    if (mStencilRefFront != mStencilRefBack) {
        ErrorInvalidOperation(msg, "stencilFuncSeparate", "reference values");
        return false;
    }

    if (mStencilValueMaskFront != mStencilValueMaskBack) {
        ErrorInvalidOperation(msg, "stencilFuncSeparate", "value masks");
        return false;
    }

    if (mStencilWriteMaskFront != mStencilWriteMaskBack) {
        ErrorInvalidOperation(msg, "stencilMaskSeparate", "write masks");
        return false;
    }

    return true;
}

static inline int32_t
FloorPOT(int32_t x)
{
    MOZ_ASSERT(x > 0);
    int32_t pot = 1;
    while (pot < 0x40000000) {
        if (x < pot*2)
            break;
        pot *= 2;
    }
    return pot;
}

bool
WebGLContext::InitAndValidateGL()
{
    if (!gl)
        return false;

    GLenum error = gl->fGetError();
    if (error != LOCAL_GL_NO_ERROR) {
        GenerateWarning("GL error 0x%x occurred during OpenGL context"
                        " initialization, before WebGL initialization!", error);
        return false;
    }

    mMinCapability = Preferences::GetBool("webgl.min_capability_mode", false);
    mDisableExtensions = Preferences::GetBool("webgl.disable-extensions", false);
    mLoseContextOnMemoryPressure = Preferences::GetBool("webgl.lose-context-on-memory-pressure", false);
    mCanLoseContextInForeground = Preferences::GetBool("webgl.can-lose-context-in-foreground", true);
    mRestoreWhenVisible = Preferences::GetBool("webgl.restore-context-when-visible", true);

    if (MinCapabilityMode())
        mDisableFragHighP = true;

    
    
    mColorWriteMask[0] = 1;
    mColorWriteMask[1] = 1;
    mColorWriteMask[2] = 1;
    mColorWriteMask[3] = 1;
    mDepthWriteMask = 1;
    mColorClearValue[0] = 0.f;
    mColorClearValue[1] = 0.f;
    mColorClearValue[2] = 0.f;
    mColorClearValue[3] = 0.f;
    mDepthClearValue = 1.f;
    mStencilClearValue = 0;
    mStencilRefFront = 0;
    mStencilRefBack = 0;

    











    gl->GetUIntegerv(LOCAL_GL_STENCIL_VALUE_MASK,      &mStencilValueMaskFront);
    gl->GetUIntegerv(LOCAL_GL_STENCIL_BACK_VALUE_MASK, &mStencilValueMaskBack);
    gl->GetUIntegerv(LOCAL_GL_STENCIL_WRITEMASK,       &mStencilWriteMaskFront);
    gl->GetUIntegerv(LOCAL_GL_STENCIL_BACK_WRITEMASK,  &mStencilWriteMaskBack);

    AssertUintParamCorrect(gl, LOCAL_GL_STENCIL_VALUE_MASK,      mStencilValueMaskFront);
    AssertUintParamCorrect(gl, LOCAL_GL_STENCIL_BACK_VALUE_MASK, mStencilValueMaskBack);
    AssertUintParamCorrect(gl, LOCAL_GL_STENCIL_WRITEMASK,       mStencilWriteMaskFront);
    AssertUintParamCorrect(gl, LOCAL_GL_STENCIL_BACK_WRITEMASK,  mStencilWriteMaskBack);

    mDitherEnabled = true;
    mRasterizerDiscardEnabled = false;
    mScissorTestEnabled = false;

    
    mActiveTexture = 0;
    mEmitContextLostErrorOnce = true;
    mWebGLError = LOCAL_GL_NO_ERROR;
    mUnderlyingGLError = LOCAL_GL_NO_ERROR;

    mBound2DTextures.Clear();
    mBoundCubeMapTextures.Clear();
    mBound3DTextures.Clear();

    mBoundArrayBuffer = nullptr;
    mBoundTransformFeedbackBuffer = nullptr;
    mCurrentProgram = nullptr;

    mBoundDrawFramebuffer = nullptr;
    mBoundReadFramebuffer = nullptr;
    mBoundRenderbuffer = nullptr;

    MakeContextCurrent();

    
    if (gl->IsCompatibilityProfile())
        gl->fEnableVertexAttribArray(0);

    if (MinCapabilityMode())
        mGLMaxVertexAttribs = MINVALUE_GL_MAX_VERTEX_ATTRIBS;
    else
        gl->fGetIntegerv(LOCAL_GL_MAX_VERTEX_ATTRIBS, &mGLMaxVertexAttribs);

    if (mGLMaxVertexAttribs < 8) {
        GenerateWarning("GL_MAX_VERTEX_ATTRIBS: %d is < 8!",
                        mGLMaxVertexAttribs);
        return false;
    }

    
    
    
    if (MinCapabilityMode())
        mGLMaxTextureUnits = MINVALUE_GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS;
    else
        gl->fGetIntegerv(LOCAL_GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &mGLMaxTextureUnits);

    if (mGLMaxTextureUnits < 8) {
        GenerateWarning("GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS: %d is < 8!",
                        mGLMaxTextureUnits);
        return false;
    }

    mBound2DTextures.SetLength(mGLMaxTextureUnits);
    mBoundCubeMapTextures.SetLength(mGLMaxTextureUnits);
    mBound3DTextures.SetLength(mGLMaxTextureUnits);

    if (MinCapabilityMode()) {
        mGLMaxTextureSize = MINVALUE_GL_MAX_TEXTURE_SIZE;
        mGLMaxCubeMapTextureSize = MINVALUE_GL_MAX_CUBE_MAP_TEXTURE_SIZE;
        mGLMaxRenderbufferSize = MINVALUE_GL_MAX_RENDERBUFFER_SIZE;
        mGLMaxTextureImageUnits = MINVALUE_GL_MAX_TEXTURE_IMAGE_UNITS;
        mGLMaxVertexTextureImageUnits = MINVALUE_GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS;
        mGLMaxSamples = 1;
    } else {
        gl->fGetIntegerv(LOCAL_GL_MAX_TEXTURE_SIZE, &mGLMaxTextureSize);
        gl->fGetIntegerv(LOCAL_GL_MAX_CUBE_MAP_TEXTURE_SIZE, &mGLMaxCubeMapTextureSize);
        gl->fGetIntegerv(LOCAL_GL_MAX_RENDERBUFFER_SIZE, &mGLMaxRenderbufferSize);
        gl->fGetIntegerv(LOCAL_GL_MAX_TEXTURE_IMAGE_UNITS, &mGLMaxTextureImageUnits);
        gl->fGetIntegerv(LOCAL_GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &mGLMaxVertexTextureImageUnits);

        if (!gl->GetPotentialInteger(LOCAL_GL_MAX_SAMPLES, (GLint*)&mGLMaxSamples))
            mGLMaxSamples = 1;
    }

    
    mGLMaxTextureSizeLog2 = 0;
    int32_t tempSize = mGLMaxTextureSize;
    while (tempSize >>= 1) {
        ++mGLMaxTextureSizeLog2;
    }

    mGLMaxCubeMapTextureSizeLog2 = 0;
    tempSize = mGLMaxCubeMapTextureSize;
    while (tempSize >>= 1) {
        ++mGLMaxCubeMapTextureSizeLog2;
    }

    mGLMaxTextureSize = FloorPOT(mGLMaxTextureSize);
    mGLMaxRenderbufferSize = FloorPOT(mGLMaxRenderbufferSize);

    if (MinCapabilityMode()) {
        mGLMaxFragmentUniformVectors = MINVALUE_GL_MAX_FRAGMENT_UNIFORM_VECTORS;
        mGLMaxVertexUniformVectors = MINVALUE_GL_MAX_VERTEX_UNIFORM_VECTORS;
        mGLMaxVaryingVectors = MINVALUE_GL_MAX_VARYING_VECTORS;
    } else {
        if (gl->IsSupported(gl::GLFeature::ES2_compatibility)) {
            gl->fGetIntegerv(LOCAL_GL_MAX_FRAGMENT_UNIFORM_VECTORS, &mGLMaxFragmentUniformVectors);
            gl->fGetIntegerv(LOCAL_GL_MAX_VERTEX_UNIFORM_VECTORS, &mGLMaxVertexUniformVectors);
            gl->fGetIntegerv(LOCAL_GL_MAX_VARYING_VECTORS, &mGLMaxVaryingVectors);
        } else {
            gl->fGetIntegerv(LOCAL_GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &mGLMaxFragmentUniformVectors);
            mGLMaxFragmentUniformVectors /= 4;
            gl->fGetIntegerv(LOCAL_GL_MAX_VERTEX_UNIFORM_COMPONENTS, &mGLMaxVertexUniformVectors);
            mGLMaxVertexUniformVectors /= 4;

            









            GLint maxVertexOutputComponents = 0;
            GLint maxFragmentInputComponents = 0;

            const bool ok = (gl->GetPotentialInteger(LOCAL_GL_MAX_VERTEX_OUTPUT_COMPONENTS,
                                                     &maxVertexOutputComponents) &&
                             gl->GetPotentialInteger(LOCAL_GL_MAX_FRAGMENT_INPUT_COMPONENTS,
                                                     &maxFragmentInputComponents));

            if (ok) {
                mGLMaxVaryingVectors = std::min(maxVertexOutputComponents,
                                                maxFragmentInputComponents) / 4;
            } else {
                mGLMaxVaryingVectors = 16;
                
                
            }
        }
    }

    
    mMaxFramebufferColorAttachments = 1;

    if (gl->IsCompatibilityProfile()) {
        
        
        gl->fEnable(LOCAL_GL_VERTEX_PROGRAM_POINT_SIZE);

        







        gl->fEnable(LOCAL_GL_POINT_SPRITE);
    }

#ifdef XP_MACOSX
    if (gl->WorkAroundDriverBugs() &&
        gl->Vendor() == gl::GLVendor::ATI &&
        !nsCocoaFeatures::IsAtLeastVersion(10,9))
    {
        
        
        gl->fPointParameterf(LOCAL_GL_POINT_SPRITE_COORD_ORIGIN,
                             LOCAL_GL_LOWER_LEFT);
    }
#endif

    
    NS_ENSURE_TRUE(Preferences::GetRootBranch(), false);

    mBypassShaderValidation = Preferences::GetBool("webgl.bypass-shader-validation",
                                                   mBypassShaderValidation);

    
    if (!ShInitialize()) {
        GenerateWarning("GLSL translator initialization failed!");
        return false;
    }

    
    
    const char* versionStr = (const char*)(gl->fGetString(LOCAL_GL_VERSION));
    mIsMesa = strstr(versionStr, "Mesa");

    
    
    
    error = gl->fGetError();
    if (error != LOCAL_GL_NO_ERROR) {
        GenerateWarning("GL error 0x%x occurred during WebGL context"
                        " initialization!", error);
        return false;
    }

    if (IsWebGL2() &&
        !InitWebGL2())
    {
        
        return false;
    }

    
    for (int32_t index = 0; index < mGLMaxVertexAttribs; ++index) {
        VertexAttrib4f(index, 0, 0, 0, 1);
    }

    mDefaultVertexArray = WebGLVertexArray::Create(this);
    mDefaultVertexArray->mAttribs.SetLength(mGLMaxVertexAttribs);
    mBoundVertexArray = mDefaultVertexArray;

    
    
    
    
    
    
    
    

    if (gl->IsCoreProfile()) {
        MakeContextCurrent();
        mDefaultVertexArray->GenVertexArray();
        mDefaultVertexArray->BindVertexArray();
    }

    if (mLoseContextOnMemoryPressure)
        mContextObserver->RegisterMemoryPressureEvent();

    return true;
}

bool
WebGLContext::ValidateFramebufferTarget(GLenum target,
                                        const char* const info)
{
    bool isValid = true;
    switch (target) {
    case LOCAL_GL_FRAMEBUFFER:
        break;

    case LOCAL_GL_DRAW_FRAMEBUFFER:
    case LOCAL_GL_READ_FRAMEBUFFER:
        isValid = IsWebGL2();
        break;

    default:
        isValid = false;
        break;
    }

    if (MOZ_LIKELY(isValid)) {
        return true;
    }

    ErrorInvalidEnum("%s: Invalid target: %s (0x%04x).", info, EnumName(target),
                     target);
    return false;
}

} 
