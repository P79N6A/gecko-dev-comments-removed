








#ifndef LIBGLESV2_RENDERER_IMAGE9_H_
#define LIBGLESV2_RENDERER_IMAGE9_H_

#include "libGLESv2/renderer/d3d/ImageD3D.h"
#include "common/debug.h"

namespace gl
{
class Framebuffer;
}

namespace rx
{
class Renderer;
class Renderer9;

class Image9 : public ImageD3D
{
  public:
    Image9();
    ~Image9();

    static Image9 *makeImage9(Image *img);

    static void generateMipmap(Image9 *dest, Image9 *source);
    static void generateMip(IDirect3DSurface9 *destSurface, IDirect3DSurface9 *sourceSurface);
    static void copyLockableSurfaces(IDirect3DSurface9 *dest, IDirect3DSurface9 *source);

    virtual bool redefine(Renderer *renderer, GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, bool forceRelease);

    D3DFORMAT getD3DFormat() const;

    virtual bool isDirty() const;
    IDirect3DSurface9 *getSurface();

    virtual void setManagedSurface2D(TextureStorage *storage, int level);
    virtual void setManagedSurfaceCube(TextureStorage *storage, int face, int level);
    virtual gl::Error copyToStorage2D(TextureStorage *storage, const gl::ImageIndex &index, const gl::Box &region);
    virtual gl::Error copyToStorageCube(TextureStorage *storage, const gl::ImageIndex &index, const gl::Box &region);
    virtual gl::Error copyToStorage3D(TextureStorage *storage, const gl::ImageIndex &index, const gl::Box &region);
    virtual gl::Error copyToStorage2DArray(TextureStorage *storage, const gl::ImageIndex &index, const gl::Box &region);

    virtual gl::Error loadData(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                               GLint unpackAlignment, GLenum type, const void *input);
    virtual gl::Error loadCompressedData(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                                         const void *input);

    virtual void copy(GLint xoffset, GLint yoffset, GLint zoffset, const gl::Rectangle &sourceArea, RenderTarget *source);
    virtual void copy(GLint xoffset, GLint yoffset, GLint zoffset, const gl::Rectangle &sourceArea,
                      const gl::ImageIndex &sourceIndex, TextureStorage *source);

  private:
    DISALLOW_COPY_AND_ASSIGN(Image9);

    void createSurface();
    void setManagedSurface(IDirect3DSurface9 *surface);
    gl::Error copyToSurface(IDirect3DSurface9 *dest, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);

    HRESULT lock(D3DLOCKED_RECT *lockedRect, const RECT *rect);
    void unlock();

    Renderer9 *mRenderer;

    D3DPOOL mD3DPool;   
    D3DFORMAT mD3DFormat;

    IDirect3DSurface9 *mSurface;
};
}

#endif   
