





#ifndef GLBLITTEXTUREIMAGEHELPER_H_
#define GLBLITTEXTUREIMAGEHELPER_H_

#include "mozilla/Attributes.h"
#include "GLContextTypes.h"
#include "GLConsts.h"
#include "mozilla/gfx/Rect.h"

namespace mozilla {
namespace gl {
    class GLContext;
    class TextureImage;
}
namespace layers {

class CompositorOGL;

class GLBlitTextureImageHelper final
{
    
    CompositorOGL* mCompositor;

    
    GLuint mBlitProgram, mBlitFramebuffer;
    void UseBlitProgram();
    void SetBlitFramebufferForDestTexture(GLuint aTexture);

public:

    explicit GLBlitTextureImageHelper(CompositorOGL *gl);
    ~GLBlitTextureImageHelper();

    
























    void BlitTextureImage(gl::TextureImage *aSrc, const gfx::IntRect& aSrcRect,
                          gl::TextureImage *aDst, const gfx::IntRect& aDstRect);
};

}
}

#endif 
