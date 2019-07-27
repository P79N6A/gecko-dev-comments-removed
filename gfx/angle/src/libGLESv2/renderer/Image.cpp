









#include "libGLESv2/renderer/Image.h"

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

}
