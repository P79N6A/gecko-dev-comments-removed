




































#ifndef GFX_GLWRAPPER_H
#define GFX_GLWRAPPER_H

#ifdef XP_WIN
#include <windows.h>
#endif
#include "prlink.h"

#include "glDefs.h"

struct OGLFunction;

class glWrapper
{
public:
  glWrapper();

  typedef void (GLAPIENTRY * PFNGLBLENDFUNCSEPARATEPROC) (GLenum,
                                                        GLenum,
                                                        GLenum,
                                                        GLenum);
  
  typedef void (GLAPIENTRY * PFNGLENABLEPROC) (GLenum);
  typedef void (GLAPIENTRY * PFNGLENABLECLIENTSTATEPROC) (GLenum);
  typedef void (GLAPIENTRY * PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint);
  typedef void (GLAPIENTRY * PFNGLDISABLEPROC) (GLenum);
  typedef void (GLAPIENTRY * PFNGLDISABLECLIENTSTATEPROC) (GLenum);
  typedef void (GLAPIENTRY * PFNGLDISABLEVERTEXATTRIBARRAYPROC) (GLuint);

  typedef void (GLAPIENTRY * PFNGLDRAWARRAYSPROC) (GLenum, GLint, GLsizei);

  typedef void (GLAPIENTRY * PFNGLFINISHPROC) (void);
  typedef void (GLAPIENTRY * PFNGLCLEARPROC) (GLbitfield);
  typedef void (GLAPIENTRY * PFNGLSCISSORPROC) (GLint, GLint, GLsizei, GLsizei);
  typedef void (GLAPIENTRY * PFNGLVIEWPORTPROC) (GLint, GLint, GLsizei, GLsizei);
  typedef void (GLAPIENTRY * PFNGLCLEARCOLORPROC) (GLclampf,
                                                 GLclampf,
                                                 GLclampf,
                                                 GLclampf);
  typedef void (GLAPIENTRY * PFNGLREADBUFFERPROC) (GLenum);
  typedef void (GLAPIENTRY * PFNGLREADPIXELSPROC) (GLint,
                                                 GLint,
                                                 GLsizei,
                                                 GLsizei,
                                                 GLenum,
                                                 GLenum,
                                                 GLvoid *);

  typedef void (GLAPIENTRY * PFNGLTEXENVFPROC) (GLenum, GLenum, GLfloat);
  typedef void (GLAPIENTRY * PFNGLTEXPARAMETERIPROC) (GLenum, GLenum, GLint);
  typedef void (GLAPIENTRY * PFNGLACTIVETEXTUREPROC) (GLenum);
  typedef void (GLAPIENTRY * PFNGLPIXELSTOREIPROC) (GLenum, GLint);

  typedef void (GLAPIENTRY * PFNGLGENTEXTURESPROC) (GLsizei, GLuint *);
  typedef void (GLAPIENTRY * PFNGLGENBUFFERSPROC) (GLsizei, GLuint *);
  typedef void (GLAPIENTRY * PFNGLGENFRAMEBUFFERSEXTPROC) (GLsizei, GLuint *);
  typedef void (GLAPIENTRY * PFNGLDELETETEXTURESPROC) (GLsizei, const GLuint *);
  typedef void (GLAPIENTRY * PFNGLDELETEFRAMEBUFFERSEXTPROC) (GLsizei,
                                                            const GLuint *);
  
  typedef void (GLAPIENTRY * PFNGLBINDTEXTUREPROC) (GLenum, GLuint);
  typedef void (GLAPIENTRY * PFNGLBINDBUFFERPROC) (GLenum, GLuint);
  typedef void (GLAPIENTRY * PFNGLBINDFRAMEBUFFEREXTPROC) (GLenum, GLuint);

  typedef void (GLAPIENTRY * PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) (GLenum,
                                                              GLenum,
                                                              GLenum,
                                                              GLuint,
                                                              GLint);
  typedef GLenum (GLAPIENTRY * PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) (GLenum);

  typedef void (GLAPIENTRY * PFNGLBUFFERDATAPROC) (GLenum,
                                                 GLsizeiptr,
                                                 const GLvoid *,
                                                 GLenum);

  typedef void (GLAPIENTRY * PFNGLVERTEXPOINTERPROC) (GLint,
                                                    GLenum,
                                                    GLsizei,
                                                    const GLvoid *);
  typedef void (GLAPIENTRY * PFNGLTEXCOORDPOINTERPROC) (GLint,
                                                      GLenum,
                                                      GLsizei,
                                                      const GLvoid *);

  typedef void (GLAPIENTRY * PFNGLTEXIMAGE2DPROC) (GLenum,
                                                 GLint,
                                                 GLint,
                                                 GLsizei,
                                                 GLsizei,
                                                 GLint,
                                                 GLenum,
                                                 GLenum,
                                                 const GLvoid *);
  typedef void (GLAPIENTRY * PFNGLTEXSUBIMAGE2DPROC) (GLenum,
                                                    GLint,
                                                    GLint,
                                                    GLint,
                                                    GLsizei,
                                                    GLsizei,
                                                    GLenum,
                                                    GLenum,
                                                    const GLvoid *);

  typedef GLuint (GLAPIENTRY * PFNGLCREATESHADERPROC) (GLenum);
  typedef GLuint (GLAPIENTRY * PFNGLCREATEPROGRAMPROC) (void);
  typedef void (GLAPIENTRY * PFNGLDELETEPROGRAMPROC) (GLuint);
  typedef void (GLAPIENTRY * PFNGLUSEPROGRAMPROC) (GLuint);
  typedef void (GLAPIENTRY * PFNGLSHADERSOURCEPROC) (GLuint,
                                                   GLsizei,
                                                   const GLchar **,
                                                   const GLint *);
  typedef void (GLAPIENTRY * PFNGLCOMPILESHADERPROC) (GLuint);
  typedef void (GLAPIENTRY * PFNGLATTACHSHADERPROC) (GLuint, GLuint);
  typedef void (GLAPIENTRY * PFNGLLINKPROGRAMPROC) (GLuint);
  typedef void (GLAPIENTRY * PFNGLGETPROGRAMIVPROC) (GLuint, GLenum, GLint *);
  typedef void (GLAPIENTRY * PFNGLGETSHADERIVPROC) (GLuint, GLenum, GLint *);

  typedef void (GLAPIENTRY * PFNGLBINDATTRIBLOCATIONPROC) (GLuint,
                                                         GLuint,
                                                         const GLchar *);
  typedef void (GLAPIENTRY * PFNGLVERTEXATTRIBPOINTERPROC) (GLuint,
                                                          GLint,
                                                          GLenum,
                                                          GLboolean,
                                                          GLsizei,
                                                          const GLvoid *);

  typedef void (GLAPIENTRY * PFNGLUNIFORM1IPROC) (GLint, GLint);
  typedef void (GLAPIENTRY * PFNGLUNIFORM1FPROC) (GLint, GLfloat);
  typedef void (GLAPIENTRY * PFNGLUNIFORM4FPROC) (GLint,
                                                GLfloat,
                                                GLfloat,
                                                GLfloat,
                                                GLfloat);
  typedef void (GLAPIENTRY * PFNGLUNIFORMMATRIX4FVPROC) (GLint,
                                                       GLsizei,
                                                       GLboolean,
                                                       const GLfloat *);
  typedef GLint (GLAPIENTRY * PFNGLGETUNIFORMLOCATIONPROC) (GLuint,
                                                          const GLchar *);

  typedef GLubyte* (GLAPIENTRY * PFNGLGETSTRINGPROC) (GLenum);

#ifdef XP_WIN
  typedef HGLRC (GLAPIENTRY * PFNWGLCREATECONTEXTPROC) (HDC);
  typedef BOOL (GLAPIENTRY * PFNWGLDELETECONTEXTPROC) (HGLRC);
  typedef BOOL (GLAPIENTRY * PFNWGLMAKECURRENTPROC) (HDC, HGLRC);
  typedef PROC (GLAPIENTRY * PFNWGLGETPROCADDRESSPROC) (LPCSTR);
#endif

  PFNGLBLENDFUNCSEPARATEPROC BlendFuncSeparate;
  
  PFNGLENABLEPROC Enable;
  PFNGLENABLECLIENTSTATEPROC EnableClientState;
  PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray;
  PFNGLDISABLEPROC Disable;
  PFNGLDISABLECLIENTSTATEPROC DisableClientState;
  PFNGLDISABLEVERTEXATTRIBARRAYPROC DisableVertexAttribArray;

  PFNGLDRAWARRAYSPROC DrawArrays;

  PFNGLFINISHPROC Finish;
  PFNGLCLEARPROC Clear;
  PFNGLSCISSORPROC Scissor;
  PFNGLVIEWPORTPROC Viewport;
  PFNGLCLEARCOLORPROC ClearColor;
  PFNGLREADBUFFERPROC ReadBuffer;
  PFNGLREADPIXELSPROC ReadPixels;

  PFNGLTEXENVFPROC TexEnvf;
  PFNGLTEXPARAMETERIPROC TexParameteri;
  PFNGLACTIVETEXTUREPROC ActiveTexture;
  PFNGLPIXELSTOREIPROC PixelStorei;

  PFNGLGENTEXTURESPROC GenTextures;
  PFNGLGENBUFFERSPROC GenBuffers;
  PFNGLGENFRAMEBUFFERSEXTPROC GenFramebuffersEXT;
  PFNGLDELETETEXTURESPROC DeleteTextures;
  PFNGLDELETEFRAMEBUFFERSEXTPROC DeleteFramebuffersEXT;

  PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC CheckFramebufferStatusEXT;
  
  PFNGLBINDTEXTUREPROC BindTexture;
  PFNGLBINDBUFFERPROC BindBuffer;
  PFNGLBINDFRAMEBUFFEREXTPROC BindFramebufferEXT;

  PFNGLFRAMEBUFFERTEXTURE2DEXTPROC FramebufferTexture2DEXT; 

  PFNGLBUFFERDATAPROC BufferData;

  PFNGLVERTEXPOINTERPROC VertexPointer;
  PFNGLTEXCOORDPOINTERPROC TexCoordPointer;

  PFNGLTEXIMAGE2DPROC TexImage2D;
  PFNGLTEXSUBIMAGE2DPROC TexSubImage2D;

  PFNGLCREATESHADERPROC CreateShader;
  PFNGLCREATEPROGRAMPROC CreateProgram;
  PFNGLDELETEPROGRAMPROC DeleteProgram;
  PFNGLUSEPROGRAMPROC UseProgram;
  PFNGLSHADERSOURCEPROC ShaderSource;
  PFNGLCOMPILESHADERPROC CompileShader;
  PFNGLATTACHSHADERPROC AttachShader;
  PFNGLLINKPROGRAMPROC LinkProgram;
  PFNGLGETPROGRAMIVPROC GetProgramiv;
  PFNGLGETSHADERIVPROC GetShaderiv;

  PFNGLBINDATTRIBLOCATIONPROC BindAttribLocation;
  PFNGLVERTEXATTRIBPOINTERPROC VertexAttribPointer;

  PFNGLUNIFORM1IPROC Uniform1i;
  PFNGLUNIFORM1FPROC Uniform1f;
  PFNGLUNIFORM4FPROC Uniform4f;
  PFNGLUNIFORMMATRIX4FVPROC UniformMatrix4fv;
  PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation;

  PFNGLGETSTRINGPROC GetString;

#ifdef XP_WIN
  



  HGLRC wCreateContext(HDC aDC);
  PFNWGLCREATECONTEXTPROC wCreateContextInternal;
  PFNWGLDELETECONTEXTPROC wDeleteContext;
  PFNWGLMAKECURRENTPROC wMakeCurrent;
  PFNWGLGETPROCADDRESSPROC wGetProcAddress;
#endif

private:
  PRBool EnsureInitialized();

  PRBool LoadSymbols(OGLFunction *functions);

  PRBool mIsInitialized;
  PRLibrary *mOGLLibrary;
};

extern glWrapper sglWrapper;

#endif
