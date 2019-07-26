




#ifndef MOZILLA_GFX_COMPOSITOROGL_H
#define MOZILLA_GFX_COMPOSITOROGL_H

#include "./../mozilla-config.h"        
#include "GLContext.h"                  
#include "GLContextTypes.h"             
#include "GLDefs.h"                     
#include "GeckoProfilerFunc.h"          
#include "LayerManagerOGLProgram.h"     
#include "Units.h"                      
#include "gfxContext.h"                 
#include "gfxPoint.h"                   
#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/TimeStamp.h"          
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
#include "nsISupportsImpl.h"            
#include "nsSize.h"                     
#include "nsTArray.h"                   
#include "nsThreadUtils.h"              
#include "nsTraceRefcnt.h"              
#include "nsXULAppAPI.h"                
#include "nscore.h"                     
#include "VBOArena.h"                   

class gfx3DMatrix;
class nsIWidget;
struct gfxMatrix;

namespace mozilla {
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
struct FPSState;

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
    return TextureFactoryIdentifier(LAYERS_OPENGL,
                                    XRE_GetProcessType(),
                                    GetMaxTextureSize(),
                                    mFBOTextureTarget == LOCAL_GL_TEXTURE_2D,
                                    SupportsPartialTextureUpdate());
  }

  virtual TemporaryRef<CompositingRenderTarget> 
  CreateRenderTarget(const gfx::IntRect &aRect, SurfaceInitMode aInit) MOZ_OVERRIDE;

  virtual TemporaryRef<CompositingRenderTarget>
  CreateRenderTargetFromSource(const gfx::IntRect &aRect,
                               const CompositingRenderTarget *aSource) MOZ_OVERRIDE;

  virtual void SetRenderTarget(CompositingRenderTarget *aSurface) MOZ_OVERRIDE;
  virtual CompositingRenderTarget* GetCurrentRenderTarget() MOZ_OVERRIDE;

  virtual void DrawQuad(const gfx::Rect& aRect, const gfx::Rect& aClipRect,
                        const EffectChain &aEffectChain,
                        gfx::Float aOpacity, const gfx::Matrix4x4 &aTransform,
                        const gfx::Point& aOffset) MOZ_OVERRIDE;

  virtual void EndFrame() MOZ_OVERRIDE;
  virtual void EndFrameForExternalComposition(const gfxMatrix& aTransform) MOZ_OVERRIDE;
  virtual void AbortFrame() MOZ_OVERRIDE;

  virtual bool SupportsPartialTextureUpdate() MOZ_OVERRIDE
  {
    return mGLContext->CanUploadSubTextures();
  }

  virtual bool CanUseCanvasLayerForSize(const gfxIntSize &aSize) MOZ_OVERRIDE
  {
    if (!mGLContext)
      return false;
    int32_t maxSize = GetMaxTextureSize();
    return aSize <= gfxIntSize(maxSize, maxSize);
  }

  virtual int32_t GetMaxTextureSize() const MOZ_OVERRIDE
  {
    MOZ_ASSERT(mGLContext);
    GLint texSize = 0;
    mGLContext->fGetIntegerv(LOCAL_GL_MAX_TEXTURE_SIZE,
                             &texSize);
    MOZ_ASSERT(texSize != 0);
    return texSize;
  }

  



  virtual void SetDestinationSurfaceSize(const gfx::IntSize& aSize) MOZ_OVERRIDE;

  virtual void SetScreenRenderOffset(const ScreenPoint& aOffset) MOZ_OVERRIDE {
    mRenderOffset = aOffset;
  }

  virtual void MakeCurrent(MakeCurrentFlags aFlags = 0) MOZ_OVERRIDE {
    if (mDestroyed) {
      NS_WARNING("Call on destroyed layer manager");
      return;
    }
    mGLContext->MakeCurrent(aFlags & ForceMakeCurrent);
  }

  virtual void SetTargetContext(gfxContext* aTarget) MOZ_OVERRIDE
  {
    mTarget = aTarget;
  }

  virtual void PrepareViewport(const gfx::IntSize& aSize,
                               const gfxMatrix& aWorldTransform) MOZ_OVERRIDE;


#ifdef MOZ_DUMP_PAINTING
  virtual const char* Name() const MOZ_OVERRIDE { return "OGL"; }
#endif 

  virtual void NotifyLayersTransaction() MOZ_OVERRIDE;

  virtual void Pause() MOZ_OVERRIDE;
  virtual bool Resume() MOZ_OVERRIDE;

  virtual nsIWidget* GetWidget() const MOZ_OVERRIDE { return mWidget; }
  virtual const nsIntSize& GetWidgetSize() MOZ_OVERRIDE {
    return mWidgetSize;
  }

  GLContext* gl() const { return mGLContext; }
  ShaderProgramType GetFBOLayerProgramType() const {
    return mFBOTextureTarget == LOCAL_GL_TEXTURE_RECTANGLE_ARB ?
           RGBARectLayerProgramType : RGBALayerProgramType;
  }
  gfx::SurfaceFormat GetFBOFormat() const {
    return gfx::FORMAT_R8G8B8A8;
  }

  





  GLuint GetTemporaryTexture(GLenum aUnit);
private:
  


  nsRefPtr<gfxContext> mTarget;

  
  nsIWidget *mWidget;
  nsIntSize mWidgetSize;
  nsRefPtr<GLContext> mGLContext;

  
  nsIntSize mSurfaceSize;

  ScreenPoint mRenderOffset;

  
  class ReadDrawFPSPref MOZ_FINAL : public nsRunnable {
  public:
    NS_IMETHOD Run() MOZ_OVERRIDE;
  };

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

  


  virtual void BeginFrame(const gfx::Rect *aClipRectIn,
                          const gfxMatrix& aTransform,
                          const gfx::Rect& aRenderBounds, 
                          gfx::Rect *aClipRectOut = nullptr,
                          gfx::Rect *aRenderBoundsOut = nullptr) MOZ_OVERRIDE;

  ShaderProgramType GetProgramTypeForEffect(Effect* aEffect) const;

  


  void SetLayerProgramProjectionMatrix(const gfx3DMatrix& aMatrix);

  



  void AddPrograms(ShaderProgramType aType);

  ShaderProgramOGL* GetProgram(ShaderProgramType aType,
                               MaskType aMask = MaskNone) {
    MOZ_ASSERT(ProgramProfileOGL::ProgramExists(aType, aMask),
               "Invalid program type.");
    return mPrograms[aType].mVariations[aMask];
  }

  






  void CreateFBOWithTexture(const gfx::IntRect& aRect, SurfaceInitMode aInit,
                            GLuint aSourceFrameBuffer,
                            GLuint *aFBO, GLuint *aTexture);

  GLintptr QuadVBOVertexOffset() { return 0; }
  GLintptr QuadVBOTexCoordOffset() { return sizeof(float)*4*2; }
  GLintptr QuadVBOFlippedTexCoordOffset() { return sizeof(float)*8*2; }

  void BindQuadVBO() {
    mGLContext->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, mQuadVBO);
  }

  void QuadVBOVerticesAttrib(GLuint aAttribIndex) {
    mGLContext->fVertexAttribPointer(aAttribIndex, 2,
                                     LOCAL_GL_FLOAT, LOCAL_GL_FALSE, 0,
                                     (GLvoid*) QuadVBOVertexOffset());
  }

  void QuadVBOTexCoordsAttrib(GLuint aAttribIndex) {
    mGLContext->fVertexAttribPointer(aAttribIndex, 2,
                                     LOCAL_GL_FLOAT, LOCAL_GL_FALSE, 0,
                                     (GLvoid*) QuadVBOTexCoordOffset());
  }

  void QuadVBOFlippedTexCoordsAttrib(GLuint aAttribIndex) {
    mGLContext->fVertexAttribPointer(aAttribIndex, 2,
                                     LOCAL_GL_FLOAT, LOCAL_GL_FALSE, 0,
                                     (GLvoid*) QuadVBOFlippedTexCoordOffset());
  }

  void BindAndDrawQuad(GLuint aVertAttribIndex,
                       GLuint aTexCoordAttribIndex,
                       bool aFlipped = false)
  {
    BindQuadVBO();
    QuadVBOVerticesAttrib(aVertAttribIndex);

    if (aTexCoordAttribIndex != GLuint(-1)) {
      if (aFlipped)
        QuadVBOFlippedTexCoordsAttrib(aTexCoordAttribIndex);
      else
        QuadVBOTexCoordsAttrib(aTexCoordAttribIndex);

      mGLContext->fEnableVertexAttribArray(aTexCoordAttribIndex);
    }

    mGLContext->fEnableVertexAttribArray(aVertAttribIndex);
    mGLContext->fDrawArrays(LOCAL_GL_TRIANGLE_STRIP, 0, 4);
    mGLContext->fDisableVertexAttribArray(aVertAttribIndex);

    if (aTexCoordAttribIndex != GLuint(-1)) {
      mGLContext->fDisableVertexAttribArray(aTexCoordAttribIndex);
    }
  }

  void BindAndDrawQuad(ShaderProgramOGL *aProg,
                       bool aFlipped = false)
  {
    NS_ASSERTION(aProg->HasInitialized(), "Shader program not correctly initialized");
    BindAndDrawQuad(aProg->AttribLocation(ShaderProgramOGL::VertexCoordAttrib),
                    aProg->AttribLocation(ShaderProgramOGL::TexCoordAttrib),
                    aFlipped);
  }

  void BindAndDrawQuadWithTextureRect(ShaderProgramOGL *aProg,
                                      const gfx::Rect& aTexCoordRect,
                                      TextureSource *aTexture);

  void CleanupResources();

  



  void CopyToTarget(gfxContext *aTarget, const gfxMatrix& aWorldMatrix);

  


  double AddFrameAndGetFps(const TimeStamp& timestamp);

  bool mDestroyed;

  nsAutoPtr<FPSState> mFPS;
  
  
  nsTArray<GLuint> mTextures;
  static bool sDrawFPS;
};

}
}

#endif 
