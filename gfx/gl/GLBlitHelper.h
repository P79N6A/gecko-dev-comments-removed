





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
class SurfaceTextureImage;
class EGLImageImage;
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



class GLBlitHelper final
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
        ConvertSurfaceTexture,
        ConvertEGLImage
    };
    
    GLContext* mGL;

    GLuint mTexBlit_Buffer;
    GLuint mTexBlit_VertShader;
    GLuint mTex2DBlit_FragShader;
    GLuint mTex2DRectBlit_FragShader;
    GLuint mTex2DBlit_Program;
    GLuint mTex2DRectBlit_Program;

    GLint mYFlipLoc;

    GLint mTextureTransformLoc;

    
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
    void BindAndUploadEGLImage(EGLImage image, GLuint target);

#ifdef MOZ_WIDGET_GONK
    bool BlitGrallocImage(layers::GrallocImage* grallocImage);
#endif
    bool BlitPlanarYCbCrImage(layers::PlanarYCbCrImage* yuvImage);
#ifdef MOZ_WIDGET_ANDROID
    
    bool BlitSurfaceTextureImage(layers::SurfaceTextureImage* stImage);
    bool BlitEGLImageImage(layers::EGLImageImage* eglImage);
#endif

    explicit GLBlitHelper(GLContext* gl);

    friend class GLContext;

public:
    ~GLBlitHelper();

    
    
    
    void BlitFramebufferToFramebuffer(GLuint srcFB, GLuint destFB,
                                      const gfx::IntSize& srcSize,
                                      const gfx::IntSize& destSize,
                                      bool internalFBs = false);
    void BlitFramebufferToFramebuffer(GLuint srcFB, GLuint destFB,
                                      const gfx::IntSize& srcSize,
                                      const gfx::IntSize& destSize,
                                      const GLFormats& srcFormats,
                                      bool internalFBs = false);
    void BlitTextureToFramebuffer(GLuint srcTex, GLuint destFB,
                                  const gfx::IntSize& srcSize,
                                  const gfx::IntSize& destSize,
                                  GLenum srcTarget = LOCAL_GL_TEXTURE_2D,
                                  bool internalFBs = false);
    void BlitFramebufferToTexture(GLuint srcFB, GLuint destTex,
                                  const gfx::IntSize& srcSize,
                                  const gfx::IntSize& destSize,
                                  GLenum destTarget = LOCAL_GL_TEXTURE_2D,
                                  bool internalFBs = false);
    void BlitTextureToTexture(GLuint srcTex, GLuint destTex,
                              const gfx::IntSize& srcSize,
                              const gfx::IntSize& destSize,
                              GLenum srcTarget = LOCAL_GL_TEXTURE_2D,
                              GLenum destTarget = LOCAL_GL_TEXTURE_2D);
    bool BlitImageToFramebuffer(layers::Image* srcImage, const gfx::IntSize& destSize,
                                GLuint destFB, OriginPos destOrigin);
    bool BlitImageToTexture(layers::Image* srcImage, const gfx::IntSize& destSize,
                            GLuint destTex, GLenum destTarget, OriginPos destOrigin);
};

} 
} 

#endif 
