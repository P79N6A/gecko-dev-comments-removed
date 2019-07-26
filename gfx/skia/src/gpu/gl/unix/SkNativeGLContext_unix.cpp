






#include "gl/SkNativeGLContext.h"

#include <GL/glu.h>

#define GLX_1_3 1

SkNativeGLContext::AutoContextRestore::AutoContextRestore() {
    fOldGLXContext = glXGetCurrentContext();
    fOldDisplay = glXGetCurrentDisplay();
    fOldDrawable = glXGetCurrentDrawable();
}

SkNativeGLContext::AutoContextRestore::~AutoContextRestore() {
    if (NULL != fOldDisplay) {
        glXMakeCurrent(fOldDisplay, fOldDrawable, fOldGLXContext);
    }
}



static bool ctxErrorOccurred = false;
static int ctxErrorHandler(Display *dpy, XErrorEvent *ev) {
    ctxErrorOccurred = true;
    return 0;
}

SkNativeGLContext::SkNativeGLContext()
    : fContext(NULL)
    , fDisplay(NULL)
    , fPixmap(0)
    , fGlxPixmap(0) {
}

SkNativeGLContext::~SkNativeGLContext() {
    this->destroyGLContext();
}

void SkNativeGLContext::destroyGLContext() {
    if (fDisplay) {
        glXMakeCurrent(fDisplay, 0, 0);

        if (fContext) {
            glXDestroyContext(fDisplay, fContext);
            fContext = NULL;
        }

        if (fGlxPixmap) {
            glXDestroyGLXPixmap(fDisplay, fGlxPixmap);
            fGlxPixmap = 0;
        }

        if (fPixmap) {
            XFreePixmap(fDisplay, fPixmap);
            fPixmap = 0;
        }

        XCloseDisplay(fDisplay);
        fDisplay = NULL;
    }
}

const GrGLInterface* SkNativeGLContext::createGLContext() {
    fDisplay = XOpenDisplay(0);

    if (!fDisplay) {
        SkDebugf("Failed to open X display.\n");
        this->destroyGLContext();
        return NULL;
    }

    
    static int visual_attribs[] = {
        GLX_X_RENDERABLE    , True,
        GLX_DRAWABLE_TYPE   , GLX_PIXMAP_BIT,
        None
    };

#ifdef GLX_1_3
    
    int fbcount;
    GLXFBConfig *fbc = glXChooseFBConfig(fDisplay, DefaultScreen(fDisplay),
                                          visual_attribs, &fbcount);
    if (!fbc) {
        SkDebugf("Failed to retrieve a framebuffer config.\n");
        this->destroyGLContext();
        return NULL;
    }
    

    
    
    int best_fbc = -1, best_num_samp = -1;

    int i;
    for (i = 0; i < fbcount; ++i) {
        XVisualInfo *vi = glXGetVisualFromFBConfig(fDisplay, fbc[i]);
        if (vi) {
            int samp_buf, samples;
            glXGetFBConfigAttrib(fDisplay, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf);
            glXGetFBConfigAttrib(fDisplay, fbc[i], GLX_SAMPLES, &samples);

            
            
            

            if (best_fbc < 0 || (samp_buf && samples > best_num_samp))
                best_fbc = i, best_num_samp = samples;
        }
        XFree(vi);
    }

    GLXFBConfig bestFbc = fbc[best_fbc];

    
    XFree(fbc);

    
    XVisualInfo *vi = glXGetVisualFromFBConfig(fDisplay, bestFbc);
    
#else
    int numVisuals;
    XVisualInfo visTemplate, *visReturn;

    visReturn = XGetVisualInfo(fDisplay, VisualNoMask, &visTemplate, &numVisuals);
    if (NULL == visReturn)
    {
        SkDebugf("Failed to get visual information.\n");
        this->destroyGLContext();
        return NULL;
    }

    int best = -1, best_num_samp = -1;

    for (int i = 0; i < numVisuals; ++i)
    {
        int samp_buf, samples;

        glXGetConfig(fDisplay, &visReturn[i], GLX_SAMPLE_BUFFERS, &samp_buf);
        glXGetConfig(fDisplay, &visReturn[i], GLX_SAMPLES, &samples);

        if (best < 0 || (samp_buf && samples > best_num_samp))
            best = i, best_num_samp = samples;
    }

    XVisualInfo temp = visReturn[best];
    XVisualInfo *vi = &temp;

    XFree(visReturn);
#endif

    fPixmap = XCreatePixmap(fDisplay, RootWindow(fDisplay, vi->screen), 10, 10, vi->depth);

    if (!fPixmap) {
        SkDebugf("Failed to create pixmap.\n");
        this->destroyGLContext();
        return NULL;
    }

    fGlxPixmap = glXCreateGLXPixmap(fDisplay, vi, fPixmap);

#ifdef GLX_1_3
    
    XFree(vi);
#endif

    

    
    
    
    
    
    
    
    ctxErrorOccurred = false;
    int (*oldHandler)(Display*, XErrorEvent*) =
        XSetErrorHandler(&ctxErrorHandler);

    
    const char *glxExts = glXQueryExtensionsString(
        fDisplay, DefaultScreen(fDisplay)
    );
    
    
    if (!gluCheckExtension(
          reinterpret_cast<const GLubyte*>("GLX_ARB_create_context")
          , reinterpret_cast<const GLubyte*>(glxExts)))
    {
        
        
#ifdef GLX_1_3
        fContext = glXCreateNewContext(fDisplay, bestFbc, GLX_RGBA_TYPE, 0, True);
#else
        fContext = glXCreateContext(fDisplay, vi, 0, True);
#endif

    }
#ifdef GLX_1_3
    else {
        

        PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB =
            (PFNGLXCREATECONTEXTATTRIBSARBPROC) glXGetProcAddressARB((GrGLubyte*)"glXCreateContextAttribsARB");
        int context_attribs[] = {
            GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
            GLX_CONTEXT_MINOR_VERSION_ARB, 0,
            
            None
        };
        fContext = glXCreateContextAttribsARB(
            fDisplay, bestFbc, 0, True, context_attribs
        );

        
        XSync(fDisplay, False);
        if (!ctxErrorOccurred && fContext) {
           
        } else {
            
            
            
            
            

            
            context_attribs[1] = 1;
            
            context_attribs[3] = 0;

            ctxErrorOccurred = false;

            
            
            fContext = glXCreateContextAttribsARB(
                fDisplay, bestFbc, 0, True, context_attribs
            );
        }
    }
#endif

    
    XSync(fDisplay, False);

    
    XSetErrorHandler(oldHandler);

    if (ctxErrorOccurred || !fContext) {
        SkDebugf("Failed to create an OpenGL context.\n");
        this->destroyGLContext();
        return NULL;
    }

    
    if (!glXIsDirect(fDisplay, fContext)) {
        
    } else {
        
    }

    
    if (!glXMakeCurrent(fDisplay, fGlxPixmap, fContext)) {
      SkDebugf("Could not set the context.\n");
        this->destroyGLContext();
        return NULL;
    }

    const GrGLInterface* interface = GrGLCreateNativeInterface();
    if (!interface) {
        SkDebugf("Failed to create gl interface");
        this->destroyGLContext();
        return NULL;
    }
    return interface;
}

void SkNativeGLContext::makeCurrent() const {
    if (!glXMakeCurrent(fDisplay, fGlxPixmap, fContext)) {
        SkDebugf("Could not set the context.\n");
    }
}
