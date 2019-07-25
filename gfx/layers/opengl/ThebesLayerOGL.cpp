





































#include "ThebesLayerBuffer.h"
#include "ThebesLayerOGL.h"

namespace mozilla {
namespace layers {

using gl::GLContext;
using gl::TextureImage;







static void
BindAndDrawQuadWithTextureRect(LayerProgram *aProg,
                               const nsIntRect& aTexCoordRect,
                               GLContext* aGl)
{
  GLuint vertAttribIndex =
    aProg->AttribLocation(LayerProgram::VertexAttrib);
  GLuint texCoordAttribIndex =
    aProg->AttribLocation(LayerProgram::TexCoordAttrib);
  NS_ASSERTION(texCoordAttribIndex != GLuint(-1), "no texture coords?");

  
  
  aGl->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, 0);

  
  GLfloat quadVertices[] = {
    0.0f, 0.0f,                 
    1.0f, 0.0f,                 
    0.0f, 1.0f,                 
    1.0f, 1.0f                  
  };
  aGl->fVertexAttribPointer(vertAttribIndex, 2,
                                   LOCAL_GL_FLOAT, LOCAL_GL_FALSE, 0,
                                   quadVertices);
  DEBUG_GL_ERROR_CHECK(aGl);

  GLfloat w(aTexCoordRect.width), h(aTexCoordRect.height);
  GLfloat xleft = GLfloat(aTexCoordRect.x) / w;
  GLfloat ytop = GLfloat(aTexCoordRect.y) / h;
  GLfloat texCoords[] = {
    xleft,         ytop,
    1.0f + xleft,  ytop,
    xleft,         1.0f + ytop,
    1.0f + xleft,  1.0f + ytop,
  };

  aGl->fVertexAttribPointer(texCoordAttribIndex, 2,
                            LOCAL_GL_FLOAT, LOCAL_GL_FALSE, 0,
                            texCoords);
  DEBUG_GL_ERROR_CHECK(aGl);

  {
    aGl->fEnableVertexAttribArray(texCoordAttribIndex);
    {
      aGl->fEnableVertexAttribArray(vertAttribIndex);

      aGl->fDrawArrays(LOCAL_GL_TRIANGLE_STRIP, 0, 4);
      DEBUG_GL_ERROR_CHECK(aGl);

      aGl->fDisableVertexAttribArray(vertAttribIndex);
    }
    aGl->fDisableVertexAttribArray(texCoordAttribIndex);
  }

  DEBUG_GL_ERROR_CHECK(aGl);
}


class ThebesLayerBufferOGL
{
  NS_INLINE_DECL_REFCOUNTING(ThebesLayerBufferOGL)
public:
  typedef TextureImage::ContentType ContentType;
  typedef ThebesLayerBuffer::PaintState PaintState;

  ThebesLayerBufferOGL(ThebesLayerOGL* aLayer, TextureImage* aTexImage)
    : mLayer(aLayer)
    , mTexImage(aTexImage)
  {}
  virtual ~ThebesLayerBufferOGL() {}

  virtual PaintState BeginPaint(ContentType aContentType) = 0;

  void RenderTo(const nsIntPoint& aOffset, LayerManagerOGL* aManager);

protected:
  virtual nsIntRect GetTexCoordRectForRepeat() = 0;

  GLContext* gl() const { return mLayer->gl(); }

  ThebesLayerOGL* mLayer;
  nsRefPtr<TextureImage> mTexImage;
};

void
ThebesLayerBufferOGL::RenderTo(const nsIntPoint& aOffset,
                               LayerManagerOGL* aManager)
{
  
  
  ColorTextureLayerProgram *program =
    mLayer->CanUseOpaqueSurface()
    ? aManager->GetBGRXLayerProgram()
    : aManager->GetBGRALayerProgram();

  if (!mTexImage->InUpdate() || !mTexImage->EndUpdate()) {
    gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexImage->Texture());
  }

  nsIntRect quadRect = mLayer->GetVisibleRegion().GetBounds();
  program->Activate();
  program->SetLayerQuadRect(quadRect);
  program->SetLayerOpacity(mLayer->GetOpacity());
  program->SetLayerTransform(mLayer->GetTransform());
  program->SetRenderOffset(aOffset);
  program->SetTextureUnit(0);
  DEBUG_GL_ERROR_CHECK(gl());

  nsIntRect texCoordRect = GetTexCoordRectForRepeat();
  BindAndDrawQuadWithTextureRect(program, texCoordRect, gl());
  DEBUG_GL_ERROR_CHECK(gl());
}





class SurfaceBufferOGL : public ThebesLayerBufferOGL, private ThebesLayerBuffer
{
public:
  typedef ThebesLayerBufferOGL::ContentType ContentType;
  typedef ThebesLayerBufferOGL::PaintState PaintState;

  SurfaceBufferOGL(ThebesLayerOGL* aLayer, TextureImage* aTexImage)
    : ThebesLayerBufferOGL(aLayer, aTexImage)
    , ThebesLayerBuffer(SizedToVisibleBounds)
  {
    mTmpSurface = mTexImage->GetBackingSurface();
    NS_ABORT_IF_FALSE(mTmpSurface, "SurfaceBuffer without backing surface??");
  }
  virtual ~SurfaceBufferOGL() {}

  
  virtual PaintState BeginPaint(ContentType aContentType)
  {
    
    return ThebesLayerBuffer::BeginPaint(mLayer, aContentType, 1.0, 1.0);
  }

  
  virtual already_AddRefed<gfxASurface>
  CreateBuffer(ContentType aType, const nsIntSize& aSize)
  {
    NS_ASSERTION(gfxASurface::CONTENT_ALPHA != aType,"ThebesBuffer has color");

    if (mTmpSurface)
    {
      NS_ASSERTION(aSize == mTexImage->GetSize(),
                   "initial TextureImage is the wrong size");
      NS_ASSERTION(aType == mTexImage->GetContentType(),
                   "initial TextureImage has the wrong content type");
      
      
      
      
      return mTmpSurface.forget();
    }

    mTexImage = gl()->CreateTextureImage(aSize, aType, LOCAL_GL_REPEAT);
    return mTexImage ? mTexImage->GetBackingSurface() : nsnull;
  }

protected:
  virtual nsIntRect
  GetTexCoordRectForRepeat()
  {
    
    
    return nsIntRect(BufferRotation(), BufferRect().Size());
  }

private:
  nsRefPtr<gfxASurface> mTmpSurface;
};






class BasicBufferOGL : public ThebesLayerBufferOGL
{
public:
  BasicBufferOGL(ThebesLayerOGL* aLayer, TextureImage* aTexImage)
    : ThebesLayerBufferOGL(aLayer, aTexImage)
  {}
  virtual ~BasicBufferOGL() {}

  virtual PaintState BeginPaint(ContentType aContentType);

protected:
  virtual nsIntRect
  GetTexCoordRectForRepeat()
  {
    
    return nsIntRect(nsIntPoint(0, 0), mBufferRect.Size());
  }

private:
  nsIntRect mBufferRect;
};

BasicBufferOGL::PaintState
BasicBufferOGL::BeginPaint(ContentType aContentType)
{
  PaintState state;
  nsIntRect visibleRect = mLayer->GetVisibleRegion().GetBounds();

  if (aContentType != mTexImage->GetContentType() ||
      visibleRect.Size() != mTexImage->GetSize())
  {
    mBufferRect = nsIntRect();
    mTexImage = gl()->CreateTextureImage(visibleRect.Size(), aContentType,
                                         LOCAL_GL_REPEAT);
    DEBUG_GL_ERROR_CHECK(gl());
    if (!mTexImage) {
      return state;
    }
  }
  NS_ABORT_IF_FALSE((mTexImage->GetContentType() == aContentType &&
                     mTexImage->GetSize() == visibleRect.Size()),
                    "TextureImage matches layer attributes");

  state.mRegionToDraw = mLayer->GetVisibleRegion();
  if (mBufferRect != visibleRect) {
    
    state.mRegionToInvalidate = mLayer->GetValidRegion();
    mBufferRect = visibleRect;
  } else {
    state.mRegionToDraw.Sub(state.mRegionToDraw, mLayer->GetValidRegion());
  }
  if (state.mRegionToDraw.IsEmpty()) {
    return state;
  }

  
  
  
  
  
  
  state.mRegionToDraw.MoveBy(-visibleRect.TopLeft());

  
  
  state.mContext = mTexImage->BeginUpdate(state.mRegionToDraw);
  if (!state.mContext) {
    NS_WARNING("unable to get context for update");
    return state;
  }

  
  
  state.mRegionToDraw.MoveBy(visibleRect.TopLeft());

  
  
  state.mContext->Translate(-gfxPoint(visibleRect.x, visibleRect.y));

  
  if (gfxASurface::CONTENT_COLOR_ALPHA == aContentType) {
    state.mContext->SetOperator(gfxContext::OPERATOR_CLEAR);
    state.mContext->Paint();
    state.mContext->SetOperator(gfxContext::OPERATOR_OVER);
  }
  return state;
}


ThebesLayerOGL::ThebesLayerOGL(LayerManagerOGL *aManager)
  : ThebesLayer(aManager, nsnull)
  , LayerOGL(aManager)
  , mBuffer(nsnull)
{
  mImplData = static_cast<LayerOGL*>(this);
}

ThebesLayerOGL::~ThebesLayerOGL()
{
  Destroy();
}

void
ThebesLayerOGL::Destroy()
{
  if (!mDestroyed) {
    mBuffer = nsnull;
    DEBUG_GL_ERROR_CHECK(gl());

    mDestroyed = PR_TRUE;
  }
}

PRBool
ThebesLayerOGL::CreateSurface()
{
  NS_ASSERTION(!mBuffer, "buffer already created?");

  if (mVisibleRegion.IsEmpty()) {
    return PR_FALSE;
  }

  nsIntSize visibleSize = mVisibleRegion.GetBounds().Size();
  TextureImage::ContentType contentType =
    CanUseOpaqueSurface() ? gfxASurface::CONTENT_COLOR :
                            gfxASurface::CONTENT_COLOR_ALPHA;
  nsRefPtr<TextureImage> teximage(
    gl()->CreateTextureImage(visibleSize, contentType,
                             LOCAL_GL_CLAMP_TO_EDGE));
  if (!teximage) {
    return PR_FALSE;
  }

  nsRefPtr<gfxASurface> surf = teximage->GetBackingSurface();
  if (surf) {
    
    mBuffer = new SurfaceBufferOGL(this, teximage);
  } else {
    mBuffer = new BasicBufferOGL(this, teximage);
  }
  return PR_TRUE;
}

void
ThebesLayerOGL::SetVisibleRegion(const nsIntRegion &aRegion)
{
  if (aRegion.IsEqual(mVisibleRegion))
    return;
  ThebesLayer::SetVisibleRegion(aRegion);
}

void
ThebesLayerOGL::InvalidateRegion(const nsIntRegion &aRegion)
{
  mValidRegion.Sub(mValidRegion, aRegion);
}

void
ThebesLayerOGL::RenderLayer(int ,
                            const nsIntPoint& aOffset)
{
  if (!mBuffer && !CreateSurface()) {
    return;
  }
  NS_ABORT_IF_FALSE(mBuffer, "should have a buffer here");

  mOGLManager->MakeCurrent();
  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);

  TextureImage::ContentType contentType =
    CanUseOpaqueSurface() ? gfxASurface::CONTENT_COLOR :
                            gfxASurface::CONTENT_COLOR_ALPHA;
  Buffer::PaintState state = mBuffer->BeginPaint(contentType);
  mValidRegion.Sub(mValidRegion, state.mRegionToInvalidate);

  if (state.mContext) {
    state.mRegionToInvalidate.And(state.mRegionToInvalidate, mVisibleRegion);

    LayerManager::DrawThebesLayerCallback callback =
      mOGLManager->GetThebesLayerCallback();
    void* callbackData = mOGLManager->GetThebesLayerCallbackData();
    callback(this, state.mContext, state.mRegionToDraw,
             state.mRegionToInvalidate, callbackData);
    mValidRegion.Or(mValidRegion, state.mRegionToDraw);
  }

  mBuffer->RenderTo(aOffset, mOGLManager);
  DEBUG_GL_ERROR_CHECK(gl());
}

Layer*
ThebesLayerOGL::GetLayer()
{
  return this;
}

PRBool
ThebesLayerOGL::IsEmpty()
{
  return !mBuffer;
}

} 
} 
