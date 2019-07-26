





#ifndef GLBLITHELPER_H_
#define GLBLITHELPER_H_

#include "GLContextTypes.h"
#include "GLConsts.h"
#include "nsSize.h"
#include "mozilla/Attributes.h"
#include "mozilla/gfx/Point.h"

namespace mozilla {
namespace gl {

class GLContext;







GLuint CreateTextureForOffscreen(GLContext* aGL, const GLFormats& aFormats,
                                 const gfx::IntSize& aSize);











GLuint CreateTexture(GLContext* aGL, GLenum aInternalFormat, GLenum aFormat,
                     GLenum aType, const gfx::IntSize& aSize);






GLuint CreateRenderbuffer(GLContext* aGL, GLenum aFormat, GLsizei aSamples,
                          const gfx::IntSize& aSize);







void CreateRenderbuffersForOffscreen(GLContext* aGL, const GLFormats& aFormats,
                                     const gfx::IntSize& aSize, bool aMultisample,
                                     GLuint* aColorMSRB, GLuint* aDepthRB,
                                     GLuint* aStencilRB);



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

    bool UseTexQuadProgram(GLenum target, const gfx::IntSize& srcSize);
    bool InitTexQuadProgram(GLenum target = LOCAL_GL_TEXTURE_2D);
    void DeleteTexBlitProgram();

public:

    GLBlitHelper(GLContext* gl);
    ~GLBlitHelper();

    
    
    
    void BlitFramebufferToFramebuffer(GLuint srcFB, GLuint destFB,
                                      const gfx::IntSize& srcSize,
                                      const gfx::IntSize& destSize);
    void BlitFramebufferToFramebuffer(GLuint srcFB, GLuint destFB,
                                      const gfx::IntSize& srcSize,
                                      const gfx::IntSize& destSize,
                                      const GLFormats& srcFormats);
    void BlitTextureToFramebuffer(GLuint srcTex, GLuint destFB,
                                  const gfx::IntSize& srcSize,
                                  const gfx::IntSize& destSize,
                                  GLenum srcTarget = LOCAL_GL_TEXTURE_2D);
    void BlitFramebufferToTexture(GLuint srcFB, GLuint destTex,
                                  const gfx::IntSize& srcSize,
                                  const gfx::IntSize& destSize,
                                  GLenum destTarget = LOCAL_GL_TEXTURE_2D);
    void BlitTextureToTexture(GLuint srcTex, GLuint destTex,
                              const gfx::IntSize& srcSize,
                              const gfx::IntSize& destSize,
                              GLenum srcTarget = LOCAL_GL_TEXTURE_2D,
                              GLenum destTarget = LOCAL_GL_TEXTURE_2D);
};

}
}

#endif 
