




































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

static inline GLint GetYCoordOfRectStartingFromBottom(GLint y, GLint height, GLint viewportHeight)
{
#ifdef XP_MACOSX
    (void) height;
    (void) viewportHeight;
    return y;
#else
    return viewportHeight - height - y;
#endif
}

void
ContainerLayerOGL::RenderLayer(int aPreviousFrameBuffer,
                               const nsIntPoint& aOffset)
{
  


  GLuint containerSurface;
  GLuint frameBuffer;

  nsIntPoint childOffset(aOffset);
  nsIntRect visibleRect = mVisibleRegion.GetBounds();

  GLint savedScissor[4];
  gl()->fGetIntegerv(LOCAL_GL_SCISSOR_BOX, savedScissor);

  float opacity = GetOpacity();
  bool needsFramebuffer = (opacity != 1.0) || !mTransform.IsIdentity();
  if (needsFramebuffer) {
    mOGLManager->CreateFBOWithTexture(visibleRect.width,
                                      visibleRect.height,
                                      &frameBuffer,
                                      &containerSurface);
    childOffset.x = visibleRect.x;
    childOffset.y = visibleRect.y;
    mOGLManager->gl()->fScissor(0, 0, visibleRect.width, visibleRect.height);
    mOGLManager->gl()->fClearColor(0.0, 0.0, 0.0, 0.0);
    mOGLManager->gl()->fClear(LOCAL_GL_COLOR_BUFFER_BIT);
  } else {
    frameBuffer = aPreviousFrameBuffer;
  }

  GLint viewport[4];
  gl()->fGetIntegerv(LOCAL_GL_VIEWPORT, viewport);

  


  LayerOGL *layerToRender = GetFirstChildOGL();
  while (layerToRender) {
    const nsIntRect *clipRect = layerToRender->GetLayer()->GetClipRect();
    if (clipRect) {
      if (needsFramebuffer) {
        gl()->fScissor(clipRect->x - visibleRect.x,
                       GetYCoordOfRectStartingFromBottom(clipRect->y - visibleRect.y, clipRect->height, viewport[3]),
                       clipRect->width,
                       clipRect->height);
      } else {
        gl()->fScissor(clipRect->x,
                       GetYCoordOfRectStartingFromBottom(clipRect->y, clipRect->height, viewport[3]),
                       clipRect->width,
                       clipRect->height);
      }
    } else {
      if (needsFramebuffer) {
        gl()->fScissor(0, 0, visibleRect.width, visibleRect.height);
      } else {
        gl()->fScissor(visibleRect.x,
                       GetYCoordOfRectStartingFromBottom(visibleRect.y, visibleRect.height, viewport[3]),
                       visibleRect.width,
                       visibleRect.height);
      }
    }

    layerToRender->RenderLayer(frameBuffer, childOffset);

    Layer *nextSibling = layerToRender->GetLayer()->GetNextSibling();
    layerToRender = nextSibling ? static_cast<LayerOGL*>(nextSibling->
                                                         ImplData())
                                : nsnull;
  }

  gl()->fScissor(savedScissor[0], savedScissor[1], savedScissor[2], savedScissor[3]);

  if (needsFramebuffer) {
    
    gl()->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, aPreviousFrameBuffer);
    gl()->fDeleteFramebuffers(1, &frameBuffer);

    gl()->fActiveTexture(LOCAL_GL_TEXTURE0);

    gl()->fBindTexture(mOGLManager->FBOTextureTarget(), containerSurface);

    ColorTextureLayerProgram *rgb = mOGLManager->GetFBOLayerProgram();

    rgb->Activate();
    rgb->SetLayerQuadRect(visibleRect);
    rgb->SetLayerTransform(mTransform);
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
