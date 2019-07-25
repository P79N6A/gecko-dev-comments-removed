



















#include <stdio.h>
#include <stdlib.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "esUtil.h"
#include "esUtil_win.h"






PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;

PFNEGLPOSTSUBBUFFERNVPROC eglPostSubBufferNV;

PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;

PFNGLDELETEFENCESNVPROC glDeleteFencesNV;
PFNGLGENFENCESNVPROC glGenFencesNV;
PFNGLGETFENCEIVNVPROC glGetFenceivNV;
PFNGLISFENCENVPROC glIsFenceNV;
PFNGLFINISHFENCENVPROC glFinishFenceNV;
PFNGLSETFENCENVPROC glSetFenceNV;
PFNGLTESTFENCENVPROC glTestFenceNV;






EGLBoolean CreateEGLContext ( EGLNativeWindowType hWnd, EGLDisplay* eglDisplay,
                              EGLContext* eglContext, EGLSurface* eglSurface,
                              EGLint* configAttribList, EGLint* surfaceAttribList)
{
   EGLint numConfigs;
   EGLint majorVersion;
   EGLint minorVersion;
   EGLDisplay display;
   EGLContext context;
   EGLSurface surface;
   EGLConfig config;
   EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };
   
   
   display = eglGetDisplay(GetDC(hWnd));
   if ( display == EGL_NO_DISPLAY )
   {
      return EGL_FALSE;
   }

   
   if ( !eglInitialize(display, &majorVersion, &minorVersion) )
   {
      return EGL_FALSE;
   }

   
   eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("eglCreateImageKHR");
   eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC) eglGetProcAddress("eglDestroyImageKHR");
   
   eglPostSubBufferNV = (PFNEGLPOSTSUBBUFFERNVPROC) eglGetProcAddress("eglPostSubBufferNV");

   glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) eglGetProcAddress("glEGLImageTargetTexture2DOES");
   
   glDeleteFencesNV = (PFNGLDELETEFENCESNVPROC) eglGetProcAddress("glDeleteFencesNV");
   glGenFencesNV = (PFNGLGENFENCESNVPROC) eglGetProcAddress("glGenFencesNV");
   glGetFenceivNV = (PFNGLGETFENCEIVNVPROC) eglGetProcAddress("glGetFenceivNV");
   glIsFenceNV = (PFNGLISFENCENVPROC) eglGetProcAddress("glIsFenceNV");
   glFinishFenceNV = (PFNGLFINISHFENCENVPROC) eglGetProcAddress("glFinishFenceNV");
   glSetFenceNV = (PFNGLSETFENCENVPROC) eglGetProcAddress("glSetFenceNV");
   glTestFenceNV = (PFNGLTESTFENCENVPROC) eglGetProcAddress("glTestFenceNV");

   
   if ( !eglGetConfigs(display, NULL, 0, &numConfigs) )
   {
      return EGL_FALSE;
   }

   
   if ( !eglChooseConfig(display, configAttribList, &config, 1, &numConfigs) )
   {
      return EGL_FALSE;
   }

   
   surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)hWnd, surfaceAttribList);
   if ( surface == EGL_NO_SURFACE )
   {
      return EGL_FALSE;
   }

   
   context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs );
   if ( context == EGL_NO_CONTEXT )
   {
      return EGL_FALSE;
   }   
   
   
   if ( !eglMakeCurrent(display, surface, surface, context) )
   {
      return EGL_FALSE;
   }
   
   *eglDisplay = display;
   *eglSurface = surface;
   *eglContext = context;
   return EGL_TRUE;
} 













void ESUTIL_API esInitContext ( ESContext *esContext )
{
   if ( esContext != NULL )
   {
      memset( esContext, 0, sizeof( ESContext) );
   }
}














GLboolean ESUTIL_API esCreateWindow ( ESContext *esContext, LPCTSTR title, GLint width, GLint height, GLuint flags )
{
   EGLint configAttribList[] =
   {
       EGL_RED_SIZE,       5,
       EGL_GREEN_SIZE,     6,
       EGL_BLUE_SIZE,      5,
       EGL_ALPHA_SIZE,     (flags & ES_WINDOW_ALPHA) ? 8 : EGL_DONT_CARE,
       EGL_DEPTH_SIZE,     (flags & ES_WINDOW_DEPTH) ? 8 : EGL_DONT_CARE,
       EGL_STENCIL_SIZE,   (flags & ES_WINDOW_STENCIL) ? 8 : EGL_DONT_CARE,
       EGL_SAMPLE_BUFFERS, (flags & ES_WINDOW_MULTISAMPLE) ? 1 : 0,
       EGL_NONE
   };
   EGLint surfaceAttribList[] =
   {
       EGL_POST_SUB_BUFFER_SUPPORTED_NV, flags & (ES_WINDOW_POST_SUB_BUFFER_SUPPORTED) ? EGL_TRUE : EGL_FALSE,
       EGL_NONE, EGL_NONE
   };
   
   if ( esContext == NULL )
   {
      return GL_FALSE;
   }

   esContext->width = width;
   esContext->height = height;

   if ( !WinCreate ( esContext, title) )
   {
      return GL_FALSE;
   }

  
   if ( !CreateEGLContext ( esContext->hWnd,
                            &esContext->eglDisplay,
                            &esContext->eglContext,
                            &esContext->eglSurface,
                            configAttribList,
                            surfaceAttribList ) )
   {
      return GL_FALSE;
   }
   

   return GL_TRUE;
}






void ESUTIL_API esMainLoop ( ESContext *esContext )
{
   WinLoop ( esContext );
}





void ESUTIL_API esRegisterDrawFunc ( ESContext *esContext, void (ESCALLBACK *drawFunc) (ESContext* ) )
{
   esContext->drawFunc = drawFunc;
}





void ESUTIL_API esRegisterUpdateFunc ( ESContext *esContext, void (ESCALLBACK *updateFunc) ( ESContext*, float ) )
{
   esContext->updateFunc = updateFunc;
}





void ESUTIL_API esRegisterKeyFunc ( ESContext *esContext,
                                    void (ESCALLBACK *keyFunc) (ESContext*, unsigned char, int, int ) )
{
   esContext->keyFunc = keyFunc;
}







void ESUTIL_API esLogMessage ( const char *formatStr, ... )
{
    va_list params;
    char buf[BUFSIZ];

    va_start ( params, formatStr );
    vsprintf_s ( buf, sizeof(buf),  formatStr, params );
    
    printf ( "%s", buf );
    
    va_end ( params );
}







char* ESUTIL_API esLoadTGA ( char *fileName, int *width, int *height )
{
   char *buffer;

   if ( WinTGALoad ( fileName, &buffer, width, height ) )
   {
      return buffer;
   }

   return NULL;
}
