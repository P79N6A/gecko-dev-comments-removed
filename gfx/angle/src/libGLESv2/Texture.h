









#ifndef LIBGLESV2_TEXTURE_H_
#define LIBGLESV2_TEXTURE_H_

#include <vector>

#define GL_APICALL
#include <GLES2/gl2.h>
#include <d3d9.h>

#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/RefCountObject.h"
#include "libGLESv2/utilities.h"
#include "common/debug.h"

namespace gl
{
class Blit;

enum
{
    
    
    
    IMPLEMENTATION_MAX_TEXTURE_SIZE = 16384,
    IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE = 16384,

    IMPLEMENTATION_MAX_TEXTURE_LEVELS = 15   
};

class Texture : public RefCountObject
{
  public:
    explicit Texture(GLuint id);

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

    virtual GLenum getFormat() const = 0;
    virtual bool isComplete() const = 0;
    virtual bool isCompressed() const = 0;
    bool isFloatingPoint() const;
    bool isRenderableFormat() const;

    D3DFORMAT getD3DFormat() const;
    IDirect3DBaseTexture9 *getTexture();
    virtual Renderbuffer *getColorbuffer(GLenum target) = 0;

    virtual void generateMipmaps() = 0;
    virtual void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, RenderbufferStorage *source) = 0;

    bool isDirty() const;

    static const GLuint INCOMPLETE_TEXTURE_ID = static_cast<GLuint>(-1); 

  protected:
    class TextureColorbufferProxy;
    friend class TextureColorbufferProxy;
    class TextureColorbufferProxy : public Colorbuffer
    {
      public:
        TextureColorbufferProxy(Texture *texture, GLenum target);
            

        virtual void addRef() const;
        virtual void release() const;

        virtual IDirect3DSurface9 *getRenderTarget();

        virtual int getWidth() const;
        virtual int getHeight() const;
        virtual GLenum getFormat() const;
        virtual bool isFloatingPoint() const;

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

    static D3DFORMAT selectFormat(GLenum format, GLenum type);

    void setImage(GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels, Image *img);
    bool subImage(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels, Image *img);
    void setCompressedImage(GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels, Image *img);
    bool subImageCompressed(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels, Image *img);
    void copyNonRenderable(Image *image, GLenum internalFormat, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, IDirect3DSurface9 *renderTarget);

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
    void createSurface(GLsizei width, GLsizei height, GLenum format, GLenum type, Image *img);

    Blit *getBlitter();

    int levelCount() const;

    bool isRenderable() const;

    unsigned int mWidth;
    unsigned int mHeight;
    GLenum mMinFilter;
    GLenum mMagFilter;
    GLenum mWrapS;
    GLenum mWrapT;
    GLenum mType;

    bool mDirtyMetaData;

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture);

    void loadImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type,
                       GLint unpackAlignment, const void *input, std::size_t outputPitch, void *output, D3DSURFACE_DESC *description) const;

    void loadAlphaImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                            size_t inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadAlphaFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                 size_t inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadAlphaHalfFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                     size_t inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadLuminanceImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                size_t inputPitch, const void *input, size_t outputPitch, void *output, bool native) const;
    void loadLuminanceFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                     size_t inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadLuminanceHalfFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                         size_t inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadLuminanceAlphaImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                     size_t inputPitch, const void *input, size_t outputPitch, void *output, bool native) const;
    void loadLuminanceAlphaFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                          size_t inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadLuminanceAlphaHalfFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                              size_t inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadRGBUByteImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                               size_t inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadRGB565ImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                             size_t inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadRGBFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                               size_t inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadRGBHalfFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                   size_t inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadRGBAUByteImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                size_t inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadRGBA4444ImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                               size_t inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadRGBA5551ImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                               size_t inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadRGBAFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                size_t inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadRGBAHalfFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                    size_t inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadBGRAImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                           size_t inputPitch, const void *input, size_t outputPitch, void *output) const;

    IDirect3DBaseTexture9 *mBaseTexture; 

    bool mDirty;
    bool mIsRenderable;
};

class Texture2D : public Texture
{
  public:
    explicit Texture2D(GLuint id);

    ~Texture2D();

    GLenum getTarget() const;
    GLenum getFormat() const;

    void setImage(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setCompressedImage(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels);
    void subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels);
    void copyImage(GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, RenderbufferStorage *source);
    void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, RenderbufferStorage *source);

    bool isComplete() const;
    bool isCompressed() const;

    virtual void generateMipmaps();

    virtual Renderbuffer *getColorbuffer(GLenum target);

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture2D);

    virtual IDirect3DBaseTexture9 *createTexture();
    virtual void updateTexture();
    virtual IDirect3DBaseTexture9 *convertToRenderTarget();
    virtual IDirect3DSurface9 *getRenderTarget(GLenum target);

    virtual bool dirtyImageData() const;

    bool redefineTexture(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum type);
    void commitRect(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);

    Image mImageArray[IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    IDirect3DTexture9 *mTexture;

    BindingPointer<Renderbuffer> mColorbufferProxy;
};

class TextureCubeMap : public Texture
{
  public:
    explicit TextureCubeMap(GLuint id);

    ~TextureCubeMap();

    GLenum getTarget() const;
    GLenum getFormat() const;

    void setImagePosX(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImageNegX(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImagePosY(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImageNegY(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImagePosZ(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImageNegZ(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);

    void setCompressedImage(GLenum face, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels);

    void subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels);
    void copyImage(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, RenderbufferStorage *source);
    void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, RenderbufferStorage *source);

    bool isComplete() const;
    bool isCompressed() const;

    virtual void generateMipmaps();

    virtual Renderbuffer *getColorbuffer(GLenum target);

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureCubeMap);

    virtual IDirect3DBaseTexture9 *createTexture();
    virtual void updateTexture();
    virtual IDirect3DBaseTexture9 *convertToRenderTarget();
    virtual IDirect3DSurface9 *getRenderTarget(GLenum target);

    virtual bool dirtyImageData() const;

    
    
    IDirect3DSurface9 *getCubeMapSurface(unsigned int faceIdentifier, unsigned int level);

    static unsigned int faceIndex(GLenum face);

    bool isCubeComplete() const;

    void setImage(int face, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void commitRect(GLenum faceTarget, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);
    bool redefineTexture(GLint level, GLenum internalFormat, GLsizei width);

    Image mImageArray[6][IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    IDirect3DCubeTexture9 *mTexture;

    BindingPointer<Renderbuffer> mFaceProxies[6];
};
}

#endif   
