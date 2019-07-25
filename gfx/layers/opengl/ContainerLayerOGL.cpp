




































#include "ContainerLayerOGL.h"

namespace mozilla {
namespace layers {

template<class Container>
static void
ContainerInsertAfter(Container* aContainer, Layer* aChild, Layer* aAfter)
{
  aChild->SetParent(aContainer);
  if (!aAfter) {
    Layer *oldFirstChild = aContainer->GetFirstChild();
    aContainer->mFirstChild = aChild;
    aChild->SetNextSibling(oldFirstChild);
    aChild->SetPrevSibling(nsnull);
    if (oldFirstChild) {
      oldFirstChild->SetPrevSibling(aChild);
    }
    NS_ADDREF(aChild);
    return;
  }
  for (Layer *child = aContainer->GetFirstChild(); 
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

template<class Container>
static void
ContainerRemoveChild(Container* aContainer, Layer* aChild)
{
  if (aContainer->GetFirstChild() == aChild) {
    aContainer->mFirstChild = aContainer->GetFirstChild()->GetNextSibling();
    if (aContainer->mFirstChild) {
      aContainer->mFirstChild->SetPrevSibling(nsnull);
    }
    aChild->SetNextSibling(nsnull);
    aChild->SetPrevSibling(nsnull);
    aChild->SetParent(nsnull);
    NS_RELEASE(aChild);
    return;
  }
  Layer *lastChild = nsnull;
  for (Layer *child = aContainer->GetFirstChild(); child; 
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

template<class Container>
static void
ContainerDestroy(Container* aContainer)
 {
  if (!aContainer->mDestroyed) {
    while (aContainer->mFirstChild) {
      aContainer->GetFirstChildOGL()->Destroy();
      aContainer->RemoveChild(aContainer->mFirstChild);
    }
    aContainer->mDestroyed = PR_TRUE;
  }
}

template<class Container>
static void
ContainerRender(Container* aContainer,
                int aPreviousFrameBuffer,
                const nsIntPoint& aOffset,
                LayerManagerOGL* aManager)
{
  


  GLuint containerSurface;
  GLuint frameBuffer;

  nsIntPoint childOffset(aOffset);
  nsIntRect visibleRect = aContainer->GetEffectiveVisibleRegion().GetBounds();
  const gfx3DMatrix& transform = aContainer->GetEffectiveTransform();

  aContainer->gl()->PushScissorRect();

  float opacity = aContainer->GetOpacity();
  bool needsFramebuffer = (opacity != 1.0) || !transform.IsIdentity();
  if (needsFramebuffer) {
    aManager->CreateFBOWithTexture(visibleRect.width,
                                   visibleRect.height,
                                   &frameBuffer,
                                   &containerSurface);
    childOffset.x = visibleRect.x;
    childOffset.y = visibleRect.y;

    aContainer->gl()->PushViewportRect();
    aManager->SetupPipeline(visibleRect.width, visibleRect.height);

    aContainer->gl()->fScissor(0, 0, visibleRect.width, visibleRect.height);
    aContainer->gl()->fClearColor(0.0, 0.0, 0.0, 0.0);
    aContainer->gl()->fClear(LOCAL_GL_COLOR_BUFFER_BIT);
  } else {
    frameBuffer = aPreviousFrameBuffer;
  }

  


  LayerOGL *layerToRender = aContainer->GetFirstChildOGL();
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
      aContainer->gl()->FixWindowCoordinateRect(scissorRect, aManager->GetWigetSize().height);
    }

    aManager->gl()->fScissor(scissorRect.x, scissorRect.y, scissorRect.width, scissorRect.height);

    layerToRender->RenderLayer(frameBuffer, childOffset);

    Layer *nextSibling = layerToRender->GetLayer()->GetNextSibling();
    layerToRender = nextSibling ? static_cast<LayerOGL*>(nextSibling->
                                                         ImplData())
                                : nsnull;
  }

  aContainer->gl()->PopScissorRect();

  if (needsFramebuffer) {
    
    
    
    aContainer->gl()->PopViewportRect();
    nsIntRect viewport = aContainer->gl()->ViewportRect();
    aManager->SetupPipeline(viewport.width, viewport.height);

    aContainer->gl()->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, aPreviousFrameBuffer);
    aContainer->gl()->fDeleteFramebuffers(1, &frameBuffer);

    aContainer->gl()->fActiveTexture(LOCAL_GL_TEXTURE0);

    aContainer->gl()->fBindTexture(aManager->FBOTextureTarget(), containerSurface);

    ColorTextureLayerProgram *rgb = aManager->GetFBOLayerProgram();

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

    DEBUG_GL_ERROR_CHECK(aContainer->gl());

    aManager->BindAndDrawQuad(rgb);

    DEBUG_GL_ERROR_CHECK(aContainer->gl());

    
    aContainer->gl()->fDeleteTextures(1, &containerSurface);

    DEBUG_GL_ERROR_CHECK(aContainer->gl());
  }
}

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
ContainerLayerOGL::InsertAfter(Layer* aChild, Layer* aAfter)
{
  ContainerInsertAfter(this, aChild, aAfter);
}

void
ContainerLayerOGL::RemoveChild(Layer *aChild)
{
  ContainerRemoveChild(this, aChild);
}

void
ContainerLayerOGL::Destroy()
{
  ContainerDestroy(this);
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
  ContainerRender(this, aPreviousFrameBuffer, aOffset, mOGLManager);
}


#ifdef MOZ_IPC

ShadowContainerLayerOGL::ShadowContainerLayerOGL(LayerManagerOGL *aManager)
  : ShadowContainerLayer(aManager, NULL)
  , LayerOGL(aManager)
{
  mImplData = static_cast<LayerOGL*>(this);
}
 
ShadowContainerLayerOGL::~ShadowContainerLayerOGL()
{
  Destroy();
}

void
ShadowContainerLayerOGL::InsertAfter(Layer* aChild, Layer* aAfter)
{
  ContainerInsertAfter(this, aChild, aAfter);
}

void
ShadowContainerLayerOGL::RemoveChild(Layer *aChild)
{
  ContainerRemoveChild(this, aChild);
}

void
ShadowContainerLayerOGL::Destroy()
{
  ContainerDestroy(this);
}

LayerOGL*
ShadowContainerLayerOGL::GetFirstChildOGL()
{
  if (!mFirstChild) {
    return nsnull;
   }
  return static_cast<LayerOGL*>(mFirstChild->ImplData());
}
 
void
ShadowContainerLayerOGL::RenderLayer(int aPreviousFrameBuffer,
                                     const nsIntPoint& aOffset)
{
  ContainerRender(this, aPreviousFrameBuffer, aOffset, mOGLManager);
}

#endif  


} 
} 
