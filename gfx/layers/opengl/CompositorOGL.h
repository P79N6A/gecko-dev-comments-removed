




#ifndef MOZILLA_GFX_COMPOSITOROGL_H
#define MOZILLA_GFX_COMPOSITOROGL_H

#include "GLContextTypes.h"             
#include "GLDefs.h"                     
#include "OGLShaderProgram.h"           
#include "Units.h"                      
#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/2D.h"             
#include "mozilla/gfx/BaseSize.h"       
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/LayersTypes.h"
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsSize.h"                     
#include "nsTArray.h"                   
#include "nsThreadUtils.h"              
#include "nsTraceRefcnt.h"              
#include "nsXULAppAPI.h"                
#include "nscore.h"                     
#include "VBOArena.h"                   

class gfx3DMatrix;
class nsIWidget;

namespace mozilla {
class TimeStamp;

namespace gfx {
class Matrix4x4;
}

namespace layers {

class CompositingRenderTarget;
class CompositingRenderTargetOGL;
class DataTextureSource;
class GLManagerCompositor;
class TextureSource;
struct Effect;
struct EffectChain;

class CompositorOGL : public Compositor
{
  typedef mozilla::gl::GLContext GLContext;
  typedef ShaderProgramType ProgramType;
  
  friend class GLManagerCompositor;

public:
  CompositorOGL(nsIWidget *aWidget, int aSurfaceWidth = -1, int aSurfaceHeight = -1,
                bool aUseExternalSurfaceSize = false);

  virtual ~CompositorOGL();

  virtual TemporaryRef<DataTextureSource>
  CreateDataTextureSource(TextureFlags aFlags = 0) MOZ_OVERRIDE;

  virtual bool Initialize() MOZ_OVERRIDE;

  virtual void Destroy() MOZ_OVERRIDE;

  virtual TextureFactoryIdentifier GetTextureFactoryIdentifier() MOZ_OVERRIDE
  {
    return TextureFactoryIdentifier(LayersBackend::LAYERS_OPENGL,
                                    XRE_GetProcessType(),
                                    GetMaxTextureSize(),
                                    mFBOTextureTarget == LOCAL_GL_TEXTURE_2D,
                                    SupportsPartialTextureUpdate());
  }

  virtual TemporaryRef<CompositingRenderTarget> 
  CreateRenderTarget(const gfx::IntRect &aRect, SurfaceInitMode aInit) MOZ_OVERRIDE;

  virtual TemporaryRef<CompositingRenderTarget>
  CreateRenderTargetFromSource(const gfx::IntRect &aRect,
                               const CompositingRenderTarget *aSource,
                               const gfx::IntPoint &aSourcePoint) MOZ_OVERRIDE;

  virtual void SetRenderTarget(CompositingRenderTarget *aSurface) MOZ_OVERRIDE;
  virtual CompositingRenderTarget* GetCurrentRenderTarget() MOZ_OVERRIDE;

  virtual void DrawQuad(const gfx::Rect& aRect,
                        const gfx::Rect& aClipRect,
                        const EffectChain &aEffectChain,
                        gfx::Float aOpacity,
                        const gfx::Matrix4x4 &aTransform) MOZ_OVERRIDE
  {
    DrawQuadInternal(aRect, aClipRect, aEffectChain,
                     aOpacity, aTransform, LOCAL_GL_TRIANGLE_STRIP);
  }

  virtual void DrawLines(const std::vector<gfx::Point>& aLines,
                         const gfx::Rect& aClipRect,
                         const gfx::Color& aColor,
                         gfx::Float aOpacity,
                         const gfx::Matrix4x4 &aTransform) MOZ_OVERRIDE;


  virtual void EndFrame() MOZ_OVERRIDE;
  virtual void EndFrameForExternalComposition(const gfx::Matrix& aTransform) MOZ_OVERRIDE;
  virtual void AbortFrame() MOZ_OVERRIDE;

  virtual bool SupportsPartialTextureUpdate() MOZ_OVERRIDE;

  virtual bool CanUseCanvasLayerForSize(const gfx::IntSize &aSize) MOZ_OVERRIDE
  {
    if (!mGLContext)
      return false;
    int32_t maxSize = GetMaxTextureSize();
    return aSize <= gfx::IntSize(maxSize, maxSize);
  }

  virtual int32_t GetMaxTextureSize() const MOZ_OVERRIDE;

  



  virtual void SetDestinationSurfaceSize(const gfx::IntSize& aSize) MOZ_OVERRIDE;

  virtual void SetScreenRenderOffset(const ScreenPoint& aOffset) MOZ_OVERRIDE {
    mRenderOffset = aOffset;
  }

  virtual void MakeCurrent(MakeCurrentFlags aFlags = 0) MOZ_OVERRIDE;

  virtual void SetTargetContext(gfx::DrawTarget* aTarget) MOZ_OVERRIDE
  {
    mTarget = aTarget;
  }

  virtual void PrepareViewport(const gfx::IntSize& aSize,
                               const gfx::Matrix& aWorldTransform) MOZ_OVERRIDE;


#ifdef MOZ_DUMP_PAINTING
  virtual const char* Name() const MOZ_OVERRIDE { return "OGL"; }
#endif 

  virtual void Pause() MOZ_OVERRIDE;
  virtual bool Resume() MOZ_OVERRIDE;

  virtual nsIWidget* GetWidget() const MOZ_OVERRIDE { return mWidget; }

  GLContext* gl() const { return mGLContext; }
  ShaderProgramType GetFBOLayerProgramType() const {
    return mFBOTextureTarget == LOCAL_GL_TEXTURE_RECTANGLE_ARB ?
           RGBARectLayerProgramType : RGBALayerProgramType;
  }
  gfx::SurfaceFormat GetFBOFormat() const {
    return gfx::SurfaceFormat::R8G8B8A8;
  }

  





  GLuint GetTemporaryTexture(GLenum aUnit);
private:
  virtual void DrawQuadInternal(const gfx::Rect& aRect,
                                const gfx::Rect& aClipRect,
                                const EffectChain &aEffectChain,
                                gfx::Float aOpacity,
                                const gfx::Matrix4x4 &aTransformi,
                                GLuint aDrawMode);

  


  RefPtr<gfx::DrawTarget> mTarget;

  
  nsIWidget *mWidget;
  nsIntSize mWidgetSize;
  nsRefPtr<GLContext> mGLContext;

  
  nsIntSize mSurfaceSize;

  ScreenPoint mRenderOffset;

  already_AddRefed<mozilla::gl::GLContext> CreateContext();

  
  struct ShaderProgramVariations {
    nsAutoTArray<nsAutoPtr<ShaderProgramOGL>, NumMaskTypes> mVariations;
    ShaderProgramVariations() {
      MOZ_COUNT_CTOR(ShaderProgramVariations);
      mVariations.SetLength(NumMaskTypes);
    }
    ~ShaderProgramVariations() {
      MOZ_COUNT_DTOR(ShaderProgramVariations);
    }
  };
  nsTArray<ShaderProgramVariations> mPrograms;

  
  GLenum mFBOTextureTarget;

  
  RefPtr<CompositingRenderTargetOGL> mCurrentRenderTarget;
#ifdef DEBUG
  CompositingRenderTargetOGL* mWindowRenderTarget;
#endif

  


  GLuint mQuadVBO;

  


  gl::VBOArena mVBOs;

  bool mHasBGRA;

  




  bool mUseExternalSurfaceSize;

  


  bool mFrameInProgress;

  


  virtual void clearFBRect(const gfx::Rect* aRect);

  


  virtual void BeginFrame(const nsIntRegion& aInvalidRegion,
                          const gfx::Rect *aClipRectIn,
                          const gfx::Matrix& aTransform,
                          const gfx::Rect& aRenderBounds,
                          gfx::Rect *aClipRectOut = nullptr,
                          gfx::Rect *aRenderBoundsOut = nullptr) MOZ_OVERRIDE;

  ShaderProgramType GetProgramTypeForEffect(Effect* aEffect) const;

  


  void SetLayerProgramProjectionMatrix(const gfx::Matrix4x4& aMatrix);

  



  void AddPrograms(ShaderProgramType aType);

  ShaderProgramOGL* GetProgram(ShaderProgramType aType,
                               MaskType aMask = MaskNone) {
    MOZ_ASSERT(ProgramProfileOGL::ProgramExists(aType, aMask),
               "Invalid program type.");
    return mPrograms[aType].mVariations[aMask];
  }

  






  void CreateFBOWithTexture(const gfx::IntRect& aRect, bool aCopyFromSource,
                            GLuint aSourceFrameBuffer,
                            GLuint *aFBO, GLuint *aTexture);

  GLintptr QuadVBOVertexOffset() { return 0; }
  GLintptr QuadVBOTexCoordOffset() { return sizeof(float)*4*2; }
  GLintptr QuadVBOFlippedTexCoordOffset() { return sizeof(float)*8*2; }

  void BindQuadVBO();
  void QuadVBOVerticesAttrib(GLuint aAttribIndex);
  void QuadVBOTexCoordsAttrib(GLuint aAttribIndex);
  void QuadVBOFlippedTexCoordsAttrib(GLuint aAttribIndex);
  void BindAndDrawQuad(GLuint aVertAttribIndex,
                       GLuint aTexCoordAttribIndex,
                       bool aFlipped = false,
                       GLuint aDrawMode = LOCAL_GL_TRIANGLE_STRIP);
  void BindAndDrawQuad(ShaderProgramOGL *aProg,
                       bool aFlipped = false,
                       GLuint aDrawMode = LOCAL_GL_TRIANGLE_STRIP);
  void BindAndDrawQuadWithTextureRect(ShaderProgramOGL *aProg,
                                      const gfx3DMatrix& aTextureTransform,
                                      const gfx::Rect& aTexCoordRect,
                                      TextureSource *aTexture);

  void CleanupResources();

  



  void CopyToTarget(gfx::DrawTarget* aTarget, const gfx::Matrix& aWorldMatrix);

  








  GLint FlipY(GLint y) const { return mHeight - y; }

  bool mDestroyed;

  
  
  nsTArray<GLuint> mTextures;
  static bool sDrawFPS;

  



  GLint mHeight;
};

}
}

#endif 
