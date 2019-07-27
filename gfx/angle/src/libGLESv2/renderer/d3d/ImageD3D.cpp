









#include "libGLESv2/renderer/d3d/ImageD3D.h"

namespace rx
{

ImageD3D::ImageD3D()
{
}

ImageD3D *ImageD3D::makeImageD3D(Image *img)
{
    ASSERT(HAS_DYNAMIC_TYPE(rx::ImageD3D*, img));
    return static_cast<rx::ImageD3D*>(img);
}

}
