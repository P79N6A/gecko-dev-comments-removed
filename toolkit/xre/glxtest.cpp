




















#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <dlfcn.h>
#include "nscore.h"

#include <fcntl.h>

#ifdef __SUNPRO_CC
#include <stdio.h>
#endif

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
  
  
  
  int fd = open("/dev/null", O_WRONLY);
  for (int i = 1; i < fd; i++)
    dup2(fd, i);
  close(fd);

  if (getenv("MOZ_AVOID_OPENGL_ALTOGETHER"))
    fatal_error("The MOZ_AVOID_OPENGL_ALTOGETHER environment variable is defined");

  
#ifdef __OpenBSD__
  #define LIBGL_FILENAME "libGL.so"
#else
  #define LIBGL_FILENAME "libGL.so.1"
#endif
  void *libgl = dlopen(LIBGL_FILENAME, RTLD_LAZY);
  if (!libgl)
    fatal_error("Unable to load " LIBGL_FILENAME);
  
  typedef void* (* PFNGLXGETPROCADDRESS) (const char *);
  PFNGLXGETPROCADDRESS glXGetProcAddress = cast<PFNGLXGETPROCADDRESS>(dlsym(libgl, "glXGetProcAddress"));
  
  if (!glXGetProcAddress)
    fatal_error("Unable to find glXGetProcAddress in " LIBGL_FILENAME);

  typedef GLXFBConfig* (* PFNGLXQUERYEXTENSION) (Display *, int *, int *);
  PFNGLXQUERYEXTENSION glXQueryExtension = cast<PFNGLXQUERYEXTENSION>(glXGetProcAddress("glXQueryExtension"));

  typedef GLXFBConfig* (* PFNGLXQUERYVERSION) (Display *, int *, int *);
  PFNGLXQUERYVERSION glXQueryVersion = cast<PFNGLXQUERYVERSION>(dlsym(libgl, "glXQueryVersion"));

  typedef XVisualInfo* (* PFNGLXCHOOSEVISUAL) (Display *, int, int *);
  PFNGLXCHOOSEVISUAL glXChooseVisual = cast<PFNGLXCHOOSEVISUAL>(glXGetProcAddress("glXChooseVisual"));

  typedef GLXContext (* PFNGLXCREATECONTEXT) (Display *, XVisualInfo *, GLXContext, Bool);
  PFNGLXCREATECONTEXT glXCreateContext = cast<PFNGLXCREATECONTEXT>(glXGetProcAddress("glXCreateContext"));

  typedef Bool (* PFNGLXMAKECURRENT) (Display*, GLXDrawable, GLXContext);
  PFNGLXMAKECURRENT glXMakeCurrent = cast<PFNGLXMAKECURRENT>(glXGetProcAddress("glXMakeCurrent"));

  typedef void (* PFNGLXDESTROYCONTEXT) (Display*, GLXContext);
  PFNGLXDESTROYCONTEXT glXDestroyContext = cast<PFNGLXDESTROYCONTEXT>(glXGetProcAddress("glXDestroyContext"));

  typedef GLubyte* (* PFNGLGETSTRING) (GLenum);
  PFNGLGETSTRING glGetString = cast<PFNGLGETSTRING>(glXGetProcAddress("glGetString"));

  if (!glXQueryExtension ||
      !glXQueryVersion ||
      !glXChooseVisual ||
      !glXCreateContext ||
      !glXMakeCurrent ||
      !glXDestroyContext ||
      !glGetString)
  {
    fatal_error("glXGetProcAddress couldn't find required functions");
  }
  
  Display *dpy = XOpenDisplay(nullptr);
  if (!dpy)
    fatal_error("Unable to open a connection to the X server");
  
  
  if (!glXQueryExtension(dpy, nullptr, nullptr))
    fatal_error("GLX extension missing");

  XSetErrorHandler(x_error_handler);

  
   int attribs[] = {
      GLX_RGBA,
      GLX_RED_SIZE, 1,
      GLX_GREEN_SIZE, 1,
      GLX_BLUE_SIZE, 1,
      None };
  XVisualInfo *vInfo = glXChooseVisual(dpy, DefaultScreen(dpy), attribs);
  if (!vInfo)
    fatal_error("No visuals found");

  
  
  Window window;
  XSetWindowAttributes swa;
  swa.colormap = XCreateColormap(dpy, RootWindow(dpy, vInfo->screen),
                                 vInfo->visual, AllocNone);

  swa.border_pixel = 0;
  window = XCreateWindow(dpy, RootWindow(dpy, vInfo->screen),
                       0, 0, 16, 16,
                       0, vInfo->depth, InputOutput, vInfo->visual,
                       CWBorderPixel | CWColormap, &swa);

  
  GLXContext context = glXCreateContext(dpy, vInfo, nullptr, True);
  glXMakeCurrent(dpy, window, context);

  
  void* glXBindTexImageEXT = glXGetProcAddress("glXBindTexImageEXT"); 

  
  enum { bufsize = 1024 };
  char buf[bufsize];
  const GLubyte *vendorString = glGetString(GL_VENDOR);
  const GLubyte *rendererString = glGetString(GL_RENDERER);
  const GLubyte *versionString = glGetString(GL_VERSION);
  
  if (!vendorString || !rendererString || !versionString)
    fatal_error("glGetString returned null");

  int length = snprintf(buf, bufsize,
                        "VENDOR\n%s\nRENDERER\n%s\nVERSION\n%s\nTFP\n%s\n",
                        vendorString,
                        rendererString,
                        versionString,
                        glXBindTexImageEXT ? "TRUE" : "FALSE");
  if (length >= bufsize)
    fatal_error("GL strings length too large for buffer size");

  
  
  
  glXMakeCurrent(dpy, None, nullptr); 
  glXDestroyContext(dpy, context);
  XDestroyWindow(dpy, window);
  XFreeColormap(dpy, swa.colormap);
  XCloseDisplay(dpy);
  dlclose(libgl);

  
  write(write_end_of_the_pipe, buf, length);
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
