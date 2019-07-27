





#ifndef GLBLITTEXTUREIMAGEHELPER_H_
#define GLBLITTEXTUREIMAGEHELPER_H_

#include "mozilla/Attributes.h"
#include "GLContextTypes.h"
#include "GLConsts.h"

struct nsIntRect;

namespace mozilla {
namespace gl {

class GLContext;
class TextureImage;

class GLBlitTextureImageHelper MOZ_FINAL
{
    
    GLContext* mGL;

    
    GLProgram mBlitProgram;
    GLuint mBlitFramebuffer;
    void UseBlitProgram();
    void SetBlitFramebufferForDestTexture(GLuint aTexture);

public:

    GLBlitTextureImageHelper(GLContext *gl);
    ~GLBlitTextureImageHelper();

    
























    void BlitTextureImage(TextureImage *aSrc, const nsIntRect& aSrcRect,
                          TextureImage *aDst, const nsIntRect& aDstRect);
};

}
}

#endif 
