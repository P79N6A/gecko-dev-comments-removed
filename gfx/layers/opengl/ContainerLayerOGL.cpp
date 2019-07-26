




#include "ContainerLayerOGL.h"
#include "gfxUtils.h"
#include "gfxPlatform.h"

namespace mozilla {
namespace layers {

template<class Container>
static void
ContainerInsertAfter(Container* aContainer, Layer* aChild, Layer* aAfter)
{
  NS_ASSERTION(aChild->Manager() == aContainer->Manager(),
               "Child has wrong manager");
  NS_ASSERTION(!aChild->GetParent(),
               "aChild already in the tree");
  NS_ASSERTION(!aChild->GetNextSibling() && !aChild->GetPrevSibling(),
               "aChild already has siblings?");
  NS_ASSERTION(!aAfter ||
               (aAfter->Manager() == aContainer->Manager() &&
                aAfter->GetParent() == aContainer),
               "aAfter is not our child");

  aChild->SetParent(aContainer);
  if (aAfter == aContainer->mLastChild) {
    aContainer->mLastChild = aChild;
  }
  if (!aAfter) {
    aChild->SetNextSibling(aContainer->mFirstChild);
    if (aContainer->mFirstChild) {
      aContainer->mFirstChild->SetPrevSibling(aChild);
    }
    aContainer->mFirstChild = aChild;
    NS_ADDREF(aChild);
    aContainer->DidInsertChild(aChild);
    return;
  }

  Layer* next = aAfter->GetNextSibling();
  aChild->SetNextSibling(next);
  aChild->SetPrevSibling(aAfter);
  if (next) {
    next->SetPrevSibling(aChild);
  }
  aAfter->SetNextSibling(aChild);
  NS_ADDREF(aChild);
  aContainer->DidInsertChild(aChild);
}

template<class Container>
static void
ContainerRemoveChild(Container* aContainer, Layer* aChild)
{
  NS_ASSERTION(aChild->Manager() == aContainer->Manager(),
               "Child has wrong manager");
  NS_ASSERTION(aChild->GetParent() == aContainer,
               "aChild not our child");

  Layer* prev = aChild->GetPrevSibling();
  Layer* next = aChild->GetNextSibling();
  if (prev) {
    prev->SetNextSibling(next);
  } else {
    aContainer->mFirstChild = next;
  }
  if (next) {
    next->SetPrevSibling(prev);
  } else {
    aContainer->mLastChild = prev;
  }

  aChild->SetNextSibling(nullptr);
  aChild->SetPrevSibling(nullptr);
  aChild->SetParent(nullptr);

  aContainer->DidRemoveChild(aChild);
  NS_RELEASE(aChild);
}

template<class Container>
static void
ContainerRepositionChild(Container* aContainer, Layer* aChild, Layer* aAfter)
{
  NS_ASSERTION(aChild->Manager() == aContainer->Manager(),
               "Child has wrong manager");
  NS_ASSERTION(aChild->GetParent() == aContainer,
               "aChild not our child");
  NS_ASSERTION(!aAfter ||
               (aAfter->Manager() == aContainer->Manager() &&
                aAfter->GetParent() == aContainer),
               "aAfter is not our child");

  Layer* prev = aChild->GetPrevSibling();
  Layer* next = aChild->GetNextSibling();
  if (prev == aAfter) {
    
    return;
  }
  if (prev) {
    prev->SetNextSibling(next);
  }
  if (next) {
    next->SetPrevSibling(prev);
  }
  if (!aAfter) {
    aChild->SetPrevSibling(nullptr);
    aChild->SetNextSibling(aContainer->mFirstChild);
    if (aContainer->mFirstChild) {
      aContainer->mFirstChild->SetPrevSibling(aChild);
    }
    aContainer->mFirstChild = aChild;
    return;
  }

  Layer* afterNext = aAfter->GetNextSibling();
  if (afterNext) {
    afterNext->SetPrevSibling(aChild);
  } else {
    aContainer->mLastChild = aChild;
  }
  aAfter->SetNextSibling(aChild);
  aChild->SetPrevSibling(aAfter);
  aChild->SetNextSibling(afterNext);
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
    aContainer->mDestroyed = true;
  }
}

template<class Container>
static void
ContainerCleanupResources(Container* aContainer)
{
  for (Layer* l = aContainer->GetFirstChild(); l; l = l->GetNextSibling()) {
    LayerOGL* layerToRender = static_cast<LayerOGL*>(l->ImplData());
    layerToRender->CleanupResources();
  }
}

static inline LayerOGL*
GetNextSibling(LayerOGL* aLayer)
{
   Layer* layer = aLayer->GetLayer()->GetNextSibling();
   return layer ? static_cast<LayerOGL*>(layer->
                                         ImplData())
                 : nullptr;
}

static bool
HasOpaqueAncestorLayer(Layer* aLayer)
{
  for (Layer* l = aLayer->GetParent(); l; l = l->GetParent()) {
    if (l->GetContentFlags() & Layer::CONTENT_OPAQUE)
      return true;
  }
  return false;
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

  nsIntRect cachedScissor = aContainer->gl()->ScissorRect();
  aContainer->gl()->PushScissorRect();
  aContainer->mSupportsComponentAlphaChildren = false;

  float opacity = aContainer->GetEffectiveOpacity();
  const gfx3DMatrix& transform = aContainer->GetEffectiveTransform();
  bool needsFramebuffer = aContainer->UseIntermediateSurface();
  if (needsFramebuffer) {
    nsIntRect framebufferRect = visibleRect;
    
    
    
    
    
    
    GLint maxTexSize;
    aContainer->gl()->fGetIntegerv(LOCAL_GL_MAX_TEXTURE_SIZE, &maxTexSize);
    framebufferRect.width = std::min(framebufferRect.width, maxTexSize);
    framebufferRect.height = std::min(framebufferRect.height, maxTexSize);

    LayerManagerOGL::InitMode mode = LayerManagerOGL::InitModeClear;
    if (aContainer->GetEffectiveVisibleRegion().GetNumRects() == 1 && 
        (aContainer->GetContentFlags() & Layer::CONTENT_OPAQUE))
    {
      
      aContainer->mSupportsComponentAlphaChildren = true;
      mode = LayerManagerOGL::InitModeNone;
    } else {
      const gfx3DMatrix& transform3D = aContainer->GetEffectiveTransform();
      gfxMatrix transform;
      
      
      
      
      if (HasOpaqueAncestorLayer(aContainer) &&
          transform3D.Is2D(&transform) && !transform.HasNonIntegerTranslation()) {
        mode = gfxPlatform::GetPlatform()->UsesSubpixelAATextRendering() ?
		LayerManagerOGL::InitModeCopy :
		LayerManagerOGL::InitModeClear;
        framebufferRect.x += transform.x0;
        framebufferRect.y += transform.y0;
        aContainer->mSupportsComponentAlphaChildren = gfxPlatform::GetPlatform()->UsesSubpixelAATextRendering();
      }
    }

    aContainer->gl()->PushViewportRect();
    framebufferRect -= childOffset;
    if (!aManager->CompositingDisabled()) {
      aManager->CreateFBOWithTexture(framebufferRect,
                                     mode,
                                     aPreviousFrameBuffer,
                                     &frameBuffer,
                                     &containerSurface);
    }
    childOffset.x = visibleRect.x;
    childOffset.y = visibleRect.y;
  } else {
    frameBuffer = aPreviousFrameBuffer;
    aContainer->mSupportsComponentAlphaChildren = (aContainer->GetContentFlags() & Layer::CONTENT_OPAQUE) ||
      (aContainer->GetParent() && aContainer->GetParent()->SupportsComponentAlphaChildren());
  }

  nsAutoTArray<Layer*, 12> children;
  aContainer->SortChildrenBy3DZOrder(children);

  


  for (uint32_t i = 0; i < children.Length(); i++) {
    LayerOGL* layerToRender = static_cast<LayerOGL*>(children.ElementAt(i)->ImplData());

    if (layerToRender->GetLayer()->GetEffectiveVisibleRegion().IsEmpty()) {
      continue;
    }

    nsIntRect scissorRect = layerToRender->GetLayer()->
        CalculateScissorRect(cachedScissor, &aManager->GetWorldTransform());
    if (scissorRect.IsEmpty()) {
      continue;
    }

    aContainer->gl()->fScissor(scissorRect.x, 
                               scissorRect.y, 
                               scissorRect.width, 
                               scissorRect.height);

    layerToRender->RenderLayer(frameBuffer, childOffset);
    aContainer->gl()->MakeCurrent();
  }


  if (needsFramebuffer) {
    
#ifdef MOZ_DUMP_PAINTING
    if (gfxUtils::sDumpPainting) {
      nsRefPtr<gfxImageSurface> surf = 
        aContainer->gl()->GetTexImage(containerSurface, true, aManager->GetFBOLayerProgramType());

      WriteSnapshotToDumpFile(aContainer, surf);
    }
#endif
    
    
    aContainer->gl()->PopViewportRect();
    nsIntRect viewport = aContainer->gl()->ViewportRect();
    aManager->SetupPipeline(viewport.width, viewport.height,
                            LayerManagerOGL::ApplyWorldTransform);
    aContainer->gl()->PopScissorRect();
    aContainer->gl()->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, aPreviousFrameBuffer);

    if (!aManager->CompositingDisabled()) {
      aContainer->gl()->fDeleteFramebuffers(1, &frameBuffer);

      aContainer->gl()->fActiveTexture(LOCAL_GL_TEXTURE0);

      aContainer->gl()->fBindTexture(aManager->FBOTextureTarget(), containerSurface);

      MaskType maskType = MaskNone;
      if (aContainer->GetMaskLayer()) {
        if (!aContainer->GetTransform().CanDraw2D()) {
          maskType = Mask3d;
        } else {
          maskType = Mask2d;
        }
      }
      ShaderProgramOGL *rgb =
        aManager->GetFBOLayerProgram(maskType);

      rgb->Activate();
      rgb->SetLayerQuadRect(visibleRect);
      rgb->SetLayerTransform(transform);
      rgb->SetLayerOpacity(opacity);
      rgb->SetRenderOffset(aOffset);
      rgb->SetTextureUnit(0);
      rgb->LoadMask(aContainer->GetMaskLayer());

      if (rgb->GetTexCoordMultiplierUniformLocation() != -1) {
        
        rgb->SetTexCoordMultiplier(visibleRect.width, visibleRect.height);
      }

      
      
      
      aManager->BindAndDrawQuad(rgb, true);

      
      aContainer->gl()->fDeleteTextures(1, &containerSurface);
    }
  } else {
    aContainer->gl()->PopScissorRect();
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
ContainerLayerOGL::RepositionChild(Layer* aChild, Layer* aAfter)
{
  ContainerRepositionChild(this, aChild, aAfter);
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
    return nullptr;
  }
  return static_cast<LayerOGL*>(mFirstChild->ImplData());
}

void
ContainerLayerOGL::RenderLayer(int aPreviousFrameBuffer,
                               const nsIntPoint& aOffset)
{
  ContainerRender(this, aPreviousFrameBuffer, aOffset, mOGLManager);
}

void
ContainerLayerOGL::CleanupResources()
{
  ContainerCleanupResources(this);
}

ShadowContainerLayerOGL::ShadowContainerLayerOGL(LayerManagerOGL *aManager)
  : ShadowContainerLayer(aManager, NULL)
  , LayerOGL(aManager)
{
  mImplData = static_cast<LayerOGL*>(this);
}
 
ShadowContainerLayerOGL::~ShadowContainerLayerOGL()
{
  
  
  
  
  
  
  
  
  
  while (mFirstChild) {
    ContainerRemoveChild(this, mFirstChild);
  }
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
ShadowContainerLayerOGL::RepositionChild(Layer* aChild, Layer* aAfter)
{
  ContainerRepositionChild(this, aChild, aAfter);
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
    return nullptr;
   }
  return static_cast<LayerOGL*>(mFirstChild->ImplData());
}
 
void
ShadowContainerLayerOGL::RenderLayer(int aPreviousFrameBuffer,
                                     const nsIntPoint& aOffset)
{
  ContainerRender(this, aPreviousFrameBuffer, aOffset, mOGLManager);
}

void
ShadowContainerLayerOGL::CleanupResources()
{
  ContainerCleanupResources(this);
}

ShadowRefLayerOGL::ShadowRefLayerOGL(LayerManagerOGL* aManager)
  : ShadowRefLayer(aManager, NULL)
  , LayerOGL(aManager)
{
  mImplData = static_cast<LayerOGL*>(this);
}

ShadowRefLayerOGL::~ShadowRefLayerOGL()
{
  Destroy();
}

void
ShadowRefLayerOGL::Destroy()
{
  MOZ_ASSERT(!mFirstChild);
  mDestroyed = true;
}

LayerOGL*
ShadowRefLayerOGL::GetFirstChildOGL()
{
  if (!mFirstChild) {
    return nullptr;
   }
  return static_cast<LayerOGL*>(mFirstChild->ImplData());
}

void
ShadowRefLayerOGL::RenderLayer(int aPreviousFrameBuffer,
                               const nsIntPoint& aOffset)
{
  ContainerRender(this, aPreviousFrameBuffer, aOffset, mOGLManager);
}

void
ShadowRefLayerOGL::CleanupResources()
{
  MOZ_ASSERT(!mFirstChild);
}

} 
} 
