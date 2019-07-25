















#ifndef ESUTIL_H
#define ESUTIL_H




#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#ifdef __cplusplus

extern "C" {
#endif

   



#define ESUTIL_API  __cdecl
#define ESCALLBACK  __cdecl



#define ES_WINDOW_RGB           0

#define ES_WINDOW_ALPHA         1 

#define ES_WINDOW_DEPTH         2 

#define ES_WINDOW_STENCIL       4

#define ES_WINDOW_MULTISAMPLE   8

#define ES_WINDOW_POST_SUB_BUFFER_SUPPORTED 16





typedef struct
{
    GLfloat   m[4][4];
} ESMatrix;

typedef struct
{
   
   void*       userData;

   
   GLint       width;

   
   GLint       height;

   
   EGLNativeWindowType  hWnd;

   
   EGLDisplay  eglDisplay;
      
   
   EGLContext  eglContext;

   
   EGLSurface  eglSurface;

   
   void (ESCALLBACK *drawFunc) ( void* );
   void (ESCALLBACK *keyFunc) ( void*, unsigned char, int, int );
   void (ESCALLBACK *updateFunc) ( void*, float deltaTime );
} ESContext;






extern PFNEGLPOSTSUBBUFFERNVPROC eglPostSubBufferNV;











void ESUTIL_API esInitContext ( ESContext *esContext );















GLboolean ESUTIL_API esCreateWindow ( ESContext *esContext, LPCTSTR title, GLint width, GLint height, GLuint flags );





void ESUTIL_API esMainLoop ( ESContext *esContext );






void ESUTIL_API esRegisterDrawFunc ( ESContext *esContext, void (ESCALLBACK *drawFunc) ( ESContext* ) );






void ESUTIL_API esRegisterUpdateFunc ( ESContext *esContext, void (ESCALLBACK *updateFunc) ( ESContext*, float ) );






void ESUTIL_API esRegisterKeyFunc ( ESContext *esContext, 
                                    void (ESCALLBACK *drawFunc) ( ESContext*, unsigned char, int, int ) );




void ESUTIL_API esLogMessage ( const char *formatStr, ... );








GLuint ESUTIL_API esLoadShader ( GLenum type, const char *shaderSrc );









GLuint ESUTIL_API esLoadProgram ( const char *vertShaderSrc, const char *fragShaderSrc );













int ESUTIL_API esGenSphere ( int numSlices, float radius, GLfloat **vertices, GLfloat **normals, 
                             GLfloat **texCoords, GLushort **indices );












int ESUTIL_API esGenCube ( float scale, GLfloat **vertices, GLfloat **normals, 
                           GLfloat **texCoords, GLushort **indices );








char* ESUTIL_API esLoadTGA ( char *fileName, int *width, int *height );







void ESUTIL_API esScale(ESMatrix *result, GLfloat sx, GLfloat sy, GLfloat sz);






void ESUTIL_API esTranslate(ESMatrix *result, GLfloat tx, GLfloat ty, GLfloat tz);







void ESUTIL_API esRotate(ESMatrix *result, GLfloat angle, GLfloat x, GLfloat y, GLfloat z);








void ESUTIL_API esFrustum(ESMatrix *result, float left, float right, float bottom, float top, float nearZ, float farZ);









void ESUTIL_API esPerspective(ESMatrix *result, float fovy, float aspect, float nearZ, float farZ);








void ESUTIL_API esOrtho(ESMatrix *result, float left, float right, float bottom, float top, float nearZ, float farZ);






void ESUTIL_API esMatrixMultiply(ESMatrix *result, ESMatrix *srcA, ESMatrix *srcB);





void ESUTIL_API esMatrixLoadIdentity(ESMatrix *result);

#ifdef __cplusplus
}
#endif

#endif
