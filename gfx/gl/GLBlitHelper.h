





#ifndef GLBLITHELPER_H_
#define GLBLITHELPER_H_

#include "GLContextTypes.h"
#include "GLConsts.h"
#include "nsSize.h"
#include "mozilla/Attributes.h"
#include "mozilla/gfx/Point.h"

namespace mozilla {

namespace layers {
class Image;
class PlanarYCbCrImage;
class GrallocImage;
}

namespace gl {

class GLContext;







GLuint CreateTextureForOffscreen(GLContext* aGL, const GLFormats& aFormats,
                                 const gfx::IntSize& aSize);











GLuint CreateTexture(GLContext* aGL, GLenum aInternalFormat, GLenum aFormat,
                     GLenum aType, const gfx::IntSize& aSize, bool linear = true);






GLuint CreateRenderbuffer(GLContext* aGL, GLenum aFormat, GLsizei aSamples,
                          const gfx::IntSize& aSize);







void CreateRenderbuffersForOffscreen(GLContext* aGL, const GLFormats& aFormats,
                                     const gfx::IntSize& aSize, bool aMultisample,
                                     GLuint* aColorMSRB, GLuint* aDepthRB,
                                     GLuint* aStencilRB);



class GLBlitHelper MOZ_FINAL
{
    enum Channel
    {
        Channel_Y = 0,
        Channel_Cb,
        Channel_Cr,
        Channel_Max,
    };

    












    enum BlitType
    {
        BlitTex2D,
        BlitTexRect,
        ConvertGralloc,
        ConvertPlanarYCbCr,
    };
    
    GLContext* mGL;

    GLuint mTexBlit_Buffer;
    GLuint mTexBlit_VertShader;
    GLuint mTex2DBlit_FragShader;
    GLuint mTex2DRectBlit_FragShader;
    GLuint mTex2DBlit_Program;
    GLuint mTex2DRectBlit_Program;

    GLint mYFlipLoc;

    
    GLuint mTexExternalBlit_FragShader;
    GLuint mTexYUVPlanarBlit_FragShader;
    GLuint mTexExternalBlit_Program;
    GLuint mTexYUVPlanarBlit_Program;
    GLuint mFBO;
    GLuint mSrcTexY;
    GLuint mSrcTexCb;
    GLuint mSrcTexCr;
    GLuint mSrcTexEGL;
    GLint mYTexScaleLoc;
    GLint mCbCrTexScaleLoc;
    int mTexWidth;
    int mTexHeight;

    
    float mCurYScale;
    float mCurCbCrScale;

    void UseBlitProgram();
    void SetBlitFramebufferForDestTexture(GLuint aTexture);

    bool UseTexQuadProgram(BlitType target, const gfx::IntSize& srcSize);
    bool InitTexQuadProgram(BlitType target = BlitTex2D);
    void DeleteTexBlitProgram();
    void BindAndUploadYUVTexture(Channel which, uint32_t width, uint32_t height, void* data, bool allocation);

#ifdef MOZ_WIDGET_GONK
    void BindAndUploadExternalTexture(EGLImage image);
    bool BlitGrallocImage(layers::GrallocImage* grallocImage, bool yFlip = false);
#endif
    bool BlitPlanarYCbCrImage(layers::PlanarYCbCrImage* yuvImage, bool yFlip = false);

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
    bool BlitImageToTexture(layers::Image* srcImage, const gfx::IntSize& destSize,
                            GLuint destTex, GLenum destTarget, bool yFlip = false, GLuint xoffset = 0,
                            GLuint yoffset = 0, GLuint width = 0, GLuint height = 0);
};

}
}

#endif 
