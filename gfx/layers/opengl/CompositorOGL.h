




#ifndef MOZILLA_GFX_COMPOSITOROGL_H
#define MOZILLA_GFX_COMPOSITOROGL_H

#include "ContextStateTracker.h"
#include "gfx2DGlue.h"
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
#include "nsISupportsImpl.h"            
#include "nsSize.h"                     
#include "nsTArray.h"                   
#include "nsThreadUtils.h"              
#include "nsXULAppAPI.h"                
#include "nscore.h"                     
#ifdef MOZ_WIDGET_GONK
#include <ui/GraphicBuffer.h>
#endif
#include "gfxVR.h"

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
class GLBlitTextureImageHelper;









class CompositorTexturePoolOGL
{
protected:
  virtual ~CompositorTexturePoolOGL() {}

public:
  NS_INLINE_DECL_REFCOUNTING(CompositorTexturePoolOGL)

  virtual void Clear() = 0;

  virtual GLuint GetTexture(GLenum aTarget, GLenum aEnum) = 0;

  virtual void EndFrame() = 0;
};





class PerUnitTexturePoolOGL : public CompositorTexturePoolOGL
{
public:
  explicit PerUnitTexturePoolOGL(gl::GLContext* aGL)
  : mTextureTarget(0) 
  , mGL(aGL)
  {}

  virtual ~PerUnitTexturePoolOGL()
  {
    DestroyTextures();
  }

  virtual void Clear() override
  {
    DestroyTextures();
  }

  virtual GLuint GetTexture(GLenum aTarget, GLenum aUnit) override;

  virtual void EndFrame() override {}

protected:
  void DestroyTextures();

  GLenum mTextureTarget;
  nsTArray<GLuint> mTextures;
  RefPtr<gl::GLContext> mGL;
};










class PerFrameTexturePoolOGL : public CompositorTexturePoolOGL
{
public:
  explicit PerFrameTexturePoolOGL(gl::GLContext* aGL)
  : mTextureTarget(0) 
  , mGL(aGL)
  {}

  virtual ~PerFrameTexturePoolOGL()
  {
    DestroyTextures();
  }

  virtual void Clear() override
  {
    DestroyTextures();
  }

  virtual GLuint GetTexture(GLenum aTarget, GLenum aUnit) override;

  virtual void EndFrame() override;

protected:
  void DestroyTextures();

  GLenum mTextureTarget;
  RefPtr<gl::GLContext> mGL;
  nsTArray<GLuint> mCreatedTextures;
  nsTArray<GLuint> mUnusedTextures;
};

struct CompositorOGLVRObjects {
  bool mInitialized;

  gfx::VRHMDConfiguration mConfiguration;

  GLuint mDistortionVertices[2];
  GLuint mDistortionIndices[2];
  GLuint mDistortionIndexCount[2];

  GLint mAPosition;
  GLint mATexCoord0;
  GLint mATexCoord1;
  GLint mATexCoord2;
  GLint mAGenericAttribs;

  
  
  

  
  GLuint mDistortionProgram[2];
  GLint mUTexture[2];
  GLint mUVREyeToSource[2];
  GLint mUVRDestionatinScaleAndOffset[2];
  GLint mUHeight[2];
};



class CompositorOGL final : public Compositor
{
  typedef mozilla::gl::GLContext GLContext;

  friend class GLManagerCompositor;

  std::map<ShaderConfigOGL, ShaderProgramOGL*> mPrograms;
public:
  explicit CompositorOGL(nsIWidget *aWidget, int aSurfaceWidth = -1, int aSurfaceHeight = -1,
                         bool aUseExternalSurfaceSize = false);

protected:
  virtual ~CompositorOGL();

public:
  virtual TemporaryRef<DataTextureSource>
  CreateDataTextureSource(TextureFlags aFlags = TextureFlags::NO_FLAGS) override;

  virtual bool Initialize() override;

  virtual void Destroy() override;

  virtual TextureFactoryIdentifier GetTextureFactoryIdentifier() override
  {
    TextureFactoryIdentifier result =
      TextureFactoryIdentifier(LayersBackend::LAYERS_OPENGL,
                               XRE_GetProcessType(),
                               GetMaxTextureSize(),
                               mFBOTextureTarget == LOCAL_GL_TEXTURE_2D,
                               SupportsPartialTextureUpdate());
    result.mSupportedBlendModes += gfx::CompositionOp::OP_SCREEN;
    result.mSupportedBlendModes += gfx::CompositionOp::OP_MULTIPLY;
    result.mSupportedBlendModes += gfx::CompositionOp::OP_SOURCE;
    return result;
  }

  virtual TemporaryRef<CompositingRenderTarget>
  CreateRenderTarget(const gfx::IntRect &aRect, SurfaceInitMode aInit) override;

  virtual TemporaryRef<CompositingRenderTarget>
  CreateRenderTargetFromSource(const gfx::IntRect &aRect,
                               const CompositingRenderTarget *aSource,
                               const gfx::IntPoint &aSourcePoint) override;

  virtual void SetRenderTarget(CompositingRenderTarget *aSurface) override;
  virtual CompositingRenderTarget* GetCurrentRenderTarget() const override;

  virtual void DrawQuad(const gfx::Rect& aRect,
                        const gfx::Rect& aClipRect,
                        const EffectChain &aEffectChain,
                        gfx::Float aOpacity,
                        const gfx::Matrix4x4 &aTransform) override;

  virtual void EndFrame() override;
  virtual void SetFBAcquireFence(Layer* aLayer) override;
  virtual FenceHandle GetReleaseFence() override;
  virtual void EndFrameForExternalComposition(const gfx::Matrix& aTransform) override;

  virtual bool SupportsPartialTextureUpdate() override;

  virtual bool CanUseCanvasLayerForSize(const gfx::IntSize &aSize) override
  {
    if (!mGLContext)
      return false;
    int32_t maxSize = GetMaxTextureSize();
    return aSize <= gfx::IntSize(maxSize, maxSize);
  }

  virtual int32_t GetMaxTextureSize() const override;

  



  virtual void SetDestinationSurfaceSize(const gfx::IntSize& aSize) override;

  virtual void SetScreenRenderOffset(const ScreenPoint& aOffset) override {
    mRenderOffset = aOffset;
  }

  virtual void MakeCurrent(MakeCurrentFlags aFlags = 0) override;

  virtual void PrepareViewport(const gfx::IntSize& aSize) override;


#ifdef MOZ_DUMP_PAINTING
  virtual const char* Name() const override { return "OGL"; }
#endif 

  virtual LayersBackend GetBackendType() const override {
    return LayersBackend::LAYERS_OPENGL;
  }

  virtual void Pause() override;
  virtual bool Resume() override;

  virtual nsIWidget* GetWidget() const override { return mWidget; }

  GLContext* gl() const { return mGLContext; }
  


  void ResetProgram();

  gfx::SurfaceFormat GetFBOFormat() const {
    return gfx::SurfaceFormat::R8G8B8A8;
  }

  GLBlitTextureImageHelper* BlitTextureImageHelper();

  





  GLuint GetTemporaryTexture(GLenum aTarget, GLenum aUnit);

  const gfx::Matrix4x4& GetProjMatrix() const {
    return mProjMatrix;
  }
private:
  virtual gfx::IntSize GetWidgetSize() const override
  {
    return mWidgetSize;
  }

  bool InitializeVR();
  void DestroyVR(GLContext *gl);

  void DrawVRDistortion(const gfx::Rect& aRect,
                        const gfx::Rect& aClipRect,
                        const EffectChain& aEffectChain,
                        gfx::Float aOpacity,
                        const gfx::Matrix4x4& aTransform);

  
  nsIWidget *mWidget;
  gfx::IntSize mWidgetSize;
  nsRefPtr<GLContext> mGLContext;
  UniquePtr<GLBlitTextureImageHelper> mBlitTextureImageHelper;
  gfx::Matrix4x4 mProjMatrix;

  
  nsIntSize mSurfaceSize;

  ScreenPoint mRenderOffset;

  already_AddRefed<mozilla::gl::GLContext> CreateContext();

  
  GLenum mFBOTextureTarget;

  
  RefPtr<CompositingRenderTargetOGL> mCurrentRenderTarget;
#ifdef DEBUG
  CompositingRenderTargetOGL* mWindowRenderTarget;
#endif

  



  GLuint mQuadVBO;

  bool mHasBGRA;

  




  bool mUseExternalSurfaceSize;

  


  bool mFrameInProgress;

  


  virtual void ClearRect(const gfx::Rect& aRect) override;

  


  virtual void BeginFrame(const nsIntRegion& aInvalidRegion,
                          const gfx::Rect *aClipRectIn,
                          const gfx::Rect& aRenderBounds,
                          gfx::Rect *aClipRectOut = nullptr,
                          gfx::Rect *aRenderBoundsOut = nullptr) override;

  ShaderConfigOGL GetShaderConfigFor(Effect *aEffect,
                                     MaskType aMask = MaskType::MaskNone,
                                     gfx::CompositionOp aOp = gfx::CompositionOp::OP_OVER,
                                     bool aColorMatrix = false) const;
  ShaderProgramOGL* GetShaderProgramFor(const ShaderConfigOGL &aConfig);

  






  void CreateFBOWithTexture(const gfx::IntRect& aRect, bool aCopyFromSource,
                            GLuint aSourceFrameBuffer,
                            GLuint *aFBO, GLuint *aTexture);

  void BindAndDrawQuads(ShaderProgramOGL *aProg,
                        int aQuads,
                        const gfx::Rect* aLayerRect,
                        const gfx::Rect* aTextureRect);
  void BindAndDrawQuad(ShaderProgramOGL *aProg,
                       const gfx::Rect& aLayerRect,
                       const gfx::Rect& aTextureRect = gfx::Rect(0.0f, 0.0f, 1.0f, 1.0f)) {
    gfx::Rect layerRects[4];
    gfx::Rect textureRects[4];
    layerRects[0] = aLayerRect;
    textureRects[0] = aTextureRect;
    BindAndDrawQuads(aProg, 1, layerRects, textureRects);
  }
  void BindAndDrawQuadWithTextureRect(ShaderProgramOGL *aProg,
                                      const gfx::Rect& aRect,
                                      const gfx::Rect& aTexCoordRect,
                                      TextureSource *aTexture);

  void ActivateProgram(ShaderProgramOGL *aProg);
  void CleanupResources();

  



  void CopyToTarget(gfx::DrawTarget* aTarget, const nsIntPoint& aTopLeft, const gfx::Matrix& aWorldMatrix);

  








  GLint FlipY(GLint y) const { return mHeight - y; }

  RefPtr<CompositorTexturePoolOGL> mTexturePool;

  ContextStateTrackerOGL mContextStateTracker;

  bool mDestroyed;

  



  GLint mHeight;

  FenceHandle mReleaseFenceHandle;
  ShaderProgramOGL *mCurrentProgram;

  CompositorOGLVRObjects mVR;
};

}
}

#endif 
