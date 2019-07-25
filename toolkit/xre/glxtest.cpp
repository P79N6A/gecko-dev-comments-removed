




















































#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <dlfcn.h>
#include "nscore.h"

namespace mozilla {
namespace widget {

extern int glxtest_pipe;

extern pid_t glxtest_pid;
}
}


static int write_end_of_the_pipe = -1;




template<typename func_ptr_type>
static func_ptr_type cast(void *ptr)
{
  return reinterpret_cast<func_ptr_type>(
           reinterpret_cast<size_t>(ptr)
         );
}

static void fatal_error(const char *str)
{
  write(write_end_of_the_pipe, str, strlen(str));
  write(write_end_of_the_pipe, "\n", 1);
  exit(EXIT_FAILURE);
}

static int
x_error_handler(Display *, XErrorEvent *ev)
{
  enum { bufsize = 1024 };
  char buf[bufsize];
  int length = snprintf(buf, bufsize,
                        "X error occurred in GLX probe, error_code=%d, request_code=%d, minor_code=%d\n",
                        ev->error_code,
                        ev->request_code,
                        ev->minor_code);
  write(write_end_of_the_pipe, buf, length);
  exit(EXIT_FAILURE);
  return 0;
}

static void glxtest()
{
  
  void *libgl = dlopen("libGL.so.1", RTLD_LAZY);
  if (!libgl)
    fatal_error("Unable to load libGL.so.1");

  typedef GLXFBConfig* (* PFNGLXQUERYEXTENSION) (Display *, int *, int *);
  PFNGLXQUERYEXTENSION glXQueryExtension = cast<PFNGLXQUERYEXTENSION>(dlsym(libgl, "glXQueryExtension"));

  typedef GLXFBConfig* (* PFNGLXCHOOSEFBCONFIG) (Display *, int, const int *, int *);
  PFNGLXCHOOSEFBCONFIG glXChooseFBConfig = cast<PFNGLXCHOOSEFBCONFIG>(dlsym(libgl, "glXChooseFBConfig"));

  typedef XVisualInfo* (* PFNGLXGETVISUALFROMFBCONFIG) (Display *, GLXFBConfig);
  PFNGLXGETVISUALFROMFBCONFIG glXGetVisualFromFBConfig = cast<PFNGLXGETVISUALFROMFBCONFIG>(dlsym(libgl, "glXGetVisualFromFBConfig"));

  typedef GLXPixmap (* PFNGLXCREATEPIXMAP) (Display *, GLXFBConfig, Pixmap, const int *);
  PFNGLXCREATEPIXMAP glXCreatePixmap = cast<PFNGLXCREATEPIXMAP>(dlsym(libgl, "glXCreatePixmap"));

  typedef GLXContext (* PFNGLXCREATENEWCONTEXT) (Display *, GLXFBConfig, int, GLXContext, Bool);
  PFNGLXCREATENEWCONTEXT glXCreateNewContext = cast<PFNGLXCREATENEWCONTEXT>(dlsym(libgl, "glXCreateNewContext"));

  typedef Bool (* PFNGLXMAKECURRENT) (Display*, GLXDrawable, GLXContext);
  PFNGLXMAKECURRENT glXMakeCurrent = cast<PFNGLXMAKECURRENT>(dlsym(libgl, "glXMakeCurrent"));

  typedef void (* PFNGLXDESTROYPIXMAP) (Display *, GLXPixmap);
  PFNGLXDESTROYPIXMAP glXDestroyPixmap = cast<PFNGLXDESTROYPIXMAP>(dlsym(libgl, "glXDestroyPixmap"));

  typedef void (* PFNGLXDESTROYCONTEXT) (Display*, GLXContext);
  PFNGLXDESTROYCONTEXT glXDestroyContext = cast<PFNGLXDESTROYCONTEXT>(dlsym(libgl, "glXDestroyContext"));

  typedef GLubyte* (* PFNGLGETSTRING) (GLenum);
  PFNGLGETSTRING glGetString = cast<PFNGLGETSTRING>(dlsym(libgl, "glGetString"));

  if (!glXQueryExtension ||
      !glXChooseFBConfig ||
      !glXGetVisualFromFBConfig ||
      !glXCreatePixmap ||
      !glXCreateNewContext ||
      !glXMakeCurrent ||
      !glXDestroyPixmap ||
      !glXDestroyContext ||
      !glGetString)
  {
    fatal_error("Unable to find required symbols in libGL.so.1");
  }
  
  Display *dpy = XOpenDisplay(NULL);
  if (!dpy)
    fatal_error("Unable to open a connection to the X server");
  
  
  if (!glXQueryExtension(dpy, NULL, NULL))
    fatal_error("GLX extension missing");

  XSetErrorHandler(x_error_handler);

  
  int attribs[] = {
    GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT,
    GLX_X_RENDERABLE, True,
    0
  };
  int numReturned;
  GLXFBConfig *fbConfigs = glXChooseFBConfig(dpy, DefaultScreen(dpy), attribs, &numReturned );
  if (!fbConfigs)
    fatal_error("No FBConfigs found");
  XVisualInfo *vInfo = glXGetVisualFromFBConfig(dpy, fbConfigs[0]);
  if (!vInfo)
    fatal_error("No visual found for first FBConfig");

  
  Pixmap pixmap = XCreatePixmap(dpy, RootWindow(dpy, vInfo->screen), 4, 4, 32);
  GLXPixmap glxpixmap = glXCreatePixmap(dpy, fbConfigs[0], pixmap, NULL);

  
  GLXContext context = glXCreateNewContext(dpy, fbConfigs[0], GLX_RGBA_TYPE, NULL, True);
  glXMakeCurrent(dpy, glxpixmap, context);

  
  enum { bufsize = 1024 };
  char buf[bufsize];
  const GLubyte *vendorString = glGetString(GL_VENDOR);
  const GLubyte *rendererString = glGetString(GL_RENDERER);
  const GLubyte *versionString = glGetString(GL_VERSION);
  
  if (!vendorString || !rendererString || !versionString)
    fatal_error("glGetString returned null");

  int length = snprintf(buf, bufsize,
                        "VENDOR\n%s\nRENDERER\n%s\nVERSION\n%s\n",
                        vendorString,
                        rendererString,
                        versionString);
  if (length >= bufsize)
    fatal_error("GL strings length too large for buffer size");

  
  
  
  
  XSync(dpy, False);
  
  
  write(write_end_of_the_pipe, buf, length);

  
  
  
  glXMakeCurrent(dpy, None, NULL); 
  glXDestroyContext(dpy, context);
  glXDestroyPixmap(dpy, glxpixmap);
  XFreePixmap(dpy, pixmap);
  XCloseDisplay(dpy);
  dlclose(libgl);
}


bool fire_glxtest_process()
{
  int pfd[2];
  if (pipe(pfd) == -1) {
      perror("pipe");
      return false;
  }
  pid_t pid = fork();
  if (pid < 0) {
      perror("fork");
      close(pfd[0]);
      close(pfd[1]);
      return false;
  }
  if (pid == 0) {
      close(pfd[0]);
      write_end_of_the_pipe = pfd[1];
      glxtest();
      close(pfd[1]);
      return true;
  }

  close(pfd[1]);
  mozilla::widget::glxtest_pipe = pfd[0];
  mozilla::widget::glxtest_pid = pid;
  return false;
}
