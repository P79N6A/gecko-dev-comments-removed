










#ifndef LIBGLESV2_RENDERBUFFER_H_
#define LIBGLESV2_RENDERBUFFER_H_

#define GL_APICALL
#include <GLES2/gl2.h>
#include <d3d9.h>

#include "common/angleutils.h"
#include "libGLESv2/RefCountObject.h"

namespace gl
{
class Texture;




class RenderbufferStorage
{
  public:
    RenderbufferStorage();

    virtual ~RenderbufferStorage() = 0;

    virtual bool isColorbuffer() const;
    virtual bool isDepthbuffer() const;
    virtual bool isStencilbuffer() const;

    virtual IDirect3DSurface9 *getRenderTarget();
    virtual IDirect3DSurface9 *getDepthStencil();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    GLuint getRedSize() const;
    GLuint getGreenSize() const;
    GLuint getBlueSize() const;
    GLuint getAlphaSize() const;
    GLuint getDepthSize() const;
    GLuint getStencilSize() const;
    virtual GLsizei getSamples() const;

    virtual D3DFORMAT getD3DFormat() const;

    unsigned int getSerial() const;

  protected:
    GLsizei mWidth;
    GLsizei mHeight;
    GLenum mInternalFormat;
    D3DFORMAT mD3DFormat;
    GLsizei mSamples;

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderbufferStorage);

    static unsigned int issueSerial();

    const unsigned int mSerial;

    static unsigned int mCurrentSerial;
};




class Renderbuffer : public RefCountObject
{
  public:
    Renderbuffer(GLuint id, RenderbufferStorage *storage);

    ~Renderbuffer();

    bool isColorbuffer() const;
    bool isDepthbuffer() const;
    bool isStencilbuffer() const;

    IDirect3DSurface9 *getRenderTarget();
    IDirect3DSurface9 *getDepthStencil();

    GLsizei getWidth() const;
    GLsizei getHeight() const;
    GLenum getInternalFormat() const;
    D3DFORMAT getD3DFormat() const;
    GLuint getRedSize() const;
    GLuint getGreenSize() const;
    GLuint getBlueSize() const;
    GLuint getAlphaSize() const;
    GLuint getDepthSize() const;
    GLuint getStencilSize() const;
    GLsizei getSamples() const;

    unsigned int getSerial() const;

    void setStorage(RenderbufferStorage *newStorage);
    RenderbufferStorage *getStorage() { return mStorage; }

  private:
    DISALLOW_COPY_AND_ASSIGN(Renderbuffer);

    RenderbufferStorage *mStorage;
};

class Colorbuffer : public RenderbufferStorage
{
  public:
    explicit Colorbuffer(IDirect3DSurface9 *renderTarget);
    Colorbuffer(Texture *texture, GLenum target);
    Colorbuffer(GLsizei width, GLsizei height, GLenum format, GLsizei samples);

    virtual ~Colorbuffer();

    virtual bool isColorbuffer() const;

    virtual IDirect3DSurface9 *getRenderTarget();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getType() const;

    virtual D3DFORMAT getD3DFormat() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Colorbuffer);

    IDirect3DSurface9 *mRenderTarget;
    Texture *mTexture;
    GLenum mTarget;
};

class DepthStencilbuffer : public RenderbufferStorage
{
  public:
    explicit DepthStencilbuffer(IDirect3DSurface9 *depthStencil);
    DepthStencilbuffer(GLsizei width, GLsizei height, GLsizei samples);

    ~DepthStencilbuffer();

    virtual bool isDepthbuffer() const;
    virtual bool isStencilbuffer() const;

    virtual IDirect3DSurface9 *getDepthStencil();

  private:
    DISALLOW_COPY_AND_ASSIGN(DepthStencilbuffer);
    IDirect3DSurface9 *mDepthStencil;
};

class Depthbuffer : public DepthStencilbuffer
{
  public:
    explicit Depthbuffer(IDirect3DSurface9 *depthStencil);
    Depthbuffer(GLsizei width, GLsizei height, GLsizei samples);

    virtual ~Depthbuffer();

    virtual bool isDepthbuffer() const;
    virtual bool isStencilbuffer() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Depthbuffer);
};

class Stencilbuffer : public DepthStencilbuffer
{
  public:
    explicit Stencilbuffer(IDirect3DSurface9 *depthStencil);
    Stencilbuffer(GLsizei width, GLsizei height, GLsizei samples);

    virtual ~Stencilbuffer();

    virtual bool isDepthbuffer() const;
    virtual bool isStencilbuffer() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Stencilbuffer);
};
}

#endif   
