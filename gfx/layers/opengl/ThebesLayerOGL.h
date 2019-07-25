




































#ifndef GFX_THEBESLAYEROGL_H
#define GFX_THEBESLAYEROGL_H

#include "mozilla/layers/PLayers.h"
#include "mozilla/layers/ShadowLayers.h"

#include "Layers.h"
#include "LayerManagerOGL.h"
#include "gfxImageSurface.h"
#include "GLContext.h"
#include "base/task.h"


namespace mozilla {
namespace layers {

class ThebesLayerBufferOGL;
class BasicBufferOGL;
class ShadowBufferOGL;

class ThebesLayerOGL : public ThebesLayer, 
                       public LayerOGL
{
  typedef ThebesLayerBufferOGL Buffer;

public:
  ThebesLayerOGL(LayerManagerOGL *aManager);
  virtual ~ThebesLayerOGL();

  
  void SetVisibleRegion(const nsIntRegion& aRegion);

  
  void InvalidateRegion(const nsIntRegion& aRegion);

  
  void Destroy();
  Layer* GetLayer();
  virtual bool IsEmpty();
  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);
  virtual void CleanupResources();

private:
  friend class BasicBufferOGL;

  bool CreateSurface();

  nsRefPtr<Buffer> mBuffer;
};

class ShadowThebesLayerBufferOGL
{
public:
  ShadowThebesLayerBufferOGL()
  {
    MOZ_COUNT_CTOR(ShadowThebesLayerBufferOGL);
  }

  ~ShadowThebesLayerBufferOGL()
  {
    MOZ_COUNT_DTOR(ShadowThebesLayerBufferOGL);
  }

  void Swap(gfxASurface* aNewBuffer,
            const nsIntRect& aNewRect, const nsIntPoint& aNewRotation,
            gfxASurface** aOldBuffer,
            nsIntRect* aOldRect, nsIntPoint* aOldRotation)
  {
    *aOldRect = mBufferRect;
    *aOldRotation = mBufferRotation;
    nsRefPtr<gfxASurface> oldBuffer = mBuffer;

    mBufferRect = aNewRect;
    mBufferRotation = aNewRotation;
    mBuffer = aNewBuffer;
    oldBuffer.forget(aOldBuffer);
  }

  nsIntRect Rect() {
    return mBufferRect;
  }

  nsIntPoint Rotation() {
    return mBufferRotation;
  }

  nsRefPtr<gfxASurface> Buffer() {
    return mBuffer;
  }

  



  void Clear()
  {
    mBuffer = nsnull;
    mBufferRect.SetEmpty();
  }

protected:
  nsRefPtr<gfxASurface> mBuffer;
  nsIntRect mBufferRect;
  nsIntPoint mBufferRotation;
};

class ShadowThebesLayerOGL : public ShadowThebesLayer,
                             public LayerOGL
{
public:
  ShadowThebesLayerOGL(LayerManagerOGL *aManager);
  virtual ~ShadowThebesLayerOGL();

  virtual bool ShouldDoubleBuffer();
  virtual void
  Swap(const ThebesBuffer& aNewFront, const nsIntRegion& aUpdatedRegion,
       OptionalThebesBuffer* aNewBack, nsIntRegion* aNewBackValidRegion,
       OptionalThebesBuffer* aReadOnlyFront, nsIntRegion* aFrontUpdatedRegion);
  virtual void EnsureTextureUpdated();
  virtual void EnsureTextureUpdated(nsIntRegion& aRegion);
  virtual void ProgressiveUpload();
  virtual void DestroyFrontBuffer();

  virtual void Disconnect();

  virtual void SetValidRegion(const nsIntRegion& aRegion)
  {
    mOldValidRegion = mValidRegion;
    ShadowThebesLayer::SetValidRegion(aRegion);
  }

  
  void Destroy();
  Layer* GetLayer();
  virtual bool IsEmpty();
  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);
  virtual void CleanupResources();

private:
  nsRefPtr<ShadowBufferOGL> mBuffer;

  
  
  nsIntRegion mRegionPendingUpload;

  
  CancelableTask* mUploadTask;

  
  ShadowThebesLayerBufferOGL mFrontBuffer;
  
  SurfaceDescriptor mFrontBufferDescriptor;
  
  
  
  
  nsIntRegion mOldValidRegion;
};

} 
} 
#endif 
