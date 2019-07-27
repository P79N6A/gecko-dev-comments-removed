





#ifndef GLREADTEXIMAGEHELPER_H_
#define GLREADTEXIMAGEHELPER_H_

#include "GLContextTypes.h"
#include "mozilla/Attributes.h"
#include "nsSize.h"
#include "nsAutoPtr.h"
#include "mozilla/RefPtr.h"
#include "mozilla/gfx/Types.h"

namespace mozilla {

namespace gfx {
class DataSourceSurface;
}

namespace gl {



bool GetActualReadFormats(GLContext* gl,
                          GLenum destFormat, GLenum destType,
                          GLenum* out_readFormat, GLenum* out_readType);

void ReadPixelsIntoDataSurface(GLContext* aGL,
                               gfx::DataSourceSurface* aSurface);

already_AddRefed<gfx::DataSourceSurface>
ReadBackSurface(GLContext* gl, GLuint aTexture, bool aYInvert, gfx::SurfaceFormat aFormat);

class GLReadTexImageHelper final
{
    
    GLContext* mGL;

    GLuint mPrograms[4];

    GLuint TextureImageProgramFor(GLenum aTextureTarget, int aShader);

    bool DidGLErrorOccur(const char* str);

public:

    explicit GLReadTexImageHelper(GLContext* gl);
    ~GLReadTexImageHelper();

    













    already_AddRefed<gfx::DataSourceSurface> ReadTexImage(GLuint aTextureId,
                                                      GLenum aTextureTarget,
                                                      const gfx::IntSize& aSize,
                               int aShaderProgram,
                                                      bool aYInvert = false);


};

}
}

#endif
