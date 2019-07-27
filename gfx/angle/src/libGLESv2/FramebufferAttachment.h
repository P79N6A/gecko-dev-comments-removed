








#ifndef LIBGLESV2_FRAMEBUFFERATTACHMENT_H_
#define LIBGLESV2_FRAMEBUFFERATTACHMENT_H_

#include "common/angleutils.h"
#include "common/RefCountObject.h"
#include "Texture.h"

#include "angle_gl.h"

namespace rx
{
class Renderer;
class RenderTarget;
class TextureStorage;
}

namespace gl
{
class Renderbuffer;







class FramebufferAttachment
{
  public:
    explicit FramebufferAttachment(GLenum binding);
    virtual ~FramebufferAttachment();

    
    GLuint getRedSize() const;
    GLuint getGreenSize() const;
    GLuint getBlueSize() const;
    GLuint getAlphaSize() const;
    GLuint getDepthSize() const;
    GLuint getStencilSize() const;
    GLenum getComponentType() const;
    GLenum getColorEncoding() const;
    bool isTexture() const;

    bool isTextureWithId(GLuint textureId) const { return isTexture() && id() == textureId; }
    bool isRenderbufferWithId(GLuint renderbufferId) const { return !isTexture() && id() == renderbufferId; }

    GLenum getBinding() const { return mBinding; }

    
    virtual GLsizei getWidth() const = 0;
    virtual GLsizei getHeight() const = 0;
    virtual GLenum getInternalFormat() const = 0;
    virtual GLenum getActualFormat() const = 0;
    virtual GLsizei getSamples() const = 0;

    virtual GLuint id() const = 0;
    virtual GLenum type() const = 0;
    virtual GLint mipLevel() const = 0;
    virtual GLint layer() const = 0;

    virtual Texture *getTexture() = 0;
    virtual const ImageIndex *getTextureImageIndex() const = 0;
    virtual Renderbuffer *getRenderbuffer() = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(FramebufferAttachment);

    GLenum mBinding;
};

class TextureAttachment : public FramebufferAttachment
{
  public:
    TextureAttachment(GLenum binding, Texture *texture, const ImageIndex &index);
    virtual ~TextureAttachment();

    virtual GLsizei getSamples() const;
    virtual GLuint id() const;

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;

    virtual GLenum type() const;
    virtual GLint mipLevel() const;
    virtual GLint layer() const;

    virtual Texture *getTexture();
    virtual const ImageIndex *getTextureImageIndex() const;
    virtual Renderbuffer *getRenderbuffer();

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureAttachment);

    BindingPointer<Texture> mTexture;
    ImageIndex mIndex;
};

class RenderbufferAttachment : public FramebufferAttachment
{
  public:
    RenderbufferAttachment(GLenum binding, Renderbuffer *renderbuffer);

    virtual ~RenderbufferAttachment();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual GLuint id() const;
    virtual GLenum type() const;
    virtual GLint mipLevel() const;
    virtual GLint layer() const;

    virtual Texture *getTexture();
    virtual const ImageIndex *getTextureImageIndex() const;
    virtual Renderbuffer *getRenderbuffer();

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderbufferAttachment);

    BindingPointer<Renderbuffer> mRenderbuffer;
};

}

#endif 
