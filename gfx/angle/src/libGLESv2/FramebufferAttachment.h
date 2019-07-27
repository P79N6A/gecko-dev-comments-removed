








#ifndef LIBGLESV2_FRAMEBUFFERATTACHMENT_H_
#define LIBGLESV2_FRAMEBUFFERATTACHMENT_H_

#include "common/angleutils.h"
#include "common/RefCountObject.h"

#include "angle_gl.h"

namespace rx
{
class Renderer;
class RenderTarget;
class TextureStorage;
}

namespace gl
{
class Texture2D;
class TextureCubeMap;
class Texture3D;
class Texture2DArray;
class Renderbuffer;







class FramebufferAttachment
{
  public:
    FramebufferAttachment();
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

    
    virtual rx::RenderTarget *getRenderTarget() = 0;
    virtual rx::TextureStorage *getTextureStorage() = 0;

    virtual GLsizei getWidth() const = 0;
    virtual GLsizei getHeight() const = 0;
    virtual GLenum getInternalFormat() const = 0;
    virtual GLenum getActualFormat() const = 0;
    virtual GLsizei getSamples() const = 0;

    virtual unsigned int getSerial() const = 0;

    virtual GLuint id() const = 0;
    virtual GLenum type() const = 0;
    virtual GLint mipLevel() const = 0;
    virtual GLint layer() const = 0;
    virtual unsigned int getTextureSerial() const = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(FramebufferAttachment);
};

class Texture2DAttachment : public FramebufferAttachment
{
  public:
    Texture2DAttachment(Texture2D *texture, GLint level);

    virtual ~Texture2DAttachment();

    rx::RenderTarget *getRenderTarget();
    rx::TextureStorage *getTextureStorage();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual unsigned int getSerial() const;

    virtual GLuint id() const;
    virtual GLenum type() const;
    virtual GLint mipLevel() const;
    virtual GLint layer() const;
    virtual unsigned int getTextureSerial() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture2DAttachment);

    BindingPointer<Texture2D> mTexture2D;
    const GLint mLevel;
};

class TextureCubeMapAttachment : public FramebufferAttachment
{
  public:
    TextureCubeMapAttachment(TextureCubeMap *texture, GLenum faceTarget, GLint level);

    virtual ~TextureCubeMapAttachment();

    rx::RenderTarget *getRenderTarget();
    rx::TextureStorage *getTextureStorage();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual unsigned int getSerial() const;

    virtual GLuint id() const;
    virtual GLenum type() const;
    virtual GLint mipLevel() const;
    virtual GLint layer() const;
    virtual unsigned int getTextureSerial() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureCubeMapAttachment);

    BindingPointer<TextureCubeMap> mTextureCubeMap;
    const GLint mLevel;
    const GLenum mFaceTarget;
};

class Texture3DAttachment : public FramebufferAttachment
{
  public:
    Texture3DAttachment(Texture3D *texture, GLint level, GLint layer);

    virtual ~Texture3DAttachment();

    rx::RenderTarget *getRenderTarget();
    rx::TextureStorage *getTextureStorage();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual unsigned int getSerial() const;

    virtual GLuint id() const;
    virtual GLenum type() const;
    virtual GLint mipLevel() const;
    virtual GLint layer() const;
    virtual unsigned int getTextureSerial() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture3DAttachment);

    BindingPointer<Texture3D> mTexture3D;
    const GLint mLevel;
    const GLint mLayer;
};

class Texture2DArrayAttachment : public FramebufferAttachment
{
  public:
    Texture2DArrayAttachment(Texture2DArray *texture, GLint level, GLint layer);

    virtual ~Texture2DArrayAttachment();

    rx::RenderTarget *getRenderTarget();
    rx::TextureStorage *getTextureStorage();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual unsigned int getSerial() const;

    virtual GLuint id() const;
    virtual GLenum type() const;
    virtual GLint mipLevel() const;
    virtual GLint layer() const;
    virtual unsigned int getTextureSerial() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture2DArrayAttachment);

    BindingPointer<Texture2DArray> mTexture2DArray;
    const GLint mLevel;
    const GLint mLayer;
};

class RenderbufferAttachment : public FramebufferAttachment
{
  public:
    RenderbufferAttachment(Renderbuffer *renderbuffer);

    virtual ~RenderbufferAttachment();

    rx::RenderTarget *getRenderTarget();
    rx::TextureStorage *getTextureStorage();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual unsigned int getSerial() const;

    virtual GLuint id() const;
    virtual GLenum type() const;
    virtual GLint mipLevel() const;
    virtual GLint layer() const;
    virtual unsigned int getTextureSerial() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderbufferAttachment);

    BindingPointer<Renderbuffer> mRenderbuffer;
};

}

#endif 
