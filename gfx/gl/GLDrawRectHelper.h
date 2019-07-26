





#ifndef GLDRAWRECTHELPER_H_
#define GLDRAWRECTHELPER_H_

#include "GLContextTypes.h"
#include "GLConsts.h"

namespace mozilla {
namespace gl {

class GLContext;
class RectTriangles;


class GLDrawRectHelper MOZ_FINAL
{
public:
    GLDrawRectHelper(GLContext* aGL);
    ~GLDrawRectHelper();

    void DrawRect(GLuint aVertAttribIndex,
                  GLuint aTexCoordAttribIndex);
    void DrawRects(GLuint aVertAttribIndex,
                   GLuint aTexCoordAttribIndex,
                   RectTriangles& aRects);

private:
    
    GLContext* mGL;
    GLuint mQuadVBO;
};

}
}

#endif 
