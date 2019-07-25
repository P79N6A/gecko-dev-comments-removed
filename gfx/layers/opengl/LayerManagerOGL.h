






































#ifndef GFX_LAYERMANAGEROGL_H
#define GFX_LAYERMANAGEROGL_H

#include "Layers.h"

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

#include "LayerManagerOGLProgram.h"

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
  LayerManagerOGL(nsIWidget *aWidget);
  virtual ~LayerManagerOGL();

  void CleanupResources();

  void Destroy();


  







  PRBool Initialize() {
    return Initialize(CreateContext());
  }

  PRBool Initialize(nsRefPtr<GLContext> aContext);

  








  void SetClippingRegion(const nsIntRegion& aClippingRegion);

  


  void BeginTransaction();

  void BeginTransactionWithTarget(gfxContext* aTarget);

  void EndConstruction();

  virtual bool EndEmptyTransaction();
  virtual void EndTransaction(DrawThebesLayerCallback aCallback,
                              void* aCallbackData);

  virtual void SetRoot(Layer* aLayer) { mRoot = aLayer; }

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer();

  virtual already_AddRefed<ContainerLayer> CreateContainerLayer();

  virtual already_AddRefed<ImageLayer> CreateImageLayer();

  virtual already_AddRefed<ColorLayer> CreateColorLayer();

  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer();

  virtual already_AddRefed<ImageContainer> CreateImageContainer();

  virtual already_AddRefed<ShadowThebesLayer> CreateShadowThebesLayer();
  virtual already_AddRefed<ShadowContainerLayer> CreateShadowContainerLayer();
  virtual already_AddRefed<ShadowImageLayer> CreateShadowImageLayer();
  virtual already_AddRefed<ShadowColorLayer> CreateShadowColorLayer();
  virtual already_AddRefed<ShadowCanvasLayer> CreateShadowCanvasLayer();

  virtual LayersBackend GetBackendType() { return LAYERS_OPENGL; }
  virtual void GetBackendName(nsAString& name) { name.AssignLiteral("OpenGL"); }

  



  


  void ForgetImageContainer(ImageContainer* aContainer);
  void RememberImageContainer(ImageContainer* aContainer);

  


  void MakeCurrent(PRBool aForce = PR_FALSE) {
    if (mDestroyed) {
      NS_WARNING("Call on destroyed layer manager");
      return;
    }
    mGLContext->MakeCurrent(aForce);
  }

  ColorTextureLayerProgram *GetColorTextureLayerProgram(ProgramType type){
    return static_cast<ColorTextureLayerProgram*>(mPrograms[type]);
  }

  ColorTextureLayerProgram *GetRGBALayerProgram() {
    return static_cast<ColorTextureLayerProgram*>(mPrograms[gl::RGBALayerProgramType]);
  }
  ColorTextureLayerProgram *GetBGRALayerProgram() {
    return static_cast<ColorTextureLayerProgram*>(mPrograms[gl::BGRALayerProgramType]);
  }
  ColorTextureLayerProgram *GetRGBXLayerProgram() {
    return static_cast<ColorTextureLayerProgram*>(mPrograms[gl::RGBXLayerProgramType]);
  }
  ColorTextureLayerProgram *GetBGRXLayerProgram() {
    return static_cast<ColorTextureLayerProgram*>(mPrograms[gl::BGRXLayerProgramType]);
  }
  ColorTextureLayerProgram *GetBasicLayerProgram(PRBool aOpaque, PRBool aIsRGB)
  {
    if (aIsRGB) {
      return aOpaque
        ? GetRGBXLayerProgram()
        : GetRGBALayerProgram();
    } else {
      return aOpaque
        ? GetBGRXLayerProgram()
        : GetBGRALayerProgram();
    }
  }

  ColorTextureLayerProgram *GetRGBARectLayerProgram() {
    return static_cast<ColorTextureLayerProgram*>(mPrograms[gl::RGBARectLayerProgramType]);
  }
  SolidColorLayerProgram *GetColorLayerProgram() {
    return static_cast<SolidColorLayerProgram*>(mPrograms[gl::ColorLayerProgramType]);
  }
  YCbCrTextureLayerProgram *GetYCbCrLayerProgram() {
    return static_cast<YCbCrTextureLayerProgram*>(mPrograms[gl::YCbCrLayerProgramType]);
  }
  ComponentAlphaTextureLayerProgram *GetComponentAlphaPass1LayerProgram() {
    return static_cast<ComponentAlphaTextureLayerProgram*>
             (mPrograms[gl::ComponentAlphaPass1ProgramType]);
  }
  ComponentAlphaTextureLayerProgram *GetComponentAlphaPass2LayerProgram() {
    return static_cast<ComponentAlphaTextureLayerProgram*>
             (mPrograms[gl::ComponentAlphaPass2ProgramType]);
  }
  CopyProgram *GetCopy2DProgram() {
    return static_cast<CopyProgram*>(mPrograms[gl::Copy2DProgramType]);
  }
  CopyProgram *GetCopy2DRectProgram() {
    return static_cast<CopyProgram*>(mPrograms[gl::Copy2DRectProgramType]);
  }

  ColorTextureLayerProgram *GetFBOLayerProgram() {
    if (mFBOTextureTarget == LOCAL_GL_TEXTURE_RECTANGLE_ARB)
      return static_cast<ColorTextureLayerProgram*>(mPrograms[gl::RGBARectLayerProgramType]);
    return static_cast<ColorTextureLayerProgram*>(mPrograms[gl::RGBALayerProgramType]);
  }

  GLContext *gl() const { return mGLContext; }

  DrawThebesLayerCallback GetThebesLayerCallback() const
  { return mThebesLayerCallback; }

  void* GetThebesLayerCallbackData() const
  { return mThebesLayerCallbackData; }

  
  
  
  
  GLContext *glForResources() const {
    if (mGLContext->GetSharedContext())
      return mGLContext->GetSharedContext();
    return mGLContext;
  }

  


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

  void BindAndDrawQuad(LayerProgram *aProg,
                       bool aFlipped = false)
  {
    BindAndDrawQuad(aProg->AttribLocation(LayerProgram::VertexAttrib),
                    aProg->AttribLocation(LayerProgram::TexCoordAttrib),
                    aFlipped);
  }

  void BindAndDrawQuadWithTextureRect(LayerProgram *aProg,
                                      const nsIntRect& aTexCoordRect,
                                      const nsIntSize& aTexSize,
                                      GLenum aWrapMode = LOCAL_GL_REPEAT);
                                      

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() const { return "OGL"; }
#endif 

  const nsIntSize& GetWigetSize() {
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

  void SetRenderFPS(bool aRenderFPS) { mRenderFPS = aRenderFPS; };

private:
  
  nsIWidget *mWidget;
  nsIntSize mWidgetSize;

  


  nsRefPtr<gfxContext> mTarget;

  nsRefPtr<GLContext> mGLContext;

  already_AddRefed<mozilla::gl::GLContext> CreateContext();

  
  
  
  nsTArray<ImageContainer*> mImageContainers;

  static ProgramType sLayerProgramTypes[];

  
  GLuint mBackBufferFBO;
  GLuint mBackBufferTexture;
  nsIntSize mBackBufferSize;

  
  nsTArray<LayerManagerOGLProgram*> mPrograms;

  
  GLenum mFBOTextureTarget;

  


  GLuint mQuadVBO;

  
  nsIntRegion mClippingRegion;

  
  PRPackedBool mHasBGRA;

  
  LayerOGL *RootLayer() const;

  


  void Render();

  


  void SetupBackBuffer(int aWidth, int aHeight);

  


  void CopyToTarget();

  









  void SetLayerProgramProjectionMatrix(const gfx3DMatrix& aMatrix);

  

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
      {}
      void DrawFPS(GLContext*, CopyProgram*);
  } mFPS;

  bool mRenderFPS;
};




class LayerOGL
{
public:
  LayerOGL(LayerManagerOGL *aManager)
    : mOGLManager(aManager), mDestroyed(PR_FALSE)
  { }

  virtual ~LayerOGL() { }

  virtual LayerOGL *GetFirstChildOGL() {
    return nsnull;
  }

  


  virtual void Destroy() = 0;

  virtual Layer* GetLayer() = 0;

  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset) = 0;

  typedef mozilla::gl::GLContext GLContext;

  LayerManagerOGL* OGLManager() const { return mOGLManager; }
  GLContext *gl() const { return mOGLManager->gl(); }

  void ApplyFilter(gfxPattern::GraphicsFilter aFilter);
protected:
  LayerManagerOGL *mOGLManager;
  PRPackedBool mDestroyed;
};

} 
} 

#endif 
