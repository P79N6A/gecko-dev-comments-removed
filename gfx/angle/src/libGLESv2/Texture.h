









#ifndef LIBGLESV2_TEXTURE_H_
#define LIBGLESV2_TEXTURE_H_

#include <vector>

#define GL_APICALL
#include <GLES2/gl2.h>
#include <d3d9.h>

#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/utilities.h"
#include "common/debug.h"

namespace gl
{
class Context;
class Blit;

enum
{
    MAX_TEXTURE_SIZE = 2048,
    MAX_CUBE_MAP_TEXTURE_SIZE = 2048,

    MAX_TEXTURE_LEVELS = 12   
};

class Texture
{
  public:
    explicit Texture(Context *context);

    virtual ~Texture();

    virtual GLenum getTarget() const = 0;

    bool setMinFilter(GLenum filter);
    bool setMagFilter(GLenum filter);
    bool setWrapS(GLenum wrap);
    bool setWrapT(GLenum wrap);

    GLenum getMinFilter() const;
    GLenum getMagFilter() const;
    GLenum getWrapS() const;
    GLenum getWrapT() const;

    GLuint getWidth() const;
    GLuint getHeight() const;

    virtual bool isComplete() const = 0;

    IDirect3DBaseTexture9 *getTexture();
    virtual Colorbuffer *getColorbuffer(GLenum target) = 0;

    virtual void generateMipmaps() = 0;

    bool isDirty() const;

  protected:
    class TextureColorbufferProxy;
    friend class TextureColorbufferProxy;
    class TextureColorbufferProxy : public Colorbuffer
    {
      public:
        TextureColorbufferProxy(Texture *texture, GLenum target);
            

        virtual IDirect3DSurface9 *getRenderTarget();

        virtual int getWidth();
        virtual int getHeight();

      private:
        Texture *mTexture;
        GLenum mTarget;
    };

    
    struct Image
    {
        Image();
        ~Image();

        GLsizei width;
        GLsizei height;
        GLenum format;

        bool dirty;

        IDirect3DSurface9 *surface;
    };

    static D3DFORMAT selectFormat(GLenum format);
    int imagePitch(const Image& img) const;

    GLenum mMinFilter;
    GLenum mMagFilter;
    GLenum mWrapS;
    GLenum mWrapT;

    void setImage(GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels, Image *img);
    void subImage(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels, Image *img);

    void needRenderTarget();

    GLint creationLevels(GLsizei width, GLsizei height, GLint maxlevel) const;
    GLint creationLevels(GLsizei size, GLint maxlevel) const;

    
    virtual IDirect3DBaseTexture9 *createTexture() = 0;
    virtual void updateTexture() = 0;
    virtual IDirect3DBaseTexture9 *convertToRenderTarget() = 0;
    virtual IDirect3DSurface9 *getRenderTarget(GLenum target) = 0;

    virtual bool dirtyImageData() const = 0;

    void dropTexture();
    void pushTexture(IDirect3DBaseTexture9 *newTexture, bool renderable);

    Blit *getBlitter();

    unsigned int mWidth;
    unsigned int mHeight;

    int levelCount() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture);

    Context *mContext;

    IDirect3DBaseTexture9 *mBaseTexture; 
    bool mDirtyMetaData;
    bool mIsRenderable;

    bool mDirty;

    void loadImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type,
                       GLint unpackAlignment, const void *input, std::size_t outputPitch, void *output) const;
};

class Texture2D : public Texture
{
  public:
    explicit Texture2D(Context *context);

    ~Texture2D();

    GLenum getTarget() const;

    void setImage(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void copyImage(GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, Renderbuffer *source);
    void copySubImage(GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, Renderbuffer *source);

    bool isComplete() const;

    virtual void generateMipmaps();

    virtual Colorbuffer *getColorbuffer(GLenum target);

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture2D);

    virtual IDirect3DBaseTexture9 *createTexture();
    virtual void updateTexture();
    virtual IDirect3DBaseTexture9 *convertToRenderTarget();

    virtual bool dirtyImageData() const;

    void commitRect(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);

    Image mImageArray[MAX_TEXTURE_LEVELS];

    IDirect3DTexture9 *mTexture;

    TextureColorbufferProxy *mColorbufferProxy;

    bool redefineTexture(GLint level, GLenum internalFormat, GLsizei width, GLsizei height);

    virtual IDirect3DSurface9 *getRenderTarget(GLenum target);
};

class TextureCubeMap : public Texture
{
  public:
    explicit TextureCubeMap(Context *context);

    ~TextureCubeMap();

    GLenum getTarget() const;

    void setImagePosX(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImageNegX(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImagePosY(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImageNegY(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImagePosZ(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImageNegZ(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);

    void subImage(GLenum face, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void copyImage(GLenum face, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, Renderbuffer *source);
    void copySubImage(GLenum face, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, Renderbuffer *source);

    bool isComplete() const;

    virtual void generateMipmaps();

    virtual Colorbuffer *getColorbuffer(GLenum target);

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureCubeMap);

    virtual IDirect3DBaseTexture9 *createTexture();
    virtual void updateTexture();
    virtual IDirect3DBaseTexture9 *convertToRenderTarget();

    virtual bool dirtyImageData() const;

    
    
    IDirect3DSurface9 *getCubeMapSurface(unsigned int faceIdentifier, unsigned int level);

    static unsigned int faceIndex(GLenum face);

    bool isCubeComplete() const;

    void setImage(int face, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void commitRect(GLenum faceTarget, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);
    bool redefineTexture(GLint level, GLenum internalFormat, GLsizei width);

    Image mImageArray[6][MAX_TEXTURE_LEVELS];

    IDirect3DCubeTexture9 *mTexture;

    TextureColorbufferProxy *mFaceProxies[6];

    virtual IDirect3DSurface9 *getRenderTarget(GLenum target);
};
}

#endif   
