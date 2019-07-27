









#ifndef LIBGLESV2_RENDERER_IMAGED3D_H_
#define LIBGLESV2_RENDERER_IMAGED3D_H_

#include "common/debug.h"
#include "libGLESv2/renderer/Image.h"

namespace gl
{
class Framebuffer;
struct ImageIndex;
struct Box;
}

namespace rx
{
class TextureStorage;

class ImageD3D : public Image
{
  public:
    ImageD3D();
    virtual ~ImageD3D() {};

    static ImageD3D *makeImageD3D(Image *img);

    virtual bool isDirty() const = 0;

    virtual void setManagedSurface2D(TextureStorage *storage, int level) {};
    virtual void setManagedSurfaceCube(TextureStorage *storage, int face, int level) {};
    virtual void setManagedSurface3D(TextureStorage *storage, int level) {};
    virtual void setManagedSurface2DArray(TextureStorage *storage, int layer, int level) {};
    virtual gl::Error copyToStorage2D(TextureStorage *storage, const gl::ImageIndex &index, const gl::Box &region) = 0;
    virtual gl::Error copyToStorageCube(TextureStorage *storage, const gl::ImageIndex &index, const gl::Box &region) = 0;
    virtual gl::Error copyToStorage3D(TextureStorage *storage, const gl::ImageIndex &index, const gl::Box &region) = 0;
    virtual gl::Error copyToStorage2DArray(TextureStorage *storage, const gl::ImageIndex &index, const gl::Box &region) = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(ImageD3D);
};

}

#endif 
