




































#include "GLContext.h"
typedef realGLboolean GLboolean;
#include <GL/glx.h>

namespace mozilla {
namespace gl {

class GLXLibrary
{
public:
    GLXLibrary() : mInitialized(PR_FALSE), mTriedInitializing(PR_FALSE),
                   mOGLLibrary(nsnull) {}

    typedef void (GLAPIENTRY * PFNGLXDESTROYCONTEXTPROC) (Display*,
                                                          GLXContext);
    PFNGLXDESTROYCONTEXTPROC xDestroyContext;
    typedef Bool (GLAPIENTRY * PFNGLXMAKECURRENTPROC) (Display*,
                                                       GLXDrawable,
                                                       GLXContext);
    PFNGLXMAKECURRENTPROC xMakeCurrent;
    typedef GLXContext (GLAPIENTRY * PFNGLXGETCURRENTCONTEXT) ();
    PFNGLXGETCURRENTCONTEXT xGetCurrentContext;
    typedef void* (GLAPIENTRY * PFNGLXGETPROCADDRESSPROC) (const char *);
    PFNGLXGETPROCADDRESSPROC xGetProcAddress;
    typedef GLXFBConfig* (GLAPIENTRY * PFNGLXCHOOSEFBCONFIG) (Display *,
                                                              int,
                                                              const int *,
                                                              int *);
    PFNGLXCHOOSEFBCONFIG xChooseFBConfig;
    typedef GLXFBConfig* (GLAPIENTRY * PFNGLXGETFBCONFIGS) (Display *,
                                                            int,
                                                            int *);
    PFNGLXGETFBCONFIGS xGetFBConfigs;
    typedef GLXContext (GLAPIENTRY * PFNGLXCREATENEWCONTEXT) (Display *,
                                                              GLXFBConfig,
                                                              int,
                                                              GLXContext,
                                                              Bool);
    PFNGLXCREATENEWCONTEXT xCreateNewContext;
    typedef XVisualInfo* (GLAPIENTRY * PFNGLXGETVISUALFROMFBCONFIG) (Display *,
                                                                     GLXFBConfig);
    PFNGLXGETVISUALFROMFBCONFIG xGetVisualFromFBConfig;
    typedef int (GLAPIENTRY * PFNGLXGETFBCONFIGATTRIB) (Display *, 
                                                        GLXFBConfig,
                                                        int,
                                                        int *);
    PFNGLXGETFBCONFIGATTRIB xGetFBConfigAttrib;

    typedef void (GLAPIENTRY * PFNGLXSWAPBUFFERS) (Display *,
                                                   GLXDrawable);
    PFNGLXSWAPBUFFERS xSwapBuffers;
    typedef const char * (GLAPIENTRY * PFNGLXQUERYEXTENSIONSSTRING) (Display *,
                                                                     int);
    PFNGLXQUERYEXTENSIONSSTRING xQueryExtensionsString;
    typedef const char * (GLAPIENTRY * PFNGLXQUERYSERVERSTRING) (Display *,
                                                                 int,
                                                                 int);
    PFNGLXQUERYSERVERSTRING xQueryServerString;

    typedef GLXPixmap (GLAPIENTRY * PFNGLXCREATEPIXMAP) (Display *,
                                                         GLXFBConfig,
                                                         Pixmap,
                                                         const int *);
    PFNGLXCREATEPIXMAP xCreatePixmap;
    typedef void (GLAPIENTRY * PFNGLXDESTROYPIXMAP) (Display *,
                                                     GLXPixmap);
    PFNGLXDESTROYPIXMAP xDestroyPixmap;
    typedef const char * (GLAPIENTRY * PFNGLXGETCLIENTSTRING) (Display *,
                                                               int);
    PFNGLXGETCLIENTSTRING xGetClientString;
    typedef GLXContext (GLAPIENTRY * PFNGLXCREATECONTEXT) (Display *,
                                                           XVisualInfo *,
                                                           GLXContext,
                                                           Bool);
    PFNGLXCREATECONTEXT xCreateContext;

    PRBool EnsureInitialized();

private:
    PRBool mInitialized;
    PRBool mTriedInitializing;
    PRLibrary *mOGLLibrary;
};


extern GLXLibrary sGLXLibrary;

} 
} 

