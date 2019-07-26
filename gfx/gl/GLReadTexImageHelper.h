





#ifndef GLREADTEXIMAGEHELPER_H_
#define GLREADTEXIMAGEHELPER_H_

#include "GLContextTypes.h"
#include "mozilla/Attributes.h"
#include "nsSize.h"
#include "nsAutoPtr.h"
#include "mozilla/RefPtr.h"
#include "mozilla/gfx/Types.h"

class gfxImageSurface;

namespace mozilla {

namespace gfx {
class DataSourceSurface;
}

namespace gl {

void ReadPixelsIntoDataSurface(GLContext* aGL,
                               gfx::DataSourceSurface* aSurface);
void ReadPixelsIntoImageSurface(GLContext* aGL, gfxImageSurface* aSurface);
void ReadScreenIntoImageSurface(GLContext* aGL, gfxImageSurface* aSurface);

TemporaryRef<gfx::DataSourceSurface>
ReadBackSurface(GLContext* gl, GLuint aTexture, bool aYInvert, gfx::SurfaceFormat aFormat);

class GLReadTexImageHelper MOZ_FINAL
{
    
    GLContext* mGL;

    GLuint mPrograms[4];

    GLuint TextureImageProgramFor(GLenum aTextureTarget, int aShader);

    bool DidGLErrorOccur(const char* str);

public:

    GLReadTexImageHelper(GLContext* gl);
    ~GLReadTexImageHelper();

    













    TemporaryRef<gfx::DataSourceSurface> ReadTexImage(GLuint aTextureId,
                                                      GLenum aTextureTarget,
                                                      const gfx::IntSize& aSize,
                               int aShaderProgram,
                                                      bool aYInvert = false);


};

}
}

#endif
