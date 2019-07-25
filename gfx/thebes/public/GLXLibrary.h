




































#include "GLContext.h"
typedef realGLboolean GLboolean;
#include <GL/glx.h>

namespace mozilla {
namespace gl {

class GLXLibrary
{
public:
    GLXLibrary() : mInitialized(PR_FALSE), mOGLLibrary(nsnull) {}

    typedef GLXContext (GLAPIENTRY * PFNGLXCREATECONTEXTPROC) (Display*,
                                                               XVisualInfo*,
                                                               GLXContext,
                                                               Bool);
    PFNGLXCREATECONTEXTPROC xCreateContext;
    typedef void (GLAPIENTRY * PFNGLXDELETECONTEXTPROC) (Display*,
                                                         GLXContext);
    PFNGLXDELETECONTEXTPROC xDeleteContext;
    typedef Bool (GLAPIENTRY * PFNGLXMAKECURRENTPROC) (Display*,
                                                       GLXDrawable,
                                                       GLXContext);
    PFNGLXMAKECURRENTPROC xMakeCurrent;
    typedef void* (GLAPIENTRY * PFNGLXGETPROCADDRESSPROC) (const char *);
    PFNGLXGETPROCADDRESSPROC xGetProcAddress;
    typedef XVisualInfo* (GLAPIENTRY * PFNGLXCHOOSEVISUALPROC) (Display*,
                                                                int,
                                                                int *);
    PFNGLXCHOOSEVISUALPROC xChooseVisual;
    typedef GLXFBConfig* (GLAPIENTRY * PFNGLXCHOOSEFBCONFIG) (Display *,
                                                              int,
                                                              const int *,
                                                              int *);
    PFNGLXCHOOSEFBCONFIG xChooseFBConfig;
    typedef GLXPbuffer (GLAPIENTRY * PFNGLXCREATEPBUFFER) (Display *,
                                                           GLXFBConfig,
                                                           const int *);
    PFNGLXCREATEPBUFFER xCreatePbuffer;
    typedef GLXContext (GLAPIENTRY * PFNGLXCREATENEWCONTEXT) (Display *,
                                                              GLXFBConfig,
                                                              int,
                                                              GLXContext,
                                                              Bool);
    PFNGLXCREATENEWCONTEXT xCreateNewContext;
    typedef void (GLAPIENTRY * PFNGLXDESTROYPBUFFER) (Display *,
                                                      GLXPbuffer);
    PFNGLXDESTROYPBUFFER xDestroyPbuffer;

    typedef XVisualInfo* (GLAPIENTRY * PFNGLXGETVISUALFROMFBCONFIG) (Display *,
                                                                     GLXFBConfig);
    PFNGLXGETVISUALFROMFBCONFIG xGetVisualFromFBConfig;
    
    PRBool EnsureInitialized();

private:
    PRBool mInitialized;
    PRLibrary *mOGLLibrary;
};


extern GLXLibrary sGLXLibrary;

} 
} 

