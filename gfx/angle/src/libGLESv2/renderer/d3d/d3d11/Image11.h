








#ifndef LIBGLESV2_RENDERER_IMAGE11_H_
#define LIBGLESV2_RENDERER_IMAGE11_H_

#include "libGLESv2/renderer/d3d/ImageD3D.h"
#include "libGLESv2/ImageIndex.h"

#include "common/debug.h"

namespace gl
{
class Framebuffer;
}

namespace rx
{
class Renderer;
class Renderer11;
class TextureStorage11;

class Image11 : public ImageD3D
{
  public:
    Image11();
    virtual ~Image11();

    static Image11 *makeImage11(Image *img);

    static void generateMipmap(Image11 *dest, Image11 *src);

    virtual bool isDirty() const;

    virtual gl::Error copyToStorage2D(TextureStorage *storage, const gl::ImageIndex &index, const gl::Box &region);
    virtual gl::Error copyToStorageCube(TextureStorage *storage, const gl::ImageIndex &index, const gl::Box &region);
    virtual gl::Error copyToStorage3D(TextureStorage *storage, const gl::ImageIndex &index, const gl::Box &region);
    virtual gl::Error copyToStorage2DArray(TextureStorage *storage, const gl::ImageIndex &index, const gl::Box &region);

    virtual bool redefine(Renderer *renderer, GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, bool forceRelease);

    DXGI_FORMAT getDXGIFormat() const;

    virtual gl::Error loadData(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                               GLint unpackAlignment, GLenum type, const void *input);
    virtual gl::Error loadCompressedData(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                                         const void *input);

    virtual void copy(GLint xoffset, GLint yoffset, GLint zoffset, const gl::Rectangle &sourceArea, RenderTarget *source);
    virtual void copy(GLint xoffset, GLint yoffset, GLint zoffset, const gl::Rectangle &sourceArea,
                      const gl::ImageIndex &sourceIndex, TextureStorage *source);

    bool recoverFromAssociatedStorage();
    bool isAssociatedStorageValid(TextureStorage11* textureStorage) const;
    void disassociateStorage();

  protected:
    HRESULT map(D3D11_MAP mapType, D3D11_MAPPED_SUBRESOURCE *map);
    void unmap();

  private:
    DISALLOW_COPY_AND_ASSIGN(Image11);

    gl::Error copyToStorageImpl(TextureStorage11 *storage11, const gl::ImageIndex &index, const gl::Box &region);
    void copy(GLint xoffset, GLint yoffset, GLint zoffset, const gl::Rectangle &sourceArea, ID3D11Texture2D *source, UINT sourceSubResource);

    ID3D11Resource *getStagingTexture();
    unsigned int getStagingSubresource();
    void createStagingTexture();
    void releaseStagingTexture();

    Renderer11 *mRenderer;

    DXGI_FORMAT mDXGIFormat;
    ID3D11Resource *mStagingTexture;
    unsigned int mStagingSubresource;

    bool mRecoverFromStorage;
    TextureStorage11 *mAssociatedStorage;
    gl::ImageIndex mAssociatedImageIndex;
    unsigned int mRecoveredFromStorageCount;
};

}

#endif 
