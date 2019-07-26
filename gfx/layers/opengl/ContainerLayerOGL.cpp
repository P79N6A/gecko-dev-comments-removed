




#include "ContainerLayerOGL.h"
#include <stdint.h>                     
#include <algorithm>                    
#include "GLContext.h"
#include "gfx3DMatrix.h"                
#include "gfxMatrix.h"                  
#include "gfxPlatform.h"                
#include "gfxUtils.h"                   
#include "mozilla/gfx/BaseRect.h"       
#include "mozilla/layers/CompositorTypes.h"  
#include "nsAutoPtr.h"                  
#include "nsDebug.h"                    
#include "nsISupportsUtils.h"           
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsTArray.h"                   
#include "LayerManagerOGL.h"            
#include "LayerManagerOGLProgram.h"     
class gfxImageSurface;

namespace mozilla {
namespace layers {

static inline LayerOGL*
GetNextSibling(LayerOGL* aLayer)
{
   Layer* layer = aLayer->GetLayer()->GetNextSibling();
   return layer ? static_cast<LayerOGL*>(layer->
                                         ImplData())
                 : nullptr;
}

ContainerLayerOGL::ContainerLayerOGL(LayerManagerOGL *mOGLManager)
  : ContainerLayer(mOGLManager, nullptr)
  , LayerOGL(mOGLManager)
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
    mDestroyed = true;
  }
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
  


  GLuint containerSurface;
  GLuint frameBuffer;

  nsIntPoint childOffset(aOffset);
  nsIntRect visibleRect = GetEffectiveVisibleRegion().GetBounds();

  nsIntRect cachedScissor = gl()->ScissorRect();
  gl()->PushScissorRect();
  mSupportsComponentAlphaChildren = false;

  float opacity = GetEffectiveOpacity();
  const gfx3DMatrix& transform = GetEffectiveTransform();
  bool needsFramebuffer = UseIntermediateSurface();
  if (needsFramebuffer) {
    nsIntRect framebufferRect = visibleRect;
    
    
    
    
    
    
    GLint maxTexSize;
    gl()->fGetIntegerv(LOCAL_GL_MAX_TEXTURE_SIZE, &maxTexSize);
    framebufferRect.width = std::min(framebufferRect.width, maxTexSize);
    framebufferRect.height = std::min(framebufferRect.height, maxTexSize);

    LayerManagerOGL::InitMode mode = LayerManagerOGL::InitModeClear;
    if (GetEffectiveVisibleRegion().GetNumRects() == 1 && 
        (GetContentFlags() & Layer::CONTENT_OPAQUE))
    {
      
      mSupportsComponentAlphaChildren = true;
      mode = LayerManagerOGL::InitModeNone;
    } else {
      const gfx3DMatrix& transform3D = GetEffectiveTransform();
      gfxMatrix transform;
      
      
      
      
      if (HasOpaqueAncestorLayer(this) &&
          transform3D.Is2D(&transform) && !transform.HasNonIntegerTranslation()) {
        mode = gfxPlatform::ComponentAlphaEnabled() ?
          LayerManagerOGL::InitModeCopy :
          LayerManagerOGL::InitModeClear;
        framebufferRect.x += transform.x0;
        framebufferRect.y += transform.y0;
        mSupportsComponentAlphaChildren = gfxPlatform::ComponentAlphaEnabled();
      }
    }

    gl()->PushViewportRect();
    framebufferRect -= childOffset;
    if (!mOGLManager->CompositingDisabled()) {
      if (!mOGLManager->CreateFBOWithTexture(framebufferRect,
                                          mode,
                                          aPreviousFrameBuffer,
                                          &frameBuffer,
                                          &containerSurface)) {
        gl()->PopViewportRect();
        gl()->PopScissorRect();
        gl()->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, aPreviousFrameBuffer);
        return;
      }
    }
    childOffset.x = visibleRect.x;
    childOffset.y = visibleRect.y;
  } else {
    frameBuffer = aPreviousFrameBuffer;
    mSupportsComponentAlphaChildren = (GetContentFlags() & Layer::CONTENT_OPAQUE) ||
      (GetParent() && GetParent()->SupportsComponentAlphaChildren());
  }

  nsAutoTArray<Layer*, 12> children;
  SortChildrenBy3DZOrder(children);

  


  for (uint32_t i = 0; i < children.Length(); i++) {
    LayerOGL* layerToRender = static_cast<LayerOGL*>(children.ElementAt(i)->ImplData());

    if (layerToRender->GetLayer()->GetEffectiveVisibleRegion().IsEmpty()) {
      continue;
    }

    nsIntRect scissorRect = layerToRender->GetLayer()->
        CalculateScissorRect(cachedScissor, &mOGLManager->GetWorldTransform());
    if (scissorRect.IsEmpty()) {
      continue;
    }

    gl()->fScissor(scissorRect.x, 
                               scissorRect.y, 
                               scissorRect.width, 
                               scissorRect.height);

    layerToRender->RenderLayer(frameBuffer, childOffset);
    gl()->MakeCurrent();
  }


  if (needsFramebuffer) {
    
#ifdef MOZ_DUMP_PAINTING
    if (gfxUtils::sDumpPainting) {
      nsRefPtr<gfxImageSurface> surf = 
        gl()->GetTexImage(containerSurface, true, mOGLManager->GetFBOTextureFormat());

      WriteSnapshotToDumpFile(this, surf);
    }
#endif
    
    
    gl()->PopViewportRect();
    nsIntRect viewport = gl()->ViewportRect();
    mOGLManager->SetupPipeline(viewport.width, viewport.height,
                            LayerManagerOGL::ApplyWorldTransform);
    gl()->PopScissorRect();

    if (!mOGLManager->CompositingDisabled()) {
      gl()->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, aPreviousFrameBuffer);
      gl()->fDeleteFramebuffers(1, &frameBuffer);

      gl()->fActiveTexture(LOCAL_GL_TEXTURE0);

      gl()->fBindTexture(mOGLManager->FBOTextureTarget(), containerSurface);

      MaskType maskType = MaskNone;
      if (GetMaskLayer()) {
        if (!GetTransform().CanDraw2D()) {
          maskType = Mask3d;
        } else {
          maskType = Mask2d;
        }
      }
      ShaderProgramOGL *rgb =
        mOGLManager->GetFBOLayerProgram(maskType);

      rgb->Activate();
      rgb->SetProjectionMatrix(mOGLManager->mProjMatrix);
      rgb->SetLayerQuadRect(visibleRect);
      rgb->SetLayerTransform(transform);
      rgb->SetTextureTransform(gfx3DMatrix());
      rgb->SetLayerOpacity(opacity);
      rgb->SetRenderOffset(aOffset);
      rgb->SetTextureUnit(0);
      rgb->LoadMask(GetMaskLayer());

      if (rgb->HasTexCoordMultiplier()) {
        rgb->SetTexCoordMultiplier(visibleRect.width, visibleRect.height);
      }

      
      
      
      mOGLManager->BindAndDrawQuad(rgb, true);

      
      gl()->fDeleteTextures(1, &containerSurface);
    }
  } else {
    gl()->PopScissorRect();
  }
}

void
ContainerLayerOGL::CleanupResources()
{
  for (Layer* l = GetFirstChild(); l; l = l->GetNextSibling()) {
    LayerOGL* layerToRender = static_cast<LayerOGL*>(l->ImplData());
    layerToRender->CleanupResources();
  }
}

} 
} 
