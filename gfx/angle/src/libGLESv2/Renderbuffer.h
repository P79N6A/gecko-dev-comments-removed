










#ifndef LIBGLESV2_RENDERBUFFER_H_
#define LIBGLESV2_RENDERBUFFER_H_

#define GL_APICALL
#include <GLES2/gl2.h>

#include "common/angleutils.h"
#include "common/RefCountObject.h"

namespace rx
{
class Renderer;
class SwapChain;
class RenderTarget;
}

namespace gl
{
class Texture2D;
class TextureCubeMap;
class Renderbuffer;
class Colorbuffer;
class DepthStencilbuffer;

class RenderbufferInterface
{
  public:
    RenderbufferInterface();

    virtual ~RenderbufferInterface() {};

    virtual void addProxyRef(const Renderbuffer *proxy);
    virtual void releaseProxy(const Renderbuffer *proxy);

    virtual rx::RenderTarget *getRenderTarget() = 0;
    virtual rx::RenderTarget *getDepthStencil() = 0;

    virtual GLsizei getWidth() const = 0;
    virtual GLsizei getHeight() const = 0;
    virtual GLenum getInternalFormat() const = 0;
    virtual GLenum getActualFormat() const = 0;
    virtual GLsizei getSamples() const = 0;

    GLuint getRedSize() const;
    GLuint getGreenSize() const;
    GLuint getBlueSize() const;
    GLuint getAlphaSize() const;
    GLuint getDepthSize() const;
    GLuint getStencilSize() const;

    virtual unsigned int getSerial() const = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderbufferInterface);
};

class RenderbufferTexture2D : public RenderbufferInterface
{
  public:
    RenderbufferTexture2D(Texture2D *texture, GLenum target);

    virtual ~RenderbufferTexture2D();

    void addProxyRef(const Renderbuffer *proxy);
    void releaseProxy(const Renderbuffer *proxy);

    rx::RenderTarget *getRenderTarget();
    rx::RenderTarget *getDepthStencil();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual unsigned int getSerial() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderbufferTexture2D);

    BindingPointer <Texture2D> mTexture2D;
    GLenum mTarget;
};

class RenderbufferTextureCubeMap : public RenderbufferInterface
{
  public:
    RenderbufferTextureCubeMap(TextureCubeMap *texture, GLenum target);

    virtual ~RenderbufferTextureCubeMap();

    void addProxyRef(const Renderbuffer *proxy);
    void releaseProxy(const Renderbuffer *proxy);

    rx::RenderTarget *getRenderTarget();
    rx::RenderTarget *getDepthStencil();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual unsigned int getSerial() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderbufferTextureCubeMap);

    BindingPointer <TextureCubeMap> mTextureCubeMap;
    GLenum mTarget;
};




class RenderbufferStorage : public RenderbufferInterface
{
  public:
    RenderbufferStorage();

    virtual ~RenderbufferStorage() = 0;

    virtual rx::RenderTarget *getRenderTarget();
    virtual rx::RenderTarget *getDepthStencil();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual unsigned int getSerial() const;

    static unsigned int issueSerial();
    static unsigned int issueCubeSerials();

  protected:
    GLsizei mWidth;
    GLsizei mHeight;
    GLenum mInternalFormat;
    GLenum mActualFormat;
    GLsizei mSamples;

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderbufferStorage);

    const unsigned int mSerial;

    static unsigned int mCurrentSerial;
};




class Renderbuffer : public RefCountObject
{
  public:
    Renderbuffer(rx::Renderer *renderer, GLuint id, RenderbufferInterface *storage);

    virtual ~Renderbuffer();

    
    
    
    
    void addRef() const;
    void release() const;

    rx::RenderTarget *getRenderTarget();
    rx::RenderTarget *getDepthStencil();

    GLsizei getWidth() const;
    GLsizei getHeight() const;
    GLenum getInternalFormat() const;
    GLenum getActualFormat() const;
    GLuint getRedSize() const;
    GLuint getGreenSize() const;
    GLuint getBlueSize() const;
    GLuint getAlphaSize() const;
    GLuint getDepthSize() const;
    GLuint getStencilSize() const;
    GLsizei getSamples() const;

    unsigned int getSerial() const;

    void setStorage(RenderbufferStorage *newStorage);

  private:
    DISALLOW_COPY_AND_ASSIGN(Renderbuffer);

    RenderbufferInterface *mInstance;
};

class Colorbuffer : public RenderbufferStorage
{
  public:
    Colorbuffer(rx::Renderer *renderer, rx::SwapChain *swapChain);
    Colorbuffer(rx::Renderer *renderer, GLsizei width, GLsizei height, GLenum format, GLsizei samples);

    virtual ~Colorbuffer();

    virtual rx::RenderTarget *getRenderTarget();

  private:
    DISALLOW_COPY_AND_ASSIGN(Colorbuffer);

    rx::RenderTarget *mRenderTarget;
};

class DepthStencilbuffer : public RenderbufferStorage
{
  public:
    DepthStencilbuffer(rx::Renderer *renderer, rx::SwapChain *swapChain);
    DepthStencilbuffer(rx::Renderer *renderer, GLsizei width, GLsizei height, GLsizei samples);

    ~DepthStencilbuffer();

    virtual rx::RenderTarget *getDepthStencil();

  protected:
    rx::RenderTarget  *mDepthStencil;

  private:
    DISALLOW_COPY_AND_ASSIGN(DepthStencilbuffer);
};

class Depthbuffer : public DepthStencilbuffer
{
  public:
    Depthbuffer(rx::Renderer *renderer, GLsizei width, GLsizei height, GLsizei samples);

    virtual ~Depthbuffer();

  private:
    DISALLOW_COPY_AND_ASSIGN(Depthbuffer);
};

class Stencilbuffer : public DepthStencilbuffer
{
  public:
    Stencilbuffer(rx::Renderer *renderer, GLsizei width, GLsizei height, GLsizei samples);

    virtual ~Stencilbuffer();

  private:
    DISALLOW_COPY_AND_ASSIGN(Stencilbuffer);
};
}

#endif   
