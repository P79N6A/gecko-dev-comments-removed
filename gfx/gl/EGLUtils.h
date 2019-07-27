



#ifndef EGLUTILS_H_
#define EGLUTILS_H_

#include "GLContextTypes.h"
#include "GLTypes.h"
#include "mozilla/Assertions.h"

namespace mozilla {
namespace gl {

class GLLibraryEGL;

bool DoesEGLContextSupportSharingWithEGLImage(GLContext* gl);
EGLImage CreateEGLImage(GLContext* gl, GLuint tex);




class EGLImageWrapper
{
public:
    static EGLImageWrapper* Create(GLContext* gl, GLuint tex);

private:
    GLLibraryEGL& mLibrary;
    const EGLDisplay mDisplay;
public:
    const EGLImage mImage;
private:
    EGLSync mSync;

    EGLImageWrapper(GLLibraryEGL& library,
                    EGLDisplay display,
                    EGLImage image)
        : mLibrary(library)
        , mDisplay(display)
        , mImage(image)
        , mSync(0)
    {
        MOZ_ASSERT(mImage);
    }

public:
    ~EGLImageWrapper();

    
    
    bool FenceSync(GLContext* gl);

    bool ClientWaitSync();
};

} 
} 

#endif
