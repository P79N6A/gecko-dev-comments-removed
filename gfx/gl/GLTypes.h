



#if !defined(GLTYPES_H_)
#define GLTYPES_H_

#if !defined(__gltypes_h_) && !defined(__gl_h_)
#define __gltypes_h_
#define __gl_h_

#include <stddef.h>
#include <stdint.h>

#ifndef GLAPIENTRY
# ifdef WIN32
#  define GLAPIENTRY APIENTRY
#  define GLAPI
# else
#  define GLAPIENTRY
#  define GLAPI
# endif
#endif

typedef unsigned int GLenum;
typedef unsigned int GLbitfield;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef char realGLboolean;
typedef signed char GLbyte;
typedef short GLshort;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef float GLfloat;
typedef float GLclampf;
#ifndef GLdouble_defined
typedef double GLdouble;
#endif
typedef double GLclampd;
typedef void GLvoid;

typedef char GLchar;
#ifndef __gl2_h_
typedef intptr_t GLsizeiptr;
typedef intptr_t GLintptr;
#endif

#endif 

#include <stdint.h>


typedef struct __GLsync* GLsync;
typedef int64_t GLint64;
typedef uint64_t GLuint64;


typedef void* GLeglImage;


typedef void (GLAPIENTRY *GLDEBUGPROC)(GLenum source,
                                       GLenum type,
                                       GLuint id,
                                       GLenum severity,
                                       GLsizei length,
                                       const GLchar* message,
                                       const GLvoid* userParam);


typedef void* EGLImage;
typedef int EGLint;
typedef unsigned int EGLBoolean;
typedef unsigned int EGLenum;
typedef void *EGLConfig;
typedef void *EGLContext;
typedef void *EGLDisplay;
typedef void *EGLSurface;
typedef void *EGLClientBuffer;
typedef void *EGLCastToRelevantPtr;
typedef void *EGLImage;
typedef void *EGLSync;
typedef uint64_t EGLTime;

#define EGL_NO_CONTEXT       ((EGLContext)0)
#define EGL_NO_DISPLAY       ((EGLDisplay)0)
#define EGL_NO_SURFACE       ((EGLSurface)0)
#define EGL_NO_CONFIG        ((EGLConfig)nullptr)
#define EGL_NO_SYNC          ((EGLSync)0)
#define EGL_NO_IMAGE         ((EGLImage)0)

#endif
