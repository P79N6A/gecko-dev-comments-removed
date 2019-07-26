




#ifndef GFX_LAYERMANAGEROGL_H
#define GFX_LAYERMANAGEROGL_H

#include <sys/types.h>                  
#include "GLDefs.h"                     
#include "LayerManagerOGLProgram.h"     
#include "Layers.h"
#include "gfxMatrix.h"                  
#include "gfxPoint.h"                   
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/BaseSize.h"       
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "nsAString.h"
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsSize.h"                     
#include "nsTArray.h"                   
#include "nsThreadUtils.h"              
#include "nscore.h"                     
#ifdef XP_WIN
#include <windows.h>
#endif

#define BUFFER_OFFSET(i) ((char *)nullptr + (i))

class gfx3DMatrix;
class gfxASurface;
class gfxContext;
class nsIWidget;
struct nsIntPoint;

namespace mozilla {
namespace gl {
class GLContext;
}
namespace gfx {
class DrawTarget;
}
namespace layers {

class Composer2D;
class ImageLayer;
class LayerOGL;
class ThebesLayerComposite;
class ContainerLayerComposite;
class ImageLayerComposite;
class CanvasLayerComposite;
class ColorLayerComposite;
struct FPSState;





class LayerManagerOGL : public LayerManager
{
  typedef mozilla::gl::GLContext GLContext;

public:
  LayerManagerOGL(nsIWidget *aWidget, int aSurfaceWidth = -1, int aSurfaceHeight = -1,
                  bool aIsRenderingToEGLSurface = false);
  virtual ~LayerManagerOGL();

  void Destroy();

  





  bool Initialize(bool force = false);

  








  void SetClippingRegion(const nsIntRegion& aClippingRegion);

  


  void BeginTransaction();

  void BeginTransactionWithTarget(gfxContext* aTarget);

  void EndConstruction();

  virtual bool EndEmptyTransaction(EndTransactionFlags aFlags = END_DEFAULT);
  virtual void EndTransaction(DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              EndTransactionFlags aFlags = END_DEFAULT);

  virtual void SetRoot(Layer* aLayer) { mRoot = aLayer; }

  virtual bool CanUseCanvasLayerForSize(const gfxIntSize &aSize) {
    if (!mGLContext)
      return false;
    int32_t maxSize = GetMaxTextureSize();
    return aSize <= gfxIntSize(maxSize, maxSize);
  }

  virtual int32_t GetMaxTextureSize() const;

  virtual already_AddRefed<ThebesLayer> CreateThebesLayer();

  virtual already_AddRefed<ContainerLayer> CreateContainerLayer();

  virtual already_AddRefed<ImageLayer> CreateImageLayer();

  virtual already_AddRefed<ColorLayer> CreateColorLayer();

  virtual already_AddRefed<CanvasLayer> CreateCanvasLayer();

  virtual LayersBackend GetBackendType() { return LAYERS_OPENGL; }
  virtual void GetBackendName(nsAString& name) { name.AssignLiteral("OpenGL"); }

  virtual already_AddRefed<gfxASurface>
    CreateOptimalMaskSurface(const gfxIntSize &aSize);

  virtual void ClearCachedResources(Layer* aSubtree = nullptr) MOZ_OVERRIDE;

  


  void MakeCurrent(bool aForce = false);

  ShaderProgramOGL* GetBasicLayerProgram(bool aOpaque, bool aIsRGB,
                                         MaskType aMask = MaskNone)
  {
    ShaderProgramType format = BGRALayerProgramType;
    if (aIsRGB) {
      if (aOpaque) {
        format = RGBXLayerProgramType;
      } else {
        format = RGBALayerProgramType;
      }
    } else {
      if (aOpaque) {
        format = BGRXLayerProgramType;
      }
    }
    return GetProgram(format, aMask);
  }

  ShaderProgramOGL* GetProgram(ShaderProgramType aType,
                               Layer* aMaskLayer) {
    if (aMaskLayer)
      return GetProgram(aType, Mask2d);
    return GetProgram(aType, MaskNone);
  }

  ShaderProgramOGL* GetProgram(ShaderProgramType aType,
                               MaskType aMask = MaskNone) {
    NS_ASSERTION(ProgramProfileOGL::ProgramExists(aType, aMask),
                 "Invalid program type.");
    return mPrograms[aType].mVariations[aMask];
  }

  ShaderProgramOGL* GetFBOLayerProgram(MaskType aMask = MaskNone) {
    return GetProgram(GetFBOLayerProgramType(), aMask);
  }

  ShaderProgramType GetFBOLayerProgramType() {
    if (mFBOTextureTarget == LOCAL_GL_TEXTURE_RECTANGLE_ARB)
      return RGBARectLayerProgramType;
    return RGBALayerProgramType;
  }

  gfx::SurfaceFormat GetFBOTextureFormat() {
    return gfx::FORMAT_R8G8B8A8;
  }

  GLContext* gl() const { return mGLContext; }

  
  void* GetNSOpenGLContext() const;

  DrawThebesLayerCallback GetThebesLayerCallback() const
  { return mThebesLayerCallback; }

  void* GetThebesLayerCallbackData() const
  { return mThebesLayerCallbackData; }

  GLenum FBOTextureTarget() { return mFBOTextureTarget; }

  







  enum InitMode {
    InitModeNone,
    InitModeClear,
    InitModeCopy
  };

  





  bool CreateFBOWithTexture(const nsIntRect& aRect, InitMode aInit,
                            GLuint aCurrentFrameBuffer,
                            GLuint *aFBO, GLuint *aTexture);

  GLuint QuadVBO() { return mQuadVBO; }
  GLintptr QuadVBOVertexOffset() { return 0; }
  GLintptr QuadVBOTexCoordOffset() { return sizeof(float)*4*2; }
  GLintptr QuadVBOFlippedTexCoordOffset() { return sizeof(float)*8*2; }

  void BindQuadVBO();
  void QuadVBOVerticesAttrib(GLuint aAttribIndex);
  void QuadVBOTexCoordsAttrib(GLuint aAttribIndex);
  void QuadVBOFlippedTexCoordsAttrib(GLuint aAttribIndex);

  

  void BindAndDrawQuad(GLuint aVertAttribIndex,
                       GLuint aTexCoordAttribIndex,
                       bool aFlipped = false);

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

  virtual const char* Name() const { return "OGL"; }

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

  void UpdateRenderBounds(const nsIntRect& aRect);

  


  void SetSurfaceSize(int width, int height);
 
  bool CompositingDisabled() { return mCompositingDisabled; }
  void SetCompositingDisabled(bool aCompositingDisabled) { mCompositingDisabled = aCompositingDisabled; }

  



  virtual TemporaryRef<mozilla::gfx::DrawTarget>
    CreateDrawTarget(const mozilla::gfx::IntSize &aSize,
                     mozilla::gfx::SurfaceFormat aFormat);

  






  float ComputeRenderIntegrity();

private:
  
  nsIWidget *mWidget;
  nsIntSize mWidgetSize;

  
  nsIntSize mSurfaceSize;

  


  nsRefPtr<gfxContext> mTarget;

  nsRefPtr<GLContext> mGLContext;

  
  nsRefPtr<Composer2D> mComposer2D;

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

  
  class ReadDrawFPSPref MOZ_FINAL : public nsRunnable {
  public:
    NS_IMETHOD Run() MOZ_OVERRIDE;
  };

  
  LayerOGL *RootLayer() const;

  


  void Render();

  


  void SetupBackBuffer(int aWidth, int aHeight);

  


  void CopyToTarget(gfxContext *aTarget);

  


  void SetLayerProgramProjectionMatrix(const gfx3DMatrix& aMatrix);

  



  void AddPrograms(ShaderProgramType aType);

  





  static void ComputeRenderIntegrityInternal(Layer* aLayer,
                                             nsIntRegion& aScreenRegion,
                                             nsIntRegion& aLowPrecisionScreenRegion,
                                             const gfx3DMatrix& aTransform);

  

  DrawThebesLayerCallback mThebesLayerCallback;
  void *mThebesLayerCallbackData;
  gfxMatrix mWorldMatrix;
  nsAutoPtr<FPSState> mFPS;
  nsIntRect mRenderBounds;
#ifdef DEBUG
  
  
  
  
  bool mMaybeInvalidTree;
#endif

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
