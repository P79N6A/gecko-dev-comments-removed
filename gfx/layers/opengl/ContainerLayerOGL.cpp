




































#include "ContainerLayerOGL.h"

namespace mozilla {
namespace layers {

ContainerLayerOGL::ContainerLayerOGL(LayerManagerOGL *aManager)
  : ContainerLayer(aManager, NULL)
  , LayerOGL(aManager)
{
  mImplData = static_cast<LayerOGL*>(this);
}

ContainerLayerOGL::~ContainerLayerOGL()
{
  Destroy();
}

void
ContainerLayerOGL::Destroy()
{
  if (!mDestroyed) {
    while (mFirstChild) {
      GetFirstChildOGL()->Destroy();
      RemoveChild(mFirstChild);
    }
    mDestroyed = PR_TRUE;
  }
}

void
ContainerLayerOGL::InsertAfter(Layer* aChild, Layer* aAfter)
{
  aChild->SetParent(this);
  if (!aAfter) {
    Layer *oldFirstChild = GetFirstChild();
    mFirstChild = aChild;
    aChild->SetNextSibling(oldFirstChild);
    aChild->SetPrevSibling(nsnull);
    if (oldFirstChild) {
      oldFirstChild->SetPrevSibling(aChild);
    }
    NS_ADDREF(aChild);
    return;
  }
  for (Layer *child = GetFirstChild(); 
       child; child = child->GetNextSibling()) {
    if (aAfter == child) {
      Layer *oldNextSibling = child->GetNextSibling();
      child->SetNextSibling(aChild);
      aChild->SetNextSibling(oldNextSibling);
      if (oldNextSibling) {
        oldNextSibling->SetPrevSibling(aChild);
      }
      aChild->SetPrevSibling(child);
      NS_ADDREF(aChild);
      return;
    }
  }
  NS_WARNING("Failed to find aAfter layer!");
}

void
ContainerLayerOGL::RemoveChild(Layer *aChild)
{
  if (GetFirstChild() == aChild) {
    mFirstChild = GetFirstChild()->GetNextSibling();
    if (mFirstChild) {
      mFirstChild->SetPrevSibling(nsnull);
    }
    aChild->SetNextSibling(nsnull);
    aChild->SetPrevSibling(nsnull);
    aChild->SetParent(nsnull);
    NS_RELEASE(aChild);
    return;
  }
  Layer *lastChild = nsnull;
  for (Layer *child = GetFirstChild(); child; 
       child = child->GetNextSibling()) {
    if (child == aChild) {
      
      lastChild->SetNextSibling(child->GetNextSibling());
      if (child->GetNextSibling()) {
        child->GetNextSibling()->SetPrevSibling(lastChild);
      }
      child->SetNextSibling(nsnull);
      child->SetPrevSibling(nsnull);
      child->SetParent(nsnull);
      NS_RELEASE(aChild);
      return;
    }
    lastChild = child;
  }
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
ContainerLayerOGL::RenderLayer(int aPreviousFrameBuffer,
                               const nsIntPoint& aOffset)
{
  


  GLuint containerSurface;
  GLuint frameBuffer;

  nsIntPoint childOffset(aOffset);
  nsIntRect visibleRect = GetEffectiveVisibleRegion().GetBounds();
  const gfx3DMatrix& transform = GetEffectiveTransform();

  gl()->PushScissorRect();

  float opacity = GetOpacity();
  bool needsFramebuffer = (opacity != 1.0) || !transform.IsIdentity();
  if (needsFramebuffer) {
    mOGLManager->CreateFBOWithTexture(visibleRect.width,
                                      visibleRect.height,
                                      &frameBuffer,
                                      &containerSurface);
    childOffset.x = visibleRect.x;
    childOffset.y = visibleRect.y;

    gl()->PushViewportRect();
    mOGLManager->SetupPipeline(visibleRect.width, visibleRect.height);

    gl()->fScissor(0, 0, visibleRect.width, visibleRect.height);
    gl()->fClearColor(0.0, 0.0, 0.0, 0.0);
    gl()->fClear(LOCAL_GL_COLOR_BUFFER_BIT);
  } else {
    frameBuffer = aPreviousFrameBuffer;
  }

  


  LayerOGL *layerToRender = GetFirstChildOGL();
  while (layerToRender) {
    nsIntRect scissorRect(visibleRect);

    const nsIntRect *clipRect = layerToRender->GetLayer()->GetEffectiveClipRect();
    if (clipRect) {
      scissorRect = *clipRect;
    }

    if (needsFramebuffer) {
      scissorRect.MoveBy(- visibleRect.TopLeft());
    }

    if (aPreviousFrameBuffer == 0) {
      gl()->FixWindowCoordinateRect(scissorRect, mOGLManager->GetWigetSize().height);
    }

    gl()->fScissor(scissorRect.x, scissorRect.y, scissorRect.width, scissorRect.height);

    layerToRender->RenderLayer(frameBuffer, childOffset);

    Layer *nextSibling = layerToRender->GetLayer()->GetNextSibling();
    layerToRender = nextSibling ? static_cast<LayerOGL*>(nextSibling->
                                                         ImplData())
                                : nsnull;
  }

  gl()->PopScissorRect();

  if (needsFramebuffer) {
    
    
    
    gl()->PopViewportRect();
    nsIntRect viewport = gl()->ViewportRect();
    mOGLManager->SetupPipeline(viewport.width, viewport.height);

    gl()->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, aPreviousFrameBuffer);
    gl()->fDeleteFramebuffers(1, &frameBuffer);

    gl()->fActiveTexture(LOCAL_GL_TEXTURE0);

    gl()->fBindTexture(mOGLManager->FBOTextureTarget(), containerSurface);

    ColorTextureLayerProgram *rgb = mOGLManager->GetFBOLayerProgram();

    rgb->Activate();
    rgb->SetLayerQuadRect(visibleRect);
    rgb->SetLayerTransform(transform);
    rgb->SetLayerOpacity(opacity);
    rgb->SetRenderOffset(aOffset);
    rgb->SetTextureUnit(0);

    if (rgb->GetTexCoordMultiplierUniformLocation() != -1) {
      
      float f[] = { float(visibleRect.width), float(visibleRect.height) };
      rgb->SetUniform(rgb->GetTexCoordMultiplierUniformLocation(),
                      2, f);
    }

    DEBUG_GL_ERROR_CHECK(gl());

    mOGLManager->BindAndDrawQuad(rgb);

    DEBUG_GL_ERROR_CHECK(gl());

    
    gl()->fDeleteTextures(1, &containerSurface);

    DEBUG_GL_ERROR_CHECK(gl());
  }
}

} 
} 
