#include "precompiled.h"








#include "libGLESv2/validationES.h"
#include "libGLESv2/validationES2.h"
#include "libGLESv2/validationES3.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/Texture.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/FramebufferAttachment.h"
#include "libGLESv2/formatutils.h"
#include "libGLESv2/main.h"
#include "libGLESv2/Query.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/TransformFeedback.h"

#include "common/mathutil.h"
#include "common/utilities.h"

namespace gl
{

bool ValidCap(const Context *context, GLenum cap)
{
    switch (cap)
    {
      case GL_CULL_FACE:
      case GL_POLYGON_OFFSET_FILL:
      case GL_SAMPLE_ALPHA_TO_COVERAGE:
      case GL_SAMPLE_COVERAGE:
      case GL_SCISSOR_TEST:
      case GL_STENCIL_TEST:
      case GL_DEPTH_TEST:
      case GL_BLEND:
      case GL_DITHER:
        return true;
      case GL_PRIMITIVE_RESTART_FIXED_INDEX:
      case GL_RASTERIZER_DISCARD:
        return (context->getClientVersion() >= 3);
      default:
        return false;
    }
}

bool ValidTextureTarget(const Context *context, GLenum target)
{
    switch (target)
    {
      case GL_TEXTURE_2D:
      case GL_TEXTURE_CUBE_MAP:
        return true;

      case GL_TEXTURE_3D:
      case GL_TEXTURE_2D_ARRAY:
        return (context->getClientVersion() >= 3);

      default:
        return false;
    }
}





bool ValidTexture2DDestinationTarget(const Context *context, GLenum target)
{
    switch (target)
    {
      case GL_TEXTURE_2D:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        return true;
      case GL_TEXTURE_2D_ARRAY:
      case GL_TEXTURE_3D:
        return (context->getClientVersion() >= 3);
      default:
        return false;
    }
}

bool ValidFramebufferTarget(GLenum target)
{
    META_ASSERT(GL_DRAW_FRAMEBUFFER_ANGLE == GL_DRAW_FRAMEBUFFER && GL_READ_FRAMEBUFFER_ANGLE == GL_READ_FRAMEBUFFER);

    switch (target)
    {
      case GL_FRAMEBUFFER:      return true;
      case GL_READ_FRAMEBUFFER: return true;
      case GL_DRAW_FRAMEBUFFER: return true;
      default:                  return false;
    }
}

bool ValidBufferTarget(const Context *context, GLenum target)
{
    switch (target)
    {
      case GL_ARRAY_BUFFER:
      case GL_ELEMENT_ARRAY_BUFFER:
        return true;

      case GL_PIXEL_PACK_BUFFER:
      case GL_PIXEL_UNPACK_BUFFER:
        return context->getCaps().extensions.pixelBufferObject;

      case GL_COPY_READ_BUFFER:
      case GL_COPY_WRITE_BUFFER:
      case GL_TRANSFORM_FEEDBACK_BUFFER:
      case GL_UNIFORM_BUFFER:
        return (context->getClientVersion() >= 3);

      default:
        return false;
    }
}

bool ValidBufferParameter(const Context *context, GLenum pname)
{
    switch (pname)
    {
      case GL_BUFFER_USAGE:
      case GL_BUFFER_SIZE:
        return true;

      
      
      case GL_BUFFER_ACCESS_FLAGS:
      case GL_BUFFER_MAPPED:
      case GL_BUFFER_MAP_OFFSET:
      case GL_BUFFER_MAP_LENGTH:
        return (context->getClientVersion() >= 3);

      default:
        return false;
    }
}

bool ValidMipLevel(const Context *context, GLenum target, GLint level)
{
    int maxLevel = 0;
    switch (target)
    {
      case GL_TEXTURE_2D:                  maxLevel = context->getMaximum2DTextureLevel();      break;
      case GL_TEXTURE_CUBE_MAP:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: maxLevel = context->getMaximumCubeTextureLevel();    break;
      case GL_TEXTURE_3D:                  maxLevel = context->getMaximum3DTextureLevel();      break;
      case GL_TEXTURE_2D_ARRAY:            maxLevel = context->getMaximum2DArrayTextureLevel(); break;
      default: UNREACHABLE();
    }

    return level < maxLevel;
}

bool ValidImageSize(const gl::Context *context, GLenum target, GLint level,
                    GLsizei width, GLsizei height, GLsizei depth)
{
    if (level < 0 || width < 0 || height < 0 || depth < 0)
    {
        return false;
    }

    if (!context->getCaps().extensions.textureNPOT &&
        (level != 0 && (!gl::isPow2(width) || !gl::isPow2(height) || !gl::isPow2(depth))))
    {
        return false;
    }

    if (!ValidMipLevel(context, target, level))
    {
        return false;
    }

    return true;
}

bool ValidCompressedImageSize(const gl::Context *context, GLenum internalFormat, GLsizei width, GLsizei height)
{
    if (!IsFormatCompressed(internalFormat))
    {
        return false;
    }

    GLint blockWidth = GetCompressedBlockWidth(internalFormat);
    GLint blockHeight = GetCompressedBlockHeight(internalFormat);
    if (width  < 0 || (width  > blockWidth  && width  % blockWidth  != 0) ||
        height < 0 || (height > blockHeight && height % blockHeight != 0))
    {
        return false;
    }

    return true;
}

bool ValidQueryType(const Context *context, GLenum queryType)
{
    META_ASSERT(GL_ANY_SAMPLES_PASSED == GL_ANY_SAMPLES_PASSED_EXT);
    META_ASSERT(GL_ANY_SAMPLES_PASSED_CONSERVATIVE == GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT);

    switch (queryType)
    {
      case GL_ANY_SAMPLES_PASSED:
      case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
        return true;
      case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
        return (context->getClientVersion() >= 3);
      default:
        return false;
    }
}

bool ValidProgram(const Context *context, GLuint id)
{
    
    
    

    if (context->getProgram(id) != NULL)
    {
        return true;
    }
    else if (context->getShader(id) != NULL)
    {
        
        return gl::error(GL_INVALID_OPERATION, false);
    }
    else
    {
        
        return gl::error(GL_INVALID_VALUE, false);
    }
}

bool ValidateRenderbufferStorageParameters(const gl::Context *context, GLenum target, GLsizei samples,
                                           GLenum internalformat, GLsizei width, GLsizei height,
                                           bool angleExtension)
{
    switch (target)
    {
      case GL_RENDERBUFFER:
        break;
      default:
        return gl::error(GL_INVALID_ENUM, false);
    }

    if (width < 0 || height < 0 || samples < 0)
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    const gl::Caps &caps = context->getCaps();
    if (!gl::IsValidInternalFormat(internalformat, caps.extensions, context->getClientVersion()))
    {
        return gl::error(GL_INVALID_ENUM, false);
    }

    
    
    
    
    if (!gl::IsSizedInternalFormat(internalformat))
    {
        return gl::error(GL_INVALID_ENUM, false);
    }

    GLenum componentType = gl::GetComponentType(internalformat);
    if ((componentType == GL_UNSIGNED_INT || componentType == GL_INT) && samples > 0)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    const TextureCaps &formatCaps = caps.textureCaps.get(internalformat);
    if (!formatCaps.colorRendering && !formatCaps.depthRendering && !formatCaps.stencilRendering)
    {
        return gl::error(GL_INVALID_ENUM, false);
    }

    if (std::max(width, height) > context->getMaximumRenderbufferDimension())
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    
    
    
    
    if (angleExtension)
    {
        if (samples > context->getMaxSupportedSamples())
        {
            return gl::error(GL_INVALID_VALUE, false);
        }
    }
    else
    {
        if (samples > context->getMaxSupportedFormatSamples(internalformat))
        {
            return gl::error(GL_INVALID_VALUE, false);
        }
    }

    GLuint handle = context->getRenderbufferHandle();
    if (handle == 0)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    return true;
}

bool ValidateFramebufferRenderbufferParameters(gl::Context *context, GLenum target, GLenum attachment,
                                               GLenum renderbuffertarget, GLuint renderbuffer)
{
    gl::Framebuffer *framebuffer = context->getTargetFramebuffer(target);
    GLuint framebufferHandle = context->getTargetFramebufferHandle(target);

    if (!framebuffer || (framebufferHandle == 0 && renderbuffer != 0))
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    if (attachment >= GL_COLOR_ATTACHMENT0_EXT && attachment <= GL_COLOR_ATTACHMENT15_EXT)
    {
        const unsigned int colorAttachment = (attachment - GL_COLOR_ATTACHMENT0_EXT);

        if (colorAttachment >= context->getMaximumRenderTargets())
        {
            return gl::error(GL_INVALID_VALUE, false);
        }
    }
    else
    {
        switch (attachment)
        {
          case GL_DEPTH_ATTACHMENT:
            break;
          case GL_STENCIL_ATTACHMENT:
            break;
          case GL_DEPTH_STENCIL_ATTACHMENT:
            if (context->getClientVersion() < 3)
            {
                return gl::error(GL_INVALID_ENUM, false);
            }
            break;
          default:
            return gl::error(GL_INVALID_ENUM, false);
        }
    }

    
    
    
    
    if (renderbuffer != 0)
    {
        if (!context->getRenderbuffer(renderbuffer))
        {
            return gl::error(GL_INVALID_OPERATION, false);
        }
    }

    return true;
}

static bool IsPartialBlit(gl::Context *context, gl::FramebufferAttachment *readBuffer, gl::FramebufferAttachment *writeBuffer,
                          GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                          GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1)
{
    if (srcX0 != 0 || srcY0 != 0 || dstX0 != 0 || dstY0 != 0 ||
        dstX1 != writeBuffer->getWidth() || dstY1 != writeBuffer->getHeight() ||
        srcX1 != readBuffer->getWidth() || srcY1 != readBuffer->getHeight())
    {
        return true;
    }
    else if (context->isScissorTestEnabled())
    {
        int scissorX, scissorY, scissorWidth, scissorHeight;
        context->getScissorParams(&scissorX, &scissorY, &scissorWidth, &scissorHeight);

        return scissorX > 0 || scissorY > 0 ||
               scissorWidth < writeBuffer->getWidth() ||
               scissorHeight < writeBuffer->getHeight();
    }
    else
    {
        return false;
    }
}

bool ValidateBlitFramebufferParameters(gl::Context *context, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                                       GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask,
                                       GLenum filter, bool fromAngleExtension)
{
    switch (filter)
    {
      case GL_NEAREST:
        break;
      case GL_LINEAR:
        if (fromAngleExtension)
        {
            return gl::error(GL_INVALID_ENUM, false);
        }
        break;
      default:
        return gl::error(GL_INVALID_ENUM, false);
    }

    if ((mask & ~(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)) != 0)
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    if (mask == 0)
    {
        
        
        return false;
    }

    if (fromAngleExtension && (srcX1 - srcX0 != dstX1 - dstX0 || srcY1 - srcY0 != dstY1 - dstY0))
    {
        ERR("Scaling and flipping in BlitFramebufferANGLE not supported by this implementation.");
        return gl::error(GL_INVALID_OPERATION, false);
    }

    
    
    if ((mask & ~GL_COLOR_BUFFER_BIT) != 0 && filter != GL_NEAREST)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    if (context->getReadFramebufferHandle() == context->getDrawFramebufferHandle())
    {
        if (fromAngleExtension)
        {
            ERR("Blits with the same source and destination framebuffer are not supported by this "
                "implementation.");
        }
        return gl::error(GL_INVALID_OPERATION, false);
    }

    gl::Framebuffer *readFramebuffer = context->getReadFramebuffer();
    gl::Framebuffer *drawFramebuffer = context->getDrawFramebuffer();
    if (!readFramebuffer || readFramebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE ||
        !drawFramebuffer || drawFramebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE)
    {
        return gl::error(GL_INVALID_FRAMEBUFFER_OPERATION, false);
    }

    if (drawFramebuffer->getSamples() != 0)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    bool sameBounds = srcX0 == dstX0 && srcY0 == dstY0 && srcX1 == dstX1 && srcY1 == dstY1;

    if (mask & GL_COLOR_BUFFER_BIT)
    {
        gl::FramebufferAttachment *readColorBuffer = readFramebuffer->getReadColorbuffer();
        gl::FramebufferAttachment *drawColorBuffer = drawFramebuffer->getFirstColorbuffer();

        if (readColorBuffer && drawColorBuffer)
        {
            GLenum readInternalFormat = readColorBuffer->getActualFormat();
            GLenum readComponentType = gl::GetComponentType(readInternalFormat);

            for (unsigned int i = 0; i < gl::IMPLEMENTATION_MAX_DRAW_BUFFERS; i++)
            {
                if (drawFramebuffer->isEnabledColorAttachment(i))
                {
                    GLenum drawInternalFormat = drawFramebuffer->getColorbuffer(i)->getActualFormat();
                    GLenum drawComponentType = gl::GetComponentType(drawInternalFormat);

                    
                    
                    
                    
                    if ( (readComponentType == GL_UNSIGNED_NORMALIZED || readComponentType == GL_SIGNED_NORMALIZED) &&
                        !(drawComponentType == GL_UNSIGNED_NORMALIZED || drawComponentType == GL_SIGNED_NORMALIZED))
                    {
                        return gl::error(GL_INVALID_OPERATION, false);
                    }

                    if (readComponentType == GL_UNSIGNED_INT && drawComponentType != GL_UNSIGNED_INT)
                    {
                        return gl::error(GL_INVALID_OPERATION, false);
                    }

                    if (readComponentType == GL_INT && drawComponentType != GL_INT)
                    {
                        return gl::error(GL_INVALID_OPERATION, false);
                    }

                    if (readColorBuffer->getSamples() > 0 && (readInternalFormat != drawInternalFormat || !sameBounds))
                    {
                        return gl::error(GL_INVALID_OPERATION, false);
                    }
                }
            }

            if ((readComponentType == GL_INT || readComponentType == GL_UNSIGNED_INT) && filter == GL_LINEAR)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }

            if (fromAngleExtension)
            {
                const GLenum readColorbufferType = readFramebuffer->getReadColorbufferType();
                if (readColorbufferType != GL_TEXTURE_2D && readColorbufferType != GL_RENDERBUFFER)
                {
                    return gl::error(GL_INVALID_OPERATION, false);
                }

                for (unsigned int colorAttachment = 0; colorAttachment < gl::IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
                {
                    if (drawFramebuffer->isEnabledColorAttachment(colorAttachment))
                    {
                        if (drawFramebuffer->getColorbufferType(colorAttachment) != GL_TEXTURE_2D &&
                            drawFramebuffer->getColorbufferType(colorAttachment) != GL_RENDERBUFFER)
                        {
                            return gl::error(GL_INVALID_OPERATION, false);
                        }

                        if (drawFramebuffer->getColorbuffer(colorAttachment)->getActualFormat() != readColorBuffer->getActualFormat())
                        {
                            return gl::error(GL_INVALID_OPERATION, false);
                        }
                    }
                }
                if (readFramebuffer->getSamples() != 0 && IsPartialBlit(context, readColorBuffer, drawColorBuffer,
                                                                        srcX0, srcY0, srcX1, srcY1,
                                                                        dstX0, dstY0, dstX1, dstY1))
                {
                    return gl::error(GL_INVALID_OPERATION, false);
                }
            }
        }
    }

    if (mask & GL_DEPTH_BUFFER_BIT)
    {
        gl::FramebufferAttachment *readDepthBuffer = readFramebuffer->getDepthbuffer();
        gl::FramebufferAttachment *drawDepthBuffer = drawFramebuffer->getDepthbuffer();

        if (readDepthBuffer && drawDepthBuffer)
        {
            if (readDepthBuffer->getActualFormat() != drawDepthBuffer->getActualFormat())
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }

            if (readDepthBuffer->getSamples() > 0 && !sameBounds)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }

            if (fromAngleExtension)
            {
                if (IsPartialBlit(context, readDepthBuffer, drawDepthBuffer,
                                  srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1))
                {
                    ERR("Only whole-buffer depth and stencil blits are supported by this implementation.");
                    return gl::error(GL_INVALID_OPERATION, false); 
                }

                if (readDepthBuffer->getSamples() != 0 || drawDepthBuffer->getSamples() != 0)
                {
                    return gl::error(GL_INVALID_OPERATION, false);
                }
            }
        }
    }

    if (mask & GL_STENCIL_BUFFER_BIT)
    {
        gl::FramebufferAttachment *readStencilBuffer = readFramebuffer->getStencilbuffer();
        gl::FramebufferAttachment *drawStencilBuffer = drawFramebuffer->getStencilbuffer();

        if (readStencilBuffer && drawStencilBuffer)
        {
            if (readStencilBuffer->getActualFormat() != drawStencilBuffer->getActualFormat())
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }

            if (readStencilBuffer->getSamples() > 0 && !sameBounds)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }

            if (fromAngleExtension)
            {
                if (IsPartialBlit(context, readStencilBuffer, drawStencilBuffer,
                                  srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1))
                {
                    ERR("Only whole-buffer depth and stencil blits are supported by this implementation.");
                    return gl::error(GL_INVALID_OPERATION, false); 
                }

                if (readStencilBuffer->getSamples() != 0 || drawStencilBuffer->getSamples() != 0)
                {
                    return gl::error(GL_INVALID_OPERATION, false);
                }
            }
        }
    }

    return true;
}

bool ValidateGetVertexAttribParameters(GLenum pname, int clientVersion)
{
    switch (pname)
    {
      case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
      case GL_VERTEX_ATTRIB_ARRAY_SIZE:
      case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
      case GL_VERTEX_ATTRIB_ARRAY_TYPE:
      case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
      case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
      case GL_CURRENT_VERTEX_ATTRIB:
        return true;

      case GL_VERTEX_ATTRIB_ARRAY_DIVISOR:
        
        
        META_ASSERT(GL_VERTEX_ATTRIB_ARRAY_DIVISOR == GL_VERTEX_ATTRIB_ARRAY_DIVISOR_ANGLE);
        return true;

      case GL_VERTEX_ATTRIB_ARRAY_INTEGER:
        return ((clientVersion >= 3) ? true : gl::error(GL_INVALID_ENUM, false));

      default:
        return gl::error(GL_INVALID_ENUM, false);
    }
}

bool ValidateTexParamParameters(gl::Context *context, GLenum pname, GLint param)
{
    switch (pname)
    {
      case GL_TEXTURE_WRAP_R:
      case GL_TEXTURE_SWIZZLE_R:
      case GL_TEXTURE_SWIZZLE_G:
      case GL_TEXTURE_SWIZZLE_B:
      case GL_TEXTURE_SWIZZLE_A:
      case GL_TEXTURE_BASE_LEVEL:
      case GL_TEXTURE_MAX_LEVEL:
      case GL_TEXTURE_COMPARE_MODE:
      case GL_TEXTURE_COMPARE_FUNC:
      case GL_TEXTURE_MIN_LOD:
      case GL_TEXTURE_MAX_LOD:
        if (context->getClientVersion() < 3)
        {
            return gl::error(GL_INVALID_ENUM, false);
        }
        break;

      default: break;
    }

    switch (pname)
    {
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_WRAP_R:
        switch (param)
        {
          case GL_REPEAT:
          case GL_CLAMP_TO_EDGE:
          case GL_MIRRORED_REPEAT:
            return true;
          default:
            return gl::error(GL_INVALID_ENUM, false);
        }

      case GL_TEXTURE_MIN_FILTER:
        switch (param)
        {
          case GL_NEAREST:
          case GL_LINEAR:
          case GL_NEAREST_MIPMAP_NEAREST:
          case GL_LINEAR_MIPMAP_NEAREST:
          case GL_NEAREST_MIPMAP_LINEAR:
          case GL_LINEAR_MIPMAP_LINEAR:
            return true;
          default:
            return gl::error(GL_INVALID_ENUM, false);
        }
        break;

      case GL_TEXTURE_MAG_FILTER:
        switch (param)
        {
          case GL_NEAREST:
          case GL_LINEAR:
            return true;
          default:
            return gl::error(GL_INVALID_ENUM, false);
        }
        break;

      case GL_TEXTURE_USAGE_ANGLE:
        switch (param)
        {
          case GL_NONE:
          case GL_FRAMEBUFFER_ATTACHMENT_ANGLE:
            return true;
          default:
            return gl::error(GL_INVALID_ENUM, false);
        }
        break;

      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
        if (!context->getCaps().extensions.textureFilterAnisotropic)
        {
            return gl::error(GL_INVALID_ENUM, false);
        }

        
        if (param < 1)
        {
            return gl::error(GL_INVALID_VALUE, false);
        }
        return true;

      case GL_TEXTURE_MIN_LOD:
      case GL_TEXTURE_MAX_LOD:
        
        return true;

      case GL_TEXTURE_COMPARE_MODE:
        
        switch (param)
        {
          case GL_NONE:
          case GL_COMPARE_REF_TO_TEXTURE:
            return true;
          default:
            return gl::error(GL_INVALID_ENUM, false);
        }
        break;

      case GL_TEXTURE_COMPARE_FUNC:
        
        switch (param)
        {
          case GL_LEQUAL:
          case GL_GEQUAL:
          case GL_LESS:
          case GL_GREATER:
          case GL_EQUAL:
          case GL_NOTEQUAL:
          case GL_ALWAYS:
          case GL_NEVER:
            return true;
          default:
            return gl::error(GL_INVALID_ENUM, false);
        }
        break;

      case GL_TEXTURE_SWIZZLE_R:
      case GL_TEXTURE_SWIZZLE_G:
      case GL_TEXTURE_SWIZZLE_B:
      case GL_TEXTURE_SWIZZLE_A:
        switch (param)
        {
          case GL_RED:
          case GL_GREEN:
          case GL_BLUE:
          case GL_ALPHA:
          case GL_ZERO:
          case GL_ONE:
            return true;
          default:
            return gl::error(GL_INVALID_ENUM, false);
        }
        break;

      case GL_TEXTURE_BASE_LEVEL:
      case GL_TEXTURE_MAX_LEVEL:
        if (param < 0)
        {
            return gl::error(GL_INVALID_VALUE, false);
        }
        return true;

      default:
        return gl::error(GL_INVALID_ENUM, false);
    }
}

bool ValidateSamplerObjectParameter(GLenum pname)
{
    switch (pname)
    {
      case GL_TEXTURE_MIN_FILTER:
      case GL_TEXTURE_MAG_FILTER:
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_WRAP_R:
      case GL_TEXTURE_MIN_LOD:
      case GL_TEXTURE_MAX_LOD:
      case GL_TEXTURE_COMPARE_MODE:
      case GL_TEXTURE_COMPARE_FUNC:
        return true;

      default:
        return gl::error(GL_INVALID_ENUM, false);
    }
}

bool ValidateReadPixelsParameters(gl::Context *context, GLint x, GLint y, GLsizei width, GLsizei height,
                                  GLenum format, GLenum type, GLsizei *bufSize, GLvoid *pixels)
{
    gl::Framebuffer *framebuffer = context->getReadFramebuffer();
    ASSERT(framebuffer);

    if (framebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE)
    {
        return gl::error(GL_INVALID_FRAMEBUFFER_OPERATION, false);
    }

    if (context->getReadFramebufferHandle() != 0 && framebuffer->getSamples() != 0)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    if (!framebuffer->getReadColorbuffer())
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    GLenum currentInternalFormat, currentFormat, currentType;
    GLuint clientVersion = context->getClientVersion();

    context->getCurrentReadFormatType(&currentInternalFormat, &currentFormat, &currentType);

    bool validReadFormat = (clientVersion < 3) ? ValidES2ReadFormatType(context, format, type) :
                                                 ValidES3ReadFormatType(context, currentInternalFormat, format, type);

    if (!(currentFormat == format && currentType == type) && !validReadFormat)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    GLenum sizedInternalFormat = IsSizedInternalFormat(format) ? format
                                                               : GetSizedInternalFormat(format, type);

    GLsizei outputPitch = GetRowPitch(sizedInternalFormat, type, width, context->getPackAlignment());
    
    if (bufSize)
    {
        int requiredSize = outputPitch * height;
        if (requiredSize > *bufSize)
        {
            return gl::error(GL_INVALID_OPERATION, false);
        }
    }

    return true;
}

bool ValidateBeginQuery(gl::Context *context, GLenum target, GLuint id)
{
    if (!ValidQueryType(context, target))
    {
        return gl::error(GL_INVALID_ENUM, false);
    }

    if (id == 0)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    
    
    
    
    
    
    

    
    
    
    
    
    
    
    if (context->isQueryActive())
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    Query *queryObject = context->getQuery(id, true, target);

    
    if (!queryObject)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    
    if (queryObject->getType() != target)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    return true;
}

bool ValidateEndQuery(gl::Context *context, GLenum target)
{
    if (!ValidQueryType(context, target))
    {
        return gl::error(GL_INVALID_ENUM, false);
    }

    const Query *queryObject = context->getActiveQuery(target);

    if (queryObject == NULL)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    if (!queryObject->isStarted())
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    return true;
}

static bool ValidateUniformCommonBase(gl::Context *context, GLenum targetUniformType,
                                      GLint location, GLsizei count, LinkedUniform **uniformOut)
{
    if (count < 0)
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    gl::ProgramBinary *programBinary = context->getCurrentProgramBinary();
    if (!programBinary)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    if (location == -1)
    {
        
        return false;
    }

    if (!programBinary->isValidUniformLocation(location))
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    LinkedUniform *uniform = programBinary->getUniformByLocation(location);

    
    if (uniform->elementCount() == 1 && count > 1)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    *uniformOut = uniform;
    return true;
}

bool ValidateUniform(gl::Context *context, GLenum uniformType, GLint location, GLsizei count)
{
    
    if (VariableComponentType(uniformType) == GL_UNSIGNED_INT && context->getClientVersion() < 3)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    LinkedUniform *uniform = NULL;
    if (!ValidateUniformCommonBase(context, uniformType, location, count, &uniform))
    {
        return false;
    }

    GLenum targetBoolType = VariableBoolVectorType(uniformType);
    bool samplerUniformCheck = (IsSampler(uniform->type) && uniformType == GL_INT);
    if (!samplerUniformCheck && uniformType != uniform->type && targetBoolType != uniform->type)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    return true;
}

bool ValidateUniformMatrix(gl::Context *context, GLenum matrixType, GLint location, GLsizei count,
                           GLboolean transpose)
{
    
    int rows = VariableRowCount(matrixType);
    int cols = VariableColumnCount(matrixType);
    if (rows != cols && context->getClientVersion() < 3)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    if (transpose != GL_FALSE && context->getClientVersion() < 3)
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    LinkedUniform *uniform = NULL;
    if (!ValidateUniformCommonBase(context, matrixType, location, count, &uniform))
    {
        return false;
    }

    if (uniform->type != matrixType)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    return true;
}

bool ValidateStateQuery(gl::Context *context, GLenum pname, GLenum *nativeType, unsigned int *numParams)
{
    if (!context->getQueryParameterInfo(pname, nativeType, numParams))
    {
        return gl::error(GL_INVALID_ENUM, false);
    }

    if (pname >= GL_DRAW_BUFFER0 && pname <= GL_DRAW_BUFFER15)
    {
        unsigned int colorAttachment = (pname - GL_DRAW_BUFFER0);

        if (colorAttachment >= context->getMaximumRenderTargets())
        {
            return gl::error(GL_INVALID_OPERATION, false);
        }
    }

    switch (pname)
    {
      case GL_TEXTURE_BINDING_2D:
      case GL_TEXTURE_BINDING_CUBE_MAP:
      case GL_TEXTURE_BINDING_3D:
      case GL_TEXTURE_BINDING_2D_ARRAY:
        if (context->getActiveSampler() >= context->getMaximumCombinedTextureImageUnits())
        {
            return gl::error(GL_INVALID_OPERATION, false);
        }
        break;

      case GL_IMPLEMENTATION_COLOR_READ_TYPE:
      case GL_IMPLEMENTATION_COLOR_READ_FORMAT:
        {
            Framebuffer *framebuffer = context->getReadFramebuffer();
            ASSERT(framebuffer);
            if (framebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }

            FramebufferAttachment *attachment = framebuffer->getReadColorbuffer();
            if (!attachment)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }
        }
        break;

      default:
        break;
    }

    
    if (numParams == 0)
    {
        return false;
    }

    return true;
}

bool ValidateCopyTexImageParametersBase(gl::Context* context, GLenum target, GLint level, GLenum internalformat, bool isSubImage,
                                        GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height,
                                        GLint border, GLenum *textureFormatOut)
{

    if (!ValidTexture2DDestinationTarget(context, target))
    {
        return gl::error(GL_INVALID_ENUM, false);
    }

    if (level < 0 || xoffset < 0 || yoffset < 0 || zoffset < 0 || width < 0 || height < 0)
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    if (std::numeric_limits<GLsizei>::max() - xoffset < width || std::numeric_limits<GLsizei>::max() - yoffset < height)
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    if (border != 0)
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    if (!ValidMipLevel(context, target, level))
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    gl::Framebuffer *framebuffer = context->getReadFramebuffer();
    if (framebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE)
    {
        return gl::error(GL_INVALID_FRAMEBUFFER_OPERATION, false);
    }

    if (context->getReadFramebufferHandle() != 0 && framebuffer->getSamples() != 0)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    gl::Texture *texture = NULL;
    GLenum textureInternalFormat = GL_NONE;
    bool textureCompressed = false;
    bool textureIsDepth = false;
    GLint textureLevelWidth = 0;
    GLint textureLevelHeight = 0;
    GLint textureLevelDepth = 0;
    int maxDimension = 0;

    switch (target)
    {
      case GL_TEXTURE_2D:
        {
            gl::Texture2D *texture2d = context->getTexture2D();
            if (texture2d)
            {
                textureInternalFormat = texture2d->getInternalFormat(level);
                textureCompressed = texture2d->isCompressed(level);
                textureIsDepth = texture2d->isDepth(level);
                textureLevelWidth = texture2d->getWidth(level);
                textureLevelHeight = texture2d->getHeight(level);
                textureLevelDepth = 1;
                texture = texture2d;
                maxDimension = context->getMaximum2DTextureDimension();
            }
        }
        break;

      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        {
            gl::TextureCubeMap *textureCube = context->getTextureCubeMap();
            if (textureCube)
            {
                textureInternalFormat = textureCube->getInternalFormat(target, level);
                textureCompressed = textureCube->isCompressed(target, level);
                textureIsDepth = false;
                textureLevelWidth = textureCube->getWidth(target, level);
                textureLevelHeight = textureCube->getHeight(target, level);
                textureLevelDepth = 1;
                texture = textureCube;
                maxDimension = context->getMaximumCubeTextureDimension();
            }
        }
        break;

      case GL_TEXTURE_2D_ARRAY:
        {
            gl::Texture2DArray *texture2dArray = context->getTexture2DArray();
            if (texture2dArray)
            {
                textureInternalFormat = texture2dArray->getInternalFormat(level);
                textureCompressed = texture2dArray->isCompressed(level);
                textureIsDepth = texture2dArray->isDepth(level);
                textureLevelWidth = texture2dArray->getWidth(level);
                textureLevelHeight = texture2dArray->getHeight(level);
                textureLevelDepth = texture2dArray->getLayers(level);
                texture = texture2dArray;
                maxDimension = context->getMaximum2DTextureDimension();
            }
        }
        break;

      case GL_TEXTURE_3D:
        {
            gl::Texture3D *texture3d = context->getTexture3D();
            if (texture3d)
            {
                textureInternalFormat = texture3d->getInternalFormat(level);
                textureCompressed = texture3d->isCompressed(level);
                textureIsDepth = texture3d->isDepth(level);
                textureLevelWidth = texture3d->getWidth(level);
                textureLevelHeight = texture3d->getHeight(level);
                textureLevelDepth = texture3d->getDepth(level);
                texture = texture3d;
                maxDimension = context->getMaximum3DTextureDimension();
            }
        }
        break;

      default:
        return gl::error(GL_INVALID_ENUM, false);
    }

    if (!texture)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    if (texture->isImmutable() && !isSubImage)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    if (textureIsDepth)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    if (textureCompressed)
    {
        GLint blockWidth = GetCompressedBlockWidth(textureInternalFormat);
        GLint blockHeight = GetCompressedBlockHeight(textureInternalFormat);

        if (((width % blockWidth) != 0 && width != textureLevelWidth) ||
            ((height % blockHeight) != 0 && height != textureLevelHeight))
        {
            return gl::error(GL_INVALID_OPERATION, false);
        }
    }

    if (isSubImage)
    {
        if (xoffset + width > textureLevelWidth ||
            yoffset + height > textureLevelHeight ||
            zoffset >= textureLevelDepth)
        {
            return gl::error(GL_INVALID_VALUE, false);
        }
    }
    else
    {
        if (IsCubemapTextureTarget(target) && width != height)
        {
            return gl::error(GL_INVALID_VALUE, false);
        }

        if (!IsValidInternalFormat(internalformat, context->getCaps().extensions, context->getClientVersion()))
        {
            return gl::error(GL_INVALID_ENUM, false);
        }

        int maxLevelDimension = (maxDimension >> level);
        if (static_cast<int>(width) > maxLevelDimension || static_cast<int>(height) > maxLevelDimension)
        {
            return gl::error(GL_INVALID_VALUE, false);
        }
    }

    *textureFormatOut = textureInternalFormat;
    return true;
}

static bool ValidateDrawBase(const gl::Context *context, GLenum mode, GLsizei count)
{
    switch (mode)
    {
      case GL_POINTS:
      case GL_LINES:
      case GL_LINE_LOOP:
      case GL_LINE_STRIP:
      case GL_TRIANGLES:
      case GL_TRIANGLE_STRIP:
      case GL_TRIANGLE_FAN:
        break;
      default:
        return gl::error(GL_INVALID_ENUM, false);
    }

    if (count < 0)
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    
    if (context->hasMappedBuffer(GL_ARRAY_BUFFER))
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    const gl::DepthStencilState &depthStencilState = context->getDepthStencilState();
    if (depthStencilState.stencilWritemask != depthStencilState.stencilBackWritemask ||
        context->getStencilRef() != context->getStencilBackRef() ||
        depthStencilState.stencilMask != depthStencilState.stencilBackMask)
    {
        
        
        ERR("This ANGLE implementation does not support separate front/back stencil "
            "writemasks, reference values, or stencil mask values.");
        return gl::error(GL_INVALID_OPERATION, false);
    }

    const gl::Framebuffer *fbo = context->getDrawFramebuffer();
    if (!fbo || fbo->completeness() != GL_FRAMEBUFFER_COMPLETE)
    {
        return gl::error(GL_INVALID_FRAMEBUFFER_OPERATION, false);
    }

    
    return (count > 0);
}

bool ValidateDrawArrays(const gl::Context *context, GLenum mode, GLint first, GLsizei count)
{
    if (first < 0)
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    gl::TransformFeedback *curTransformFeedback = context->getCurrentTransformFeedback();
    if (curTransformFeedback && curTransformFeedback->isStarted() && !curTransformFeedback->isPaused() &&
        curTransformFeedback->getDrawMode() != mode)
    {
        
        
        
        return gl::error(GL_INVALID_OPERATION, false);
    }

    if (!ValidateDrawBase(context, mode, count))
    {
        return false;
    }

    return true;
}

bool ValidateDrawArraysInstanced(const gl::Context *context, GLenum mode, GLint first, GLsizei count, GLsizei primcount)
{
    if (primcount < 0)
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    if (!ValidateDrawArrays(context, mode, first, count))
    {
        return false;
    }

    
    return (primcount > 0);
}

bool ValidateDrawElements(const gl::Context *context, GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
    switch (type)
    {
      case GL_UNSIGNED_BYTE:
      case GL_UNSIGNED_SHORT:
        break;
      case GL_UNSIGNED_INT:
        if (!context->getCaps().extensions.elementIndexUint)
        {
            return gl::error(GL_INVALID_ENUM, false);
        }
        break;
      default:
        return gl::error(GL_INVALID_ENUM, false);
    }

    gl::TransformFeedback *curTransformFeedback = context->getCurrentTransformFeedback();
    if (curTransformFeedback && curTransformFeedback->isStarted() && !curTransformFeedback->isPaused())
    {
        
        
        return gl::error(GL_INVALID_OPERATION, false);
    }

    
    if (context->hasMappedBuffer(GL_ELEMENT_ARRAY_BUFFER))
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    if (!ValidateDrawBase(context, mode, count))
    {
        return false;
    }

    return true;
}

bool ValidateDrawElementsInstanced(const gl::Context *context, GLenum mode, GLsizei count, GLenum type,
                                   const GLvoid *indices, GLsizei primcount)
{
    if (primcount < 0)
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    if (!ValidateDrawElements(context, mode, count, type, indices))
    {
        return false;
    }

    
    return (primcount > 0);
}

}
