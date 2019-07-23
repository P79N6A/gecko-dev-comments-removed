




































#include "ContainerLayerOGL.h"

#include "glWrapper.h"

namespace mozilla {
namespace layers {

ContainerLayerOGL::ContainerLayerOGL(LayerManager *aManager)
  : ContainerLayer(aManager, NULL)
{
  mImplData = static_cast<LayerOGL*>(this);
}

const nsIntRect&
ContainerLayerOGL::GetVisibleRect()
{
  return mVisibleRect;
}

void
ContainerLayerOGL::SetVisibleRegion(const nsIntRegion &aRegion)
{
  mVisibleRect = aRegion.GetBounds();
}

void
ContainerLayerOGL::InsertAfter(Layer* aChild, Layer* aAfter)
{
  LayerOGL *newChild = static_cast<LayerOGL*>(aChild->ImplData());
  aChild->SetParent(this);
  if (!aAfter) {
    LayerOGL *oldFirstChild = GetFirstChildOGL();
    mFirstChild = newChild->GetLayer();
    newChild->SetNextSibling(oldFirstChild);
    return;
  }
  for (LayerOGL *child = GetFirstChildOGL(); 
    child; child = child->GetNextSibling()) {
    if (aAfter == child->GetLayer()) {
      LayerOGL *oldNextSibling = child->GetNextSibling();
      child->SetNextSibling(newChild);
      child->GetNextSibling()->SetNextSibling(oldNextSibling);
      return;
    }
  }
  NS_WARNING("Failed to find aAfter layer!");
}

void
ContainerLayerOGL::RemoveChild(Layer *aChild)
{
  if (GetFirstChild() == aChild) {
    mFirstChild = GetFirstChildOGL()->GetNextSibling()->GetLayer();
    return;
  }
  LayerOGL *lastChild = NULL;
  for (LayerOGL *child = GetFirstChildOGL(); child; 
    child = child->GetNextSibling()) {
    if (child->GetLayer() == aChild) {
      
      lastChild->SetNextSibling(child->GetNextSibling());
      child->SetNextSibling(NULL);
      child->GetLayer()->SetParent(NULL);
      return;
    }
    lastChild = child;
  }
}

LayerOGL::LayerType
ContainerLayerOGL::GetType()
{
  return TYPE_CONTAINER;
}

Layer*
ContainerLayerOGL::GetLayer()
{
  return this;
}

LayerOGL*
ContainerLayerOGL::GetFirstChildOGL()
{
  if (!mFirstChild) {
    return nsnull;
  }
  return static_cast<LayerOGL*>(mFirstChild->ImplData());
}

void
ContainerLayerOGL::RenderLayer(int aPreviousFrameBuffer)
{
  


  GLuint containerSurface;

  sglWrapper.GenTextures(1, &containerSurface);
  sglWrapper.BindTexture(LOCAL_GL_TEXTURE_2D, containerSurface);
  sglWrapper.TexImage2D(LOCAL_GL_TEXTURE_2D,
                        0,
                        LOCAL_GL_RGBA,
                        mVisibleRect.width,
                        mVisibleRect.height,
                        0,
                        LOCAL_GL_BGRA,
                        LOCAL_GL_UNSIGNED_BYTE,
                        NULL);

  



  GLuint frameBuffer;
  sglWrapper.GenFramebuffersEXT(1, &frameBuffer);
  sglWrapper.BindFramebufferEXT(LOCAL_GL_FRAMEBUFFER_EXT, frameBuffer);
  sglWrapper.FramebufferTexture2DEXT(LOCAL_GL_FRAMEBUFFER_EXT,
                                     LOCAL_GL_COLOR_ATTACHMENT0_EXT,
                                     LOCAL_GL_TEXTURE_2D,
                                     containerSurface,
                                     0);

  RGBLayerProgram *rgbProgram =
    static_cast<LayerManagerOGL*>(mManager)->GetRGBLayerProgram();
  YCbCrLayerProgram *yCbCrProgram =
    static_cast<LayerManagerOGL*>(mManager)->GetYCbCrLayerProgram();

  



  
  rgbProgram->Activate();
  rgbProgram->PushRenderTargetOffset((GLfloat)GetVisibleRect().x,
                                     (GLfloat)GetVisibleRect().y);
  yCbCrProgram->Activate();
  yCbCrProgram->PushRenderTargetOffset((GLfloat)GetVisibleRect().x,
                                       (GLfloat)GetVisibleRect().y);
  
  


  LayerOGL *layerToRender = GetFirstChildOGL();
  while (layerToRender) {
    const nsIntRect *clipRect = layerToRender->GetLayer()->GetClipRect();
    if (clipRect) {
      sglWrapper.Scissor(clipRect->x - GetVisibleRect().x,
                clipRect->y - GetVisibleRect().y,
                clipRect->width,
                clipRect->height);
    } else {
      sglWrapper.Scissor(0, 0, GetVisibleRect().width, GetVisibleRect().height);
    }

    layerToRender->RenderLayer(frameBuffer);
    layerToRender = layerToRender->GetNextSibling();
  }

  
  sglWrapper.BindFramebufferEXT(LOCAL_GL_FRAMEBUFFER_EXT, aPreviousFrameBuffer);
  sglWrapper.DeleteFramebuffersEXT(1, &frameBuffer);

  
  yCbCrProgram->Activate();
  yCbCrProgram->PopRenderTargetOffset();

  rgbProgram->Activate();
  rgbProgram->PopRenderTargetOffset();

  


  float quadTransform[4][4];
  



  memset(&quadTransform, 0, sizeof(quadTransform));
  quadTransform[0][0] = (float)GetVisibleRect().width;
  quadTransform[1][1] = (float)GetVisibleRect().height;
  quadTransform[2][2] = 1.0f;
  quadTransform[3][0] = (float)GetVisibleRect().x;
  quadTransform[3][1] = (float)GetVisibleRect().y;
  quadTransform[3][3] = 1.0f;

  rgbProgram->SetLayerQuadTransform(&quadTransform[0][0]);

  sglWrapper.BindTexture(LOCAL_GL_TEXTURE_2D, containerSurface);

  rgbProgram->SetLayerOpacity(GetOpacity());
  rgbProgram->SetLayerTransform(&mTransform._11);
  rgbProgram->Apply();

  sglWrapper.DrawArrays(LOCAL_GL_TRIANGLE_STRIP, 0, 4);

  
  sglWrapper.DeleteTextures(1, &containerSurface);
}

} 
} 
