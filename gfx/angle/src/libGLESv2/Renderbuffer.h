









#ifndef LIBGLESV2_RENDERBUFFER_H_
#define LIBGLESV2_RENDERBUFFER_H_

#define GL_APICALL
#include <GLES2/gl2.h>
#include <d3d9.h>

#include "common/angleutils.h"

namespace gl
{
class Renderbuffer
{
  public:
    Renderbuffer();

    virtual ~Renderbuffer();

    virtual bool isColorbuffer();
    virtual bool isDepthbuffer();
    virtual bool isStencilbuffer();

    virtual IDirect3DSurface9 *getRenderTarget();
    virtual IDirect3DSurface9 *getDepthStencil();

    virtual int getWidth();
    virtual int getHeight();
    GLenum getFormat();
    unsigned int getSerial() const;

    static unsigned int issueSerial();

  protected:
    void setSize(int width, int height);
    GLenum mFormat;
    unsigned int mSerial;

  private:
    DISALLOW_COPY_AND_ASSIGN(Renderbuffer);

    static unsigned int mCurrentSerial;

    int mWidth;
    int mHeight;
};

class Colorbuffer : public Renderbuffer
{
  public:
    explicit Colorbuffer(IDirect3DSurface9 *renderTarget);
    Colorbuffer(int width, int height, GLenum format);

    ~Colorbuffer();

    bool isColorbuffer();

    GLuint getRedSize();
    GLuint getGreenSize();
    GLuint getBlueSize();
    GLuint getAlphaSize();

    IDirect3DSurface9 *getRenderTarget();

  protected:
    IDirect3DSurface9 *mRenderTarget;

  private:
    DISALLOW_COPY_AND_ASSIGN(Colorbuffer);
};

class Depthbuffer : public Renderbuffer
{
  public:
    explicit Depthbuffer(IDirect3DSurface9 *depthStencil);
    Depthbuffer(int width, int height);

    ~Depthbuffer();

    bool isDepthbuffer();

    GLuint getDepthSize();

    IDirect3DSurface9 *getDepthStencil();

  private:
    DISALLOW_COPY_AND_ASSIGN(Depthbuffer);
    IDirect3DSurface9 *mDepthStencil;
};

class Stencilbuffer : public Renderbuffer
{
  public:
    explicit Stencilbuffer(IDirect3DSurface9 *depthStencil);
    Stencilbuffer(int width, int height);

    ~Stencilbuffer();

    bool isStencilbuffer();

    GLuint getStencilSize();

    IDirect3DSurface9 *getDepthStencil();

  private:
    DISALLOW_COPY_AND_ASSIGN(Stencilbuffer);
    IDirect3DSurface9 *mDepthStencil;
};
}

#endif   
