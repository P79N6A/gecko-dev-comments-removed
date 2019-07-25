








#ifndef LIBGLESV2_FRAMEBUFFER_H_
#define LIBGLESV2_FRAMEBUFFER_H_

#define GL_APICALL
#include <GLES2/gl2.h>
#include <d3d9.h>

#include "common/angleutils.h"

namespace gl
{
class Colorbuffer;
class Depthbuffer;
class Stencilbuffer;

class Framebuffer
{
  public:
    Framebuffer();

    ~Framebuffer();

    void setColorbuffer(GLenum type, GLuint colorbuffer);
    void setDepthbuffer(GLenum type, GLuint depthbuffer);
    void setStencilbuffer(GLenum type, GLuint stencilbuffer);

    void detachTexture(GLuint texture);
    void detachRenderbuffer(GLuint renderbuffer);

    IDirect3DSurface9 *getRenderTarget();
    IDirect3DSurface9 *getDepthStencil();

    unsigned int getRenderTargetSerial();
    unsigned int getDepthbufferSerial();

    Colorbuffer *getColorbuffer();
    Depthbuffer *getDepthbuffer();
    Stencilbuffer *getStencilbuffer();

    GLenum getColorbufferType();
    GLenum getDepthbufferType();
    GLenum getStencilbufferType();

    GLuint getColorbufferHandle();
    GLuint getDepthbufferHandle();
    GLuint getStencilbufferHandle();

    GLenum completeness();

  private:
    DISALLOW_COPY_AND_ASSIGN(Framebuffer);

    GLuint mColorbufferHandle;
    GLenum mColorbufferType;

    GLuint mDepthbufferHandle;
    GLenum mDepthbufferType;

    GLuint mStencilbufferHandle;
    GLenum mStencilbufferType;
};
}

#endif   
