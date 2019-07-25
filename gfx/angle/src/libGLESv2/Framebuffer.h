








#ifndef LIBGLESV2_FRAMEBUFFER_H_
#define LIBGLESV2_FRAMEBUFFER_H_

#define GL_APICALL
#include <GLES2/gl2.h>
#include <d3d9.h>

#include "common/angleutils.h"
#include "libGLESv2/RefCountObject.h"

namespace gl
{
class Renderbuffer;
class Colorbuffer;
class Depthbuffer;
class Stencilbuffer;
class DepthStencilbuffer;

class Framebuffer
{
  public:
    Framebuffer();

    virtual ~Framebuffer();

    void setColorbuffer(GLenum type, GLuint colorbuffer);
    void setDepthbuffer(GLenum type, GLuint depthbuffer);
    void setStencilbuffer(GLenum type, GLuint stencilbuffer);

    void detachTexture(GLuint texture);
    void detachRenderbuffer(GLuint renderbuffer);

    IDirect3DSurface9 *getRenderTarget();
    IDirect3DSurface9 *getDepthStencil();

    unsigned int getRenderTargetSerial();
    unsigned int getDepthbufferSerial();
    unsigned int getStencilbufferSerial();

    Colorbuffer *getColorbuffer();
    DepthStencilbuffer *getDepthbuffer();
    DepthStencilbuffer *getStencilbuffer();

    GLenum getColorbufferType();
    GLenum getDepthbufferType();
    GLenum getStencilbufferType();

    GLuint getColorbufferHandle();
    GLuint getDepthbufferHandle();
    GLuint getStencilbufferHandle();

    bool hasStencil();
    int getSamples();

    virtual GLenum completeness();

  protected:
    GLenum mColorbufferType;
    BindingPointer<Renderbuffer> mColorbufferPointer;

    GLenum mDepthbufferType;
    BindingPointer<Renderbuffer> mDepthbufferPointer;

    GLenum mStencilbufferType;
    BindingPointer<Renderbuffer> mStencilbufferPointer;

  private:
    DISALLOW_COPY_AND_ASSIGN(Framebuffer);

    Renderbuffer *lookupRenderbuffer(GLenum type, GLuint handle) const;
};

class DefaultFramebuffer : public Framebuffer
{
  public:
    DefaultFramebuffer(Colorbuffer *color, DepthStencilbuffer *depthStencil);

    virtual GLenum completeness();

  private:
    DISALLOW_COPY_AND_ASSIGN(DefaultFramebuffer);
};

}

#endif   
