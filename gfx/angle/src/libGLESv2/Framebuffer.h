








#ifndef LIBGLESV2_FRAMEBUFFER_H_
#define LIBGLESV2_FRAMEBUFFER_H_

#include "common/angleutils.h"
#include "common/RefCountObject.h"
#include "constants.h"

namespace rx
{
class Renderer;
}

namespace gl
{
class FramebufferAttachment;
class Colorbuffer;
class Depthbuffer;
class Stencilbuffer;
class DepthStencilbuffer;
struct Caps;

class Framebuffer
{
  public:
    Framebuffer(rx::Renderer *renderer, GLuint id);

    virtual ~Framebuffer();

    GLuint id() const { return mId; }

    void setColorbuffer(unsigned int colorAttachment, GLenum type, GLuint colorbuffer, GLint level, GLint layer);
    void setDepthbuffer(GLenum type, GLuint depthbuffer, GLint level, GLint layer);
    void setStencilbuffer(GLenum type, GLuint stencilbuffer, GLint level, GLint layer);
    void setDepthStencilBuffer(GLenum type, GLuint depthStencilBuffer, GLint level, GLint layer);

    void detachTexture(GLuint texture);
    void detachRenderbuffer(GLuint renderbuffer);

    FramebufferAttachment *getColorbuffer(unsigned int colorAttachment) const;
    FramebufferAttachment *getDepthbuffer() const;
    FramebufferAttachment *getStencilbuffer() const;
    FramebufferAttachment *getDepthStencilBuffer() const;
    FramebufferAttachment *getDepthOrStencilbuffer() const;
    FramebufferAttachment *getReadColorbuffer() const;
    GLenum getReadColorbufferType() const;
    FramebufferAttachment *getFirstColorbuffer() const;

    virtual FramebufferAttachment *getAttachment(GLenum attachment) const;

    GLenum getDrawBufferState(unsigned int colorAttachment) const;
    void setDrawBufferState(unsigned int colorAttachment, GLenum drawBuffer);

    bool isEnabledColorAttachment(unsigned int colorAttachment) const;
    bool hasEnabledColorAttachment() const;
    bool hasStencil() const;
    int getSamples() const;
    bool usingExtendedDrawBuffers() const;

    virtual GLenum completeness() const;
    bool hasValidDepthStencil() const;

    void invalidate(const Caps &caps, GLsizei numAttachments, const GLenum *attachments);
    void invalidateSub(const Caps &caps, GLsizei numAttachments, const GLenum *attachments,
                       GLint x, GLint y, GLsizei width, GLsizei height);

  protected:
    rx::Renderer *mRenderer;

    GLuint mId;

    FramebufferAttachment *mColorbuffers[IMPLEMENTATION_MAX_DRAW_BUFFERS];
    GLenum mDrawBufferStates[IMPLEMENTATION_MAX_DRAW_BUFFERS];
    GLenum mReadBufferState;

    FramebufferAttachment *mDepthbuffer;
    FramebufferAttachment *mStencilbuffer;

private:
    DISALLOW_COPY_AND_ASSIGN(Framebuffer);

    FramebufferAttachment *createAttachment(GLenum type, GLuint handle, GLint level, GLint layer) const;
};

class DefaultFramebuffer : public Framebuffer
{
  public:
    DefaultFramebuffer(rx::Renderer *Renderer, Colorbuffer *colorbuffer, DepthStencilbuffer *depthStencil);

    virtual GLenum completeness() const;
    virtual FramebufferAttachment *getAttachment(GLenum attachment) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(DefaultFramebuffer);
};

}

#endif   
