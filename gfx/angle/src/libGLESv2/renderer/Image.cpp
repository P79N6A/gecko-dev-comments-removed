









#include "libGLESv2/renderer/Image.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/main.h"

namespace rx
{

Image::Image()
{
    mWidth = 0;
    mHeight = 0;
    mDepth = 0;
    mInternalFormat = GL_NONE;
    mActualFormat = GL_NONE;
    mTarget = GL_NONE;
    mRenderable = false;
    mDirty = false;
}

gl::Error Image::copy(GLint xoffset, GLint yoffset, GLint zoffset, const gl::Rectangle &area, gl::Framebuffer *source)
{
    gl::FramebufferAttachment *colorbuffer = source->getReadColorbuffer();
    ASSERT(colorbuffer);

    RenderTarget *renderTarget = NULL;
    gl::Error error = GetAttachmentRenderTarget(colorbuffer, &renderTarget);
    if (error.isError())
    {
        return error;
    }

    ASSERT(renderTarget);
    return copy(xoffset, yoffset, zoffset, area, renderTarget);
}

}
