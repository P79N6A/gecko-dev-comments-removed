








#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/main.h"
#include "libGLESv2/formatutils.h"
#include "libGLESv2/Texture.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/FramebufferAttachment.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/RenderTarget.h"

#include "common/utilities.h"

namespace gl
{

Framebuffer::Framebuffer(rx::Renderer *renderer, GLuint id)
    : mRenderer(renderer),
      mId(id),
      mReadBufferState(GL_COLOR_ATTACHMENT0_EXT),
      mDepthbuffer(NULL),
      mStencilbuffer(NULL)
{
    for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        mColorbuffers[colorAttachment] = NULL;
        mDrawBufferStates[colorAttachment] = GL_NONE;
    }
    mDrawBufferStates[0] = GL_COLOR_ATTACHMENT0_EXT;
}

Framebuffer::~Framebuffer()
{
    for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        SafeDelete(mColorbuffers[colorAttachment]);
    }
    SafeDelete(mDepthbuffer);
    SafeDelete(mStencilbuffer);
}

FramebufferAttachment *Framebuffer::createAttachment(GLenum type, GLuint handle, GLint level, GLint layer) const
{
    if (handle == 0)
    {
        return NULL;
    }

    gl::Context *context = gl::getContext();

    switch (type)
    {
      case GL_NONE:
        return NULL;

      case GL_RENDERBUFFER:
        return new RenderbufferAttachment(context->getRenderbuffer(handle));

      case GL_TEXTURE_2D:
        {
            Texture *texture = context->getTexture(handle);
            if (texture && texture->getTarget() == GL_TEXTURE_2D)
            {
                Texture2D *tex2D = static_cast<Texture2D*>(texture);
                return new Texture2DAttachment(tex2D, level);
            }
            else
            {
                return NULL;
            }
        }

      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        {
            Texture *texture = context->getTexture(handle);
            if (texture && texture->getTarget() == GL_TEXTURE_CUBE_MAP)
            {
                TextureCubeMap *texCube = static_cast<TextureCubeMap*>(texture);
                return new TextureCubeMapAttachment(texCube, type, level);
            }
            else
            {
                return NULL;
            }
        }

      case GL_TEXTURE_3D:
        {
            Texture *texture = context->getTexture(handle);
            if (texture && texture->getTarget() == GL_TEXTURE_3D)
            {
                Texture3D *tex3D = static_cast<Texture3D*>(texture);
                return new Texture3DAttachment(tex3D, level, layer);
            }
            else
            {
                return NULL;
            }
        }

      case GL_TEXTURE_2D_ARRAY:
        {
            Texture *texture = context->getTexture(handle);
            if (texture && texture->getTarget() == GL_TEXTURE_2D_ARRAY)
            {
                Texture2DArray *tex2DArray = static_cast<Texture2DArray*>(texture);
                return new Texture2DArrayAttachment(tex2DArray, level, layer);
            }
            else
            {
                return NULL;
            }
        }

      default:
        UNREACHABLE();
        return NULL;
    }
}

void Framebuffer::setColorbuffer(unsigned int colorAttachment, GLenum type, GLuint colorbuffer, GLint level, GLint layer)
{
    ASSERT(colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS);
    SafeDelete(mColorbuffers[colorAttachment]);
    mColorbuffers[colorAttachment] = createAttachment(type, colorbuffer, level, layer);
}

void Framebuffer::setDepthbuffer(GLenum type, GLuint depthbuffer, GLint level, GLint layer)
{
    SafeDelete(mDepthbuffer);
    mDepthbuffer = createAttachment(type, depthbuffer, level, layer);
}

void Framebuffer::setStencilbuffer(GLenum type, GLuint stencilbuffer, GLint level, GLint layer)
{
    SafeDelete(mStencilbuffer);
    mStencilbuffer = createAttachment(type, stencilbuffer, level, layer);
}

void Framebuffer::setDepthStencilBuffer(GLenum type, GLuint depthStencilBuffer, GLint level, GLint layer)
{
    FramebufferAttachment *attachment = createAttachment(type, depthStencilBuffer, level, layer);

    SafeDelete(mDepthbuffer);
    SafeDelete(mStencilbuffer);

    
    if (attachment && attachment->getDepthSize() > 0 && attachment->getStencilSize() > 0)
    {
        mDepthbuffer = attachment;

        
        
        mStencilbuffer = createAttachment(type, depthStencilBuffer, level, layer);
    }
}

void Framebuffer::detachTexture(GLuint textureId)
{
    for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        FramebufferAttachment *attachment = mColorbuffers[colorAttachment];

        if (attachment && attachment->isTextureWithId(textureId))
        {
            SafeDelete(mColorbuffers[colorAttachment]);
        }
    }

    if (mDepthbuffer && mDepthbuffer->isTextureWithId(textureId))
    {
        SafeDelete(mDepthbuffer);
    }

    if (mStencilbuffer && mStencilbuffer->isTextureWithId(textureId))
    {
        SafeDelete(mStencilbuffer);
    }
}

void Framebuffer::detachRenderbuffer(GLuint renderbufferId)
{
    for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        FramebufferAttachment *attachment = mColorbuffers[colorAttachment];

        if (attachment && attachment->isRenderbufferWithId(renderbufferId))
        {
            SafeDelete(mColorbuffers[colorAttachment]);
        }
    }

    if (mDepthbuffer && mDepthbuffer->isRenderbufferWithId(renderbufferId))
    {
        SafeDelete(mDepthbuffer);
    }

    if (mStencilbuffer && mStencilbuffer->isRenderbufferWithId(renderbufferId))
    {
        SafeDelete(mStencilbuffer);
    }
}

FramebufferAttachment *Framebuffer::getColorbuffer(unsigned int colorAttachment) const
{
    ASSERT(colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS);
    return mColorbuffers[colorAttachment];
}

FramebufferAttachment *Framebuffer::getDepthbuffer() const
{
    return mDepthbuffer;
}

FramebufferAttachment *Framebuffer::getStencilbuffer() const
{
    return mStencilbuffer;
}

FramebufferAttachment *Framebuffer::getDepthStencilBuffer() const
{
    return (hasValidDepthStencil() ? mDepthbuffer : NULL);
}

FramebufferAttachment *Framebuffer::getDepthOrStencilbuffer() const
{
    FramebufferAttachment *depthstencilbuffer = mDepthbuffer;
    
    if (!depthstencilbuffer)
    {
        depthstencilbuffer = mStencilbuffer;
    }

    return depthstencilbuffer;
}

FramebufferAttachment *Framebuffer::getReadColorbuffer() const
{
    
    return mColorbuffers[0];
}

GLenum Framebuffer::getReadColorbufferType() const
{
    
    return (mColorbuffers[0] ? mColorbuffers[0]->type() : GL_NONE);
}

FramebufferAttachment *Framebuffer::getFirstColorbuffer() const
{
    for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        if (mColorbuffers[colorAttachment])
        {
            return mColorbuffers[colorAttachment];
        }
    }

    return NULL;
}

FramebufferAttachment *Framebuffer::getAttachment(GLenum attachment) const
{
    if (attachment >= GL_COLOR_ATTACHMENT0 && attachment <= GL_COLOR_ATTACHMENT15)
    {
        return getColorbuffer(attachment - GL_COLOR_ATTACHMENT0);
    }
    else
    {
        switch (attachment)
        {
          case GL_DEPTH_ATTACHMENT:
            return getDepthbuffer();
          case GL_STENCIL_ATTACHMENT:
            return getStencilbuffer();
          case GL_DEPTH_STENCIL_ATTACHMENT:
            return getDepthStencilBuffer();
          default:
            UNREACHABLE();
            return NULL;
        }
    }
}

GLenum Framebuffer::getDrawBufferState(unsigned int colorAttachment) const
{
    return mDrawBufferStates[colorAttachment];
}

void Framebuffer::setDrawBufferState(unsigned int colorAttachment, GLenum drawBuffer)
{
    mDrawBufferStates[colorAttachment] = drawBuffer;
}

bool Framebuffer::isEnabledColorAttachment(unsigned int colorAttachment) const
{
    return (mColorbuffers[colorAttachment] && mDrawBufferStates[colorAttachment] != GL_NONE);
}

bool Framebuffer::hasEnabledColorAttachment() const
{
    for (unsigned int colorAttachment = 0; colorAttachment < gl::IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        if (isEnabledColorAttachment(colorAttachment))
        {
            return true;
        }
    }

    return false;
}

bool Framebuffer::hasStencil() const
{
    return (mStencilbuffer && mStencilbuffer->getStencilSize() > 0);
}

bool Framebuffer::usingExtendedDrawBuffers() const
{
    for (unsigned int colorAttachment = 1; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        if (isEnabledColorAttachment(colorAttachment))
        {
            return true;
        }
    }

    return false;
}

GLenum Framebuffer::completeness() const
{
    int width = 0;
    int height = 0;
    unsigned int colorbufferSize = 0;
    int samples = -1;
    bool missingAttachment = true;
    GLuint clientVersion = mRenderer->getCurrentClientVersion();

    for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        const FramebufferAttachment *colorbuffer = mColorbuffers[colorAttachment];

        if (colorbuffer)
        {
            if (colorbuffer->getWidth() == 0 || colorbuffer->getHeight() == 0)
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }

            GLenum internalformat = colorbuffer->getInternalFormat();
            
            const TextureCaps &formatCaps = mRenderer->getRendererTextureCaps().get(internalformat);
            const InternalFormat &formatInfo = GetInternalFormatInfo(internalformat);
            if (colorbuffer->isTexture())
            {
                if (!formatCaps.renderable)
                {
                    return GL_FRAMEBUFFER_UNSUPPORTED;
                }

                if (formatInfo.depthBits > 0 || formatInfo.stencilBits > 0)
                {
                    return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
                }
            }
            else
            {
                if (!formatCaps.renderable || formatInfo.depthBits > 0 || formatInfo.stencilBits > 0)
                {
                    return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
                }
            }

            if (!missingAttachment)
            {
                
                if (colorbuffer->getWidth() != width || colorbuffer->getHeight() != height)
                {
                    return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
                }

                
                
                if (colorbuffer->getSamples() != samples)
                {
                    return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT;
                }

                
                
                if (clientVersion < 3)
                {
                    if (formatInfo.pixelBytes != colorbufferSize)
                    {
                        return GL_FRAMEBUFFER_UNSUPPORTED;
                    }
                }

                
                for (unsigned int previousColorAttachment = 0; previousColorAttachment < colorAttachment; previousColorAttachment++)
                {
                    const FramebufferAttachment *previousAttachment = mColorbuffers[previousColorAttachment];

                    if (previousAttachment &&
                        (colorbuffer->id() == previousAttachment->id() &&
                         colorbuffer->type() == previousAttachment->type()))
                    {
                        return GL_FRAMEBUFFER_UNSUPPORTED;
                    }
                }
            }
            else
            {
                width = colorbuffer->getWidth();
                height = colorbuffer->getHeight();
                samples = colorbuffer->getSamples();
                colorbufferSize = formatInfo.pixelBytes;
                missingAttachment = false;
            }
        }
    }

    if (mDepthbuffer)
    {
        if (mDepthbuffer->getWidth() == 0 || mDepthbuffer->getHeight() == 0)
        {
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        }

        GLenum internalformat = mDepthbuffer->getInternalFormat();
        
        const TextureCaps &formatCaps = mRenderer->getRendererTextureCaps().get(internalformat);
        const InternalFormat &formatInfo = GetInternalFormatInfo(internalformat);
        if (mDepthbuffer->isTexture())
        {
            
            
            if (!mRenderer->getRendererExtensions().depthTextures)
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }

            if (!formatCaps.renderable)
            {
                return GL_FRAMEBUFFER_UNSUPPORTED;
            }

            if (formatInfo.depthBits == 0)
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
        }
        else
        {
            if (!formatCaps.renderable || formatInfo.depthBits == 0)
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
        }

        if (missingAttachment)
        {
            width = mDepthbuffer->getWidth();
            height = mDepthbuffer->getHeight();
            samples = mDepthbuffer->getSamples();
            missingAttachment = false;
        }
        else if (width != mDepthbuffer->getWidth() || height != mDepthbuffer->getHeight())
        {
            return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
        }
        else if (samples != mDepthbuffer->getSamples())
        {
            return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_ANGLE;
        }
    }

    if (mStencilbuffer)
    {
        if (mStencilbuffer->getWidth() == 0 || mStencilbuffer->getHeight() == 0)
        {
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        }

        GLenum internalformat = mStencilbuffer->getInternalFormat();
        
        const TextureCaps &formatCaps = mRenderer->getRendererTextureCaps().get(internalformat);
        const InternalFormat &formatInfo = GetInternalFormatInfo(internalformat);
        if (mStencilbuffer->isTexture())
        {
            
            
            
            if (!mRenderer->getRendererExtensions().depthTextures)
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }

            if (!formatCaps.renderable)
            {
                return GL_FRAMEBUFFER_UNSUPPORTED;
            }

            if (formatInfo.stencilBits == 0)
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
        }
        else
        {
            if (!formatCaps.renderable || formatInfo.stencilBits == 0)
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
        }

        if (missingAttachment)
        {
            width = mStencilbuffer->getWidth();
            height = mStencilbuffer->getHeight();
            samples = mStencilbuffer->getSamples();
            missingAttachment = false;
        }
        else if (width != mStencilbuffer->getWidth() || height != mStencilbuffer->getHeight())
        {
            return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
        }
        else if (samples != mStencilbuffer->getSamples())
        {
            return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_ANGLE;
        }
    }

    
    
    if (mDepthbuffer && mStencilbuffer && !hasValidDepthStencil())
    {
        return GL_FRAMEBUFFER_UNSUPPORTED;
    }

    
    if (missingAttachment)
    {
        return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
    }

    return GL_FRAMEBUFFER_COMPLETE;
}

void Framebuffer::invalidate(const Caps &caps, GLsizei numAttachments, const GLenum *attachments)
{
    GLuint maxDimension = caps.maxRenderbufferSize;
    invalidateSub(caps, numAttachments, attachments, 0, 0, maxDimension, maxDimension);
}

void Framebuffer::invalidateSub(const Caps &caps, GLsizei numAttachments, const GLenum *attachments,
                                GLint x, GLint y, GLsizei width, GLsizei height)
{
    ASSERT(completeness() == GL_FRAMEBUFFER_COMPLETE);
    for (int i = 0; i < numAttachments; ++i)
    {
        rx::RenderTarget *renderTarget = NULL;

        if (attachments[i] >= GL_COLOR_ATTACHMENT0 && attachments[i] <= GL_COLOR_ATTACHMENT15)
        {
            gl::FramebufferAttachment *attachment = getColorbuffer(attachments[i] - GL_COLOR_ATTACHMENT0);
            if (attachment)
            {
                renderTarget = attachment->getRenderTarget();
            }
        }
        else if (attachments[i] == GL_COLOR)
        {
            gl::FramebufferAttachment *attachment = getColorbuffer(0);
            if (attachment)
            {
                renderTarget = attachment->getRenderTarget();
            }
        }
        else
        {
            gl::FramebufferAttachment *attachment = NULL;
            switch (attachments[i])
            {
              case GL_DEPTH_ATTACHMENT:
              case GL_DEPTH:
                attachment = mDepthbuffer;
                break;
              case GL_STENCIL_ATTACHMENT:
              case GL_STENCIL:
                attachment = mStencilbuffer;
                break;
              case GL_DEPTH_STENCIL_ATTACHMENT:
                attachment = getDepthOrStencilbuffer();
                break;
              default:
                UNREACHABLE();
            }

            if (attachment)
            {
                renderTarget = attachment->getRenderTarget();
            }
        }

        if (renderTarget)
        {
            renderTarget->invalidate(x, y, width, height);
        }
    }
}

DefaultFramebuffer::DefaultFramebuffer(rx::Renderer *renderer, Colorbuffer *colorbuffer, DepthStencilbuffer *depthStencil)
    : Framebuffer(renderer, 0)
{
    Renderbuffer *colorRenderbuffer = new Renderbuffer(0, colorbuffer);
    mColorbuffers[0] = new RenderbufferAttachment(colorRenderbuffer);

    Renderbuffer *depthStencilBuffer = new Renderbuffer(0, depthStencil);

    
    
    mDepthbuffer = (depthStencilBuffer->getDepthSize() != 0 ? new RenderbufferAttachment(depthStencilBuffer) : NULL);
    mStencilbuffer = (depthStencilBuffer->getStencilSize() != 0 ? new RenderbufferAttachment(depthStencilBuffer) : NULL);

    mDrawBufferStates[0] = GL_BACK;
    mReadBufferState = GL_BACK;
}

int Framebuffer::getSamples() const
{
    if (completeness() == GL_FRAMEBUFFER_COMPLETE)
    {
        
        
        for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
        {
            if (mColorbuffers[colorAttachment])
            {
                return mColorbuffers[colorAttachment]->getSamples();
            }
        }
    }

    return 0;
}

bool Framebuffer::hasValidDepthStencil() const
{
    
    
    return (mDepthbuffer && mStencilbuffer &&
            mDepthbuffer->type() == mStencilbuffer->type() &&
            mDepthbuffer->id() == mStencilbuffer->id());
}

GLenum DefaultFramebuffer::completeness() const
{
    
    
    return GL_FRAMEBUFFER_COMPLETE;
}

FramebufferAttachment *DefaultFramebuffer::getAttachment(GLenum attachment) const
{
    switch (attachment)
    {
      case GL_BACK:
        return getColorbuffer(0);
      case GL_DEPTH:
        return getDepthbuffer();
      case GL_STENCIL:
        return getStencilbuffer();
      case GL_DEPTH_STENCIL:
        return getDepthStencilBuffer();
      default:
        UNREACHABLE();
        return NULL;
    }
}

}
