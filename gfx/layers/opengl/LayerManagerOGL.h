




#ifndef GFX_LAYERMANAGEROGL_H
#define GFX_LAYERMANAGEROGL_H

#include "LayerManagerOGLProgram.h"

#include "mozilla/layers/ShadowLayers.h"

#include "mozilla/TimeStamp.h"

#ifdef XP_WIN
#include <windows.h>
#endif





typedef unsigned int GLenum;
typedef unsigned int GLbitfield;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#include "gfxContext.h"
#include "gfx3DMatrix.h"
#include "nsIWidget.h"
#include "GLContext.h"

namespace mozilla {
namespace layers {

class LayerOGL;
class ShadowThebesLayer;
class ShadowContainerLayer;
class ShadowImageLayer;
class ShadowCanvasLayer;
class ShadowColorLayer;





class THEBES_API LayerManagerOGL :
    public ShadowLayerManager
{
  typedef mozilla::gl::GLContext GLContext;
  typedef mozilla::gl::ShaderProgramType ProgramType;

public:
  LayerManagerOGL(nsIWidget *aWidget, int aSurfaceWidth = -1, int aSurfaceHeight = -1,
                  bool aIsRenderingToEGLSurface = false);
  virtual ~LayerManagerOGL();

  void CleanupResources();

  void Destroy();


  







  bool Initialize(bool force = false) {
    return Initialize(CreateContext(), force);
  }

  bool Initialize(nsRefPtr<GLContext> aContext, bool force = false);

  








  void SetClippingRegion(const nsIntRegion& aClippingRegion);

  


  virtual ShadowLayerManager* AsShadowManager()
  {
    return this;
  }

  void BeginTransaction();

  void BeginTransactionWithTarget(gfxContext* aTarget);

  void EndConstruction();

  virtual bool EndEmptyTransaction(EndTransactionFlags aFlags = END_DEFAULT);
  virtual void EndTransaction(DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags aFlags = END_DEFAULT);

  virtual void SetRoot(Layer* aLayer) { mRoot = aLayer; }

  virtual bool CanUseCanvasLayerForSize(const gfxIntSize &aSize)
  {
      if (!mGLContext)
          return false;
      int32_t maxSize = mGLContext->GetMaxTextureSize();
      return aSize <= gfxIntSize(maxSize, maxSize);
  }

  virtual int32_t GetMaxTextureSize() const
  {
    return mGLContext->GetMaxTextureSize();
  }

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer();

  virtual already_AddRefed<ContainerLayer> CreateContainerLayer();

  virtual already_AddRefed<ImageLayer> CreateImageLayer();

  virtual already_AddRefed<ColorLayer> CreateColorLayer();

  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer();

  virtual already_AddRefed<ShadowThebesLayer> CreateShadowThebesLayer();
  virtual already_AddRefed<ShadowContainerLayer> CreateShadowContainerLayer();
  virtual already_AddRefed<ShadowImageLayer> CreateShadowImageLayer();
  virtual already_AddRefed<ShadowColorLayer> CreateShadowColorLayer();
  virtual already_AddRefed<ShadowCanvasLayer> CreateShadowCanvasLayer();
  virtual already_AddRefed<ShadowRefLayer> CreateShadowRefLayer();

  virtual LayersBackend GetBackendType() { return LAYERS_OPENGL; }
  virtual void GetBackendName(nsAString& name) { name.AssignLiteral("OpenGL"); }

  virtual already_AddRefed<gfxASurface>
    CreateOptimalMaskSurface(const gfxIntSize &aSize);

  


  void MakeCurrent(bool aForce = false) {
    if (mDestroyed) {
      NS_WARNING("Call on destroyed layer manager");
      return;
    }
    mGLContext->MakeCurrent(aForce);
  }

  ShaderProgramOGL* GetBasicLayerProgram(bool aOpaque, bool aIsRGB,
                                         MaskType aMask = MaskNone)
  {
    gl::ShaderProgramType format = gl::BGRALayerProgramType;
    if (aIsRGB) {
      if (aOpaque) {
        format = gl::RGBXLayerProgramType;
      } else {
        format = gl::RGBALayerProgramType;
      }
    } else {
      if (aOpaque) {
        format = gl::BGRXLayerProgramType;
      }
    }
    return GetProgram(format, aMask);
  }

  ShaderProgramOGL* GetProgram(gl::ShaderProgramType aType,
                               Layer* aMaskLayer) {
    if (aMaskLayer)
      return GetProgram(aType, Mask2d);
    return GetProgram(aType, MaskNone);
  }

  ShaderProgramOGL* GetProgram(gl::ShaderProgramType aType,
                               MaskType aMask = MaskNone) {
    NS_ASSERTION(ProgramProfileOGL::ProgramExists(aType, aMask),
                 "Invalid program type.");
    return mPrograms[aType].mVariations[aMask];
  }

  ShaderProgramOGL* GetFBOLayerProgram(MaskType aMask = MaskNone) {
    return GetProgram(GetFBOLayerProgramType(), aMask);
  }

  gl::ShaderProgramType GetFBOLayerProgramType() {
    if (mFBOTextureTarget == LOCAL_GL_TEXTURE_RECTANGLE_ARB)
      return gl::RGBARectLayerProgramType;
    return gl::RGBALayerProgramType;
  }

  GLContext* gl() const { return mGLContext; }

  DrawThebesLayerCallback GetThebesLayerCallback() const
  { return mThebesLayerCallback; }

  void* GetThebesLayerCallbackData() const
  { return mThebesLayerCallbackData; }

  


  void CallThebesLayerDrawCallback(ThebesLayer* aLayer,
                                   gfxContext* aContext,
                                   const nsIntRegion& aRegionToDraw)
  {
    NS_ASSERTION(mThebesLayerCallback,
                 "CallThebesLayerDrawCallback without callback!");
    mThebesLayerCallback(aLayer, aContext,
                         aRegionToDraw, nsIntRegion(),
                         mThebesLayerCallbackData);
  }

  GLenum FBOTextureTarget() { return mFBOTextureTarget; }

  







  enum InitMode {
    InitModeNone,
    InitModeClear,
    InitModeCopy
  };

  





  void CreateFBOWithTexture(const nsIntRect& aRect, InitMode aInit,
                            GLuint aCurrentFrameBuffer,
                            GLuint *aFBO, GLuint *aTexture);

  GLuint QuadVBO() { return mQuadVBO; }
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
                                      const nsIntRect& aTexCoordRect,
                                      const nsIntSize& aTexSize,
                                      GLenum aWrapMode = LOCAL_GL_REPEAT,
                                      bool aFlipped = false);

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() const { return "OGL"; }
#endif 

  const nsIntSize& GetWidgetSize() {
    return mWidgetSize;
  }

  enum WorldTransforPolicy {
    ApplyWorldTransform,
    DontApplyWorldTransform
  };

  



  void SetupPipeline(int aWidth, int aHeight, WorldTransforPolicy aTransformPolicy);

  




  void SetWorldTransform(const gfxMatrix& aMatrix);
  gfxMatrix& GetWorldTransform(void);
  void WorldTransformRect(nsIntRect& aRect);

  


  void SetSurfaceSize(int width, int height);

  bool CompositingDisabled() { return mCompositingDisabled; }
  void SetCompositingDisabled(bool aCompositingDisabled) { mCompositingDisabled = aCompositingDisabled; }

private:
  
  nsIWidget *mWidget;
  nsIntSize mWidgetSize;

  
  nsIntSize mSurfaceSize;

  


  nsRefPtr<gfxContext> mTarget;

  nsRefPtr<GLContext> mGLContext;

  already_AddRefed<mozilla::gl::GLContext> CreateContext();

  
  GLuint mBackBufferFBO;
  GLuint mBackBufferTexture;
  nsIntSize mBackBufferSize;

  
  struct ShaderProgramVariations {
    ShaderProgramOGL* mVariations[NumMaskTypes];
  };
  nsTArray<ShaderProgramVariations> mPrograms;

  
  GLenum mFBOTextureTarget;

  


  GLuint mQuadVBO;

  
  nsIntRegion mClippingRegion;

  
  bool mHasBGRA;
  bool mCompositingDisabled;

  




  bool mIsRenderingToEGLSurface;

  
  LayerOGL *RootLayer() const;

  


  void Render();

  


  void SetupBackBuffer(int aWidth, int aHeight);

  


  void CopyToTarget(gfxContext *aTarget);

  


  void SetLayerProgramProjectionMatrix(const gfx3DMatrix& aMatrix);

  



  void AddPrograms(gl::ShaderProgramType aType);

  

  DrawThebesLayerCallback mThebesLayerCallback;
  void *mThebesLayerCallbackData;
  gfxMatrix mWorldMatrix;

  struct FPSState
  {
      GLuint texture;
      int fps;
      bool initialized;
      int fcount;
      TimeStamp last;

      FPSState()
        : texture(0)
        , fps(0)
        , initialized(false)
        , fcount(0)
      {
        last = TimeStamp::Now();
      }
      void DrawFPS(GLContext*, ShaderProgramOGL*);
  } mFPS;

  static bool sDrawFPS;
};




class LayerOGL
{
public:
  LayerOGL(LayerManagerOGL *aManager)
    : mOGLManager(aManager), mDestroyed(false)
  { }

  virtual ~LayerOGL() { }

  virtual LayerOGL *GetFirstChildOGL() {
    return nullptr;
  }

  


  virtual void Destroy() = 0;

  virtual Layer* GetLayer() = 0;

  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset) = 0;

  typedef mozilla::gl::GLContext GLContext;

  LayerManagerOGL* OGLManager() const { return mOGLManager; }
  GLContext *gl() const { return mOGLManager->gl(); }
  virtual void CleanupResources() = 0;

  









  virtual bool LoadAsTexture(GLuint aTextureUnit, gfxIntSize* aSize)
  {
    NS_WARNING("LoadAsTexture called without being overriden");
    return false;
  }

protected:
  LayerManagerOGL *mOGLManager;
  bool mDestroyed;
};

} 
} 

#endif 
