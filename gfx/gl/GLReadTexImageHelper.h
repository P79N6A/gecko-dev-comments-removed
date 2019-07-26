





#ifndef GLREADTEXIMAGEHELPER_H_
#define GLREADTEXIMAGEHELPER_H_

#include "GLContextTypes.h"
#include "mozilla/Attributes.h"
#include "nsSize.h"
#include "nsAutoPtr.h"

class gfxImageSurface;

namespace mozilla {
namespace gl {

class GLReadTexImageHelper MOZ_FINAL
{
    
    GLContext* mGL;

    GLuint mPrograms[4];

    GLuint TextureImageProgramFor(GLenum aTextureTarget, int aShader);
    bool ReadBackPixelsIntoSurface(gfxImageSurface* aSurface, const gfxIntSize& aSize);

    bool DidGLErrorOccur(const char* str);

public:

    GLReadTexImageHelper(GLContext* gl);
    ~GLReadTexImageHelper();

    













    already_AddRefed<gfxImageSurface> ReadTexImage(GLuint aTextureId,
                                                   GLenum aTextureTarget,
                                                   const gfxIntSize& aSize,
                            int aShaderProgram,
                                                   bool aYInvert = false);


};

}
}

#endif
