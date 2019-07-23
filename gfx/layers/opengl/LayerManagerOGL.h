




































#ifndef GFX_LAYERMANAGEROGL_H
#define GFX_LAYERMANAGEROGL_H

#include "Layers.h"

#ifdef XP_WIN
#include <windows.h>
#endif

typedef unsigned int GLuint;
typedef int GLint;
typedef float GLfloat;
typedef char GLchar;

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#include "gfxContext.h"
#include "nsIWidget.h"

namespace mozilla {
namespace layers {

class LayerOGL;

struct GLvec2
{
  GLfloat mX;
  GLfloat mY;
};




class LayerProgram
{
public:
  LayerProgram();
  virtual ~LayerProgram();

  PRBool Initialize(GLuint aVertexShader, GLuint aFragmentShader);

  virtual void UpdateLocations();

  void Activate();

  void SetMatrixUniform(GLint aLocation, const GLfloat *aValue);
  void SetInt(GLint aLocation, GLint aValue);

  void SetMatrixProj(GLfloat *aValue)
  {
    SetMatrixUniform(mMatrixProjLocation, aValue);
  }

  void SetLayerQuadTransform(GLfloat *aValue)
  {
    SetMatrixUniform(mLayerQuadTransformLocation, aValue);
  }

  void SetLayerTransform(const GLfloat *aValue)
  {
    SetMatrixUniform(mLayerTransformLocation, aValue);
  }

  void SetLayerOpacity(GLfloat aValue);

  void PushRenderTargetOffset(GLfloat aValueX, GLfloat aValueY);
  void PopRenderTargetOffset();

  void Apply();

protected:
  GLuint mProgram;
  GLint mMatrixProjLocation;
  GLint mLayerQuadTransformLocation;
  GLint mLayerTransformLocation;
  GLint mRenderTargetOffsetLocation;
  GLint mLayerOpacityLocation;

  nsTArray<GLvec2> mRenderTargetOffsetStack;
};

class RGBLayerProgram : public LayerProgram
{
public:
  void UpdateLocations();

  void SetLayerTexture(GLint aValue)
  {
    SetInt(mLayerTextureLocation, aValue);
  }
protected:
  GLint mLayerTextureLocation;
};

class YCbCrLayerProgram : public LayerProgram
{
public:
  void UpdateLocations();

  void SetYTexture(GLint aValue)
  {
    SetInt(mYTextureLocation, aValue);
  }
  void SetCbTexture(GLint aValue)
  {
    SetInt(mCbTextureLocation, aValue);
  }
  void SetCrTexture(GLint aValue)
  {
    SetInt(mCrTextureLocation, aValue);
  }
protected:
  GLint mYTextureLocation;
  GLint mCbTextureLocation;
  GLint mCrTextureLocation;
};





class THEBES_API LayerManagerOGL : public LayerManager {
public:
  LayerManagerOGL(nsIWidget *aWidget);
  virtual ~LayerManagerOGL();

  







  PRBool Initialize();

  








  void SetClippingRegion(const nsIntRegion& aClippingRegion);

  


  void BeginTransaction();

  void BeginTransactionWithTarget(gfxContext* aTarget);

  void EndConstruction();

  void EndTransaction();

  void SetRoot(Layer* aLayer);
  
  virtual already_AddRefed<ThebesLayer> CreateThebesLayer();

  virtual already_AddRefed<ContainerLayer> CreateContainerLayer();

  virtual already_AddRefed<ImageLayer> CreateImageLayer();

  virtual already_AddRefed<ImageContainer> CreateImageContainer();

  virtual LayersBackend GetBackendType() { return LAYERS_OPENGL; }

  


  void SetClippingEnabled(PRBool aEnabled);

  void MakeCurrent();

  RGBLayerProgram *GetRGBLayerProgram() { return mRGBLayerProgram; }
  YCbCrLayerProgram *GetYCbCrLayerProgram() { return mYCbCrLayerProgram; }

private:
  
  nsIWidget *mWidget;
  


  nsRefPtr<gfxContext> mTarget;

#ifdef XP_WIN
  
  HDC mDC;

  
  HGLRC mContext;
#endif

  
  GLuint mBackBuffer;
  
  nsIntSize mBackBufferSize;
  
  GLuint mFrameBuffer;
  
  RGBLayerProgram *mRGBLayerProgram;
  
  YCbCrLayerProgram *mYCbCrLayerProgram;
  
  GLuint mVertexShader;
  
  GLuint mRGBShader;
  
  GLuint mYUVShader;
  
  LayerOGL *mRootLayer;
  
  GLuint mVBO;

  


  nsIntRegion mClippingRegion;
  


  void Render();
  


  void SetupPipeline();
  




  PRBool SetupBackBuffer();
  


  void CopyToTarget();
};




class LayerOGL
{
public:
  LayerOGL();

  enum LayerType { TYPE_THEBES, TYPE_CONTAINER, TYPE_IMAGE };
  
  virtual LayerType GetType() = 0;

  LayerOGL *GetNextSibling();
  virtual LayerOGL *GetFirstChildOGL() { return nsnull; }

  void SetNextSibling(LayerOGL *aParent);
  void SetFirstChild(LayerOGL *aParent);

  virtual Layer* GetLayer() = 0;

  virtual void RenderLayer(int aPreviousFrameBuffer) = 0;

protected:
  LayerOGL *mNextSibling;
};

} 
} 

#endif 
