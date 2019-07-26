





#ifndef GLBLITHELPER_H_
#define GLBLITHELPER_H_

#include "GLContextTypes.h"
#include "GLConsts.h"
#include "mozilla/Attributes.h"

struct nsIntSize;

namespace mozilla {
namespace gl {

class GLContext;

class GLBlitHelper MOZ_FINAL
{
    
    GLContext* mGL;

    GLuint mTexBlit_Buffer;
    GLuint mTexBlit_VertShader;
    GLuint mTex2DBlit_FragShader;
    GLuint mTex2DRectBlit_FragShader;
    GLuint mTex2DBlit_Program;
    GLuint mTex2DRectBlit_Program;

    void UseBlitProgram();
    void SetBlitFramebufferForDestTexture(GLuint aTexture);

    bool UseTexQuadProgram(GLenum target, const nsIntSize& srcSize);
    bool InitTexQuadProgram(GLenum target = LOCAL_GL_TEXTURE_2D);
    void DeleteTexBlitProgram();

public:

    GLBlitHelper(GLContext* gl);
    ~GLBlitHelper();

    
    
    
    void BlitFramebufferToFramebuffer(GLuint srcFB, GLuint destFB,
                                      const nsIntSize& srcSize,
                                      const nsIntSize& destSize);
    void BlitFramebufferToFramebuffer(GLuint srcFB, GLuint destFB,
                                      const nsIntSize& srcSize,
                                      const nsIntSize& destSize,
                                      const GLFormats& srcFormats);
    void BlitTextureToFramebuffer(GLuint srcTex, GLuint destFB,
                                  const nsIntSize& srcSize,
                                  const nsIntSize& destSize,
                                  GLenum srcTarget = LOCAL_GL_TEXTURE_2D);
    void BlitFramebufferToTexture(GLuint srcFB, GLuint destTex,
                                  const nsIntSize& srcSize,
                                  const nsIntSize& destSize,
                                  GLenum destTarget = LOCAL_GL_TEXTURE_2D);
    void BlitTextureToTexture(GLuint srcTex, GLuint destTex,
                              const nsIntSize& srcSize,
                              const nsIntSize& destSize,
                              GLenum srcTarget = LOCAL_GL_TEXTURE_2D,
                              GLenum destTarget = LOCAL_GL_TEXTURE_2D);
};

}
}

#endif 
