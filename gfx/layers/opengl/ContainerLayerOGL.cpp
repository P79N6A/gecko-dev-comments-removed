




































#include "ContainerLayerOGL.h"

namespace mozilla {
namespace layers {

ContainerLayerOGL::ContainerLayerOGL(LayerManagerOGL *aManager)
  : ContainerLayer(aManager, NULL)
  , LayerOGL(aManager)
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
  GLuint frameBuffer;
  RGBLayerProgram *rgbProgram =
    static_cast<LayerManagerOGL*>(mManager)->GetRGBLayerProgram();
  YCbCrLayerProgram *yCbCrProgram =
    static_cast<LayerManagerOGL*>(mManager)->GetYCbCrLayerProgram();

  if (GetOpacity() != 1.0) {
    gl()->fGenTextures(1, &containerSurface);
    gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, containerSurface);
    gl()->fTexImage2D(LOCAL_GL_TEXTURE_2D,
			    0,
			    LOCAL_GL_RGBA,
			    mVisibleRect.width,
			    mVisibleRect.height,
			    0,
			    LOCAL_GL_BGRA,
			    LOCAL_GL_UNSIGNED_BYTE,
			    NULL);
    gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, LOCAL_GL_LINEAR);
    gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, LOCAL_GL_LINEAR);

    



    gl()->fGenFramebuffers(1, &frameBuffer);
    gl()->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, frameBuffer);
    gl()->fFramebufferTexture2D(LOCAL_GL_FRAMEBUFFER,
                              LOCAL_GL_COLOR_ATTACHMENT0,
                              LOCAL_GL_TEXTURE_2D,
                              containerSurface,
                              0);

    NS_ASSERTION(
	  gl()->fCheckFramebufferStatus(LOCAL_GL_FRAMEBUFFER) ==
	    LOCAL_GL_FRAMEBUFFER_COMPLETE, "Error setting up framebuffer.");

    



    
    rgbProgram->Activate();
    rgbProgram->PushRenderTargetOffset((GLfloat)GetVisibleRect().x,
					 (GLfloat)GetVisibleRect().y);
    yCbCrProgram->Activate();
    yCbCrProgram->PushRenderTargetOffset((GLfloat)GetVisibleRect().x,
					   (GLfloat)GetVisibleRect().y);
  } else {
    frameBuffer = aPreviousFrameBuffer;
  }
  
  


  LayerOGL *layerToRender = GetFirstChildOGL();
  while (layerToRender) {
    const nsIntRect *clipRect = layerToRender->GetLayer()->GetClipRect();
    if (clipRect) {
      gl()->fScissor(clipRect->x - GetVisibleRect().x,
                   clipRect->y - GetVisibleRect().y,
                   clipRect->width,
                   clipRect->height);
    } else {
      gl()->fScissor(0, 0, GetVisibleRect().width, GetVisibleRect().height);
    }

    layerToRender->RenderLayer(frameBuffer);
    layerToRender = layerToRender->GetNextSibling();
  }

  if (GetOpacity() != 1.0) {
    
    gl()->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, aPreviousFrameBuffer);
    gl()->fDeleteFramebuffers(1, &frameBuffer);

    
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

    gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, containerSurface);

    rgbProgram->SetLayerOpacity(GetOpacity());
    rgbProgram->SetLayerTransform(&mTransform._11);
    rgbProgram->Apply();

    gl()->fDrawArrays(LOCAL_GL_TRIANGLE_STRIP, 0, 4);

    
    gl()->fDeleteTextures(1, &containerSurface);
  }
}

} 
} 
