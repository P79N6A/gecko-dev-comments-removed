




































#include "GLContext.h"
typedef realGLboolean GLboolean;
#include <GL/glx.h>

namespace mozilla {
namespace gl {

class GLXLibrary
{
public:
    GLXLibrary() : mInitialized(PR_FALSE), mOGLLibrary(nsnull) {}

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
    typedef GLXFBConfig* (GLAPIENTRY * PFNGLXGETFBCONFIGS) (Display *,
                                                            int,
                                                            int *);
    PFNGLXGETFBCONFIGS xGetFBConfigs;
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
    typedef int (GLAPIENTRY * PFNGLXGETFBCONFIGATTRIB) (Display *, 
                                                        GLXFBConfig,
                                                        int,
                                                        int *);
    PFNGLXGETFBCONFIGATTRIB xGetFBConfigAttrib;

    typedef void (GLAPIENTRY * PFNGLXSWAPBUFFERS) (Display *,
                                                   GLXDrawable);
    PFNGLXSWAPBUFFERS xSwapBuffers;
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
    typedef int (GLAPIENTRY * PFNGLXGETCONFIG) (Display *,
                                                XVisualInfo *,
                                                int,
                                                int *);
    PFNGLXGETCONFIG xGetConfig;
    typedef GLXPixmap (GLAPIENTRY * PFNGLXCREATEGLXPIXMAP) (Display *,
                                                            XVisualInfo *,
                                                            Pixmap);
    PFNGLXCREATEGLXPIXMAP xCreateGLXPixmap;

    PRBool EnsureInitialized();

private:
    PRBool mInitialized;
    PRLibrary *mOGLLibrary;
};


extern GLXLibrary sGLXLibrary;

} 
} 

