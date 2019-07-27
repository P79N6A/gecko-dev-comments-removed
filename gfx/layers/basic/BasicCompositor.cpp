




#include "BasicCompositor.h"
#include "BasicLayersImpl.h"            
#include "TextureHostBasic.h"
#include "mozilla/layers/Effects.h"
#include "mozilla/layers/YCbCrImageDataSerializer.h"
#include "nsIWidget.h"
#include "gfx2DGlue.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/Helpers.h"
#include "gfxUtils.h"
#include "YCbCrUtils.h"
#include <algorithm>
#include "ImageContainer.h"
#include "gfxPrefs.h"
#include "skia/SkCanvas.h"              
#include "skia/SkBitmapDevice.h"        

namespace mozilla {
using namespace mozilla::gfx;

namespace layers {

class DataTextureSourceBasic : public DataTextureSource
                             , public TextureSourceBasic
{
public:

  virtual TextureSourceBasic* AsSourceBasic() MOZ_OVERRIDE { return this; }

  virtual gfx::SourceSurface* GetSurface(DrawTarget* aTarget) MOZ_OVERRIDE { return mSurface; }

  SurfaceFormat GetFormat() const MOZ_OVERRIDE
  {
    return mSurface->GetFormat();
  }

  virtual IntSize GetSize() const MOZ_OVERRIDE
  {
    return mSurface->GetSize();
  }

  virtual bool Update(gfx::DataSourceSurface* aSurface,
                      nsIntRegion* aDestRegion = nullptr,
                      gfx::IntPoint* aSrcOffset = nullptr) MOZ_OVERRIDE
  {
    mSurface = aSurface;
    return true;
  }

  virtual void DeallocateDeviceData() MOZ_OVERRIDE
  {
    mSurface = nullptr;
    SetUpdateSerial(0);
  }

public:
  RefPtr<gfx::DataSourceSurface> mSurface;
};

BasicCompositor::BasicCompositor(nsIWidget *aWidget)
  : mWidget(aWidget)
{
  MOZ_COUNT_CTOR(BasicCompositor);
  SetBackend(LayersBackend::LAYERS_BASIC);

  mMaxTextureSize =
    Factory::GetMaxSurfaceSize(gfxPlatform::GetPlatform()->GetContentBackend());
}

BasicCompositor::~BasicCompositor()
{
  MOZ_COUNT_DTOR(BasicCompositor);
}

int32_t
BasicCompositor::GetMaxTextureSize() const
{
  return mMaxTextureSize;
}

void
BasicCompositingRenderTarget::BindRenderTarget()
{
  if (mClearOnBind) {
    mDrawTarget->ClearRect(Rect(0, 0, mSize.width, mSize.height));
    mClearOnBind = false;
  }
}

void BasicCompositor::Destroy()
{
  mWidget->CleanupRemoteDrawing();
  mWidget = nullptr;
}

TemporaryRef<CompositingRenderTarget>
BasicCompositor::CreateRenderTarget(const IntRect& aRect, SurfaceInitMode aInit)
{
  MOZ_ASSERT(aRect.width != 0 && aRect.height != 0, "Trying to create a render target of invalid size");

  if (aRect.width * aRect.height == 0) {
    return nullptr;
  }

  RefPtr<DrawTarget> target = mDrawTarget->CreateSimilarDrawTarget(aRect.Size(), SurfaceFormat::B8G8R8A8);

  if (!target) {
    return nullptr;
  }

  RefPtr<BasicCompositingRenderTarget> rt = new BasicCompositingRenderTarget(target, aRect);

  return rt.forget();
}

TemporaryRef<CompositingRenderTarget>
BasicCompositor::CreateRenderTargetFromSource(const IntRect &aRect,
                                              const CompositingRenderTarget *aSource,
                                              const IntPoint &aSourcePoint)
{
  MOZ_CRASH("Shouldn't be called!");
  return nullptr;
}

TemporaryRef<DataTextureSource>
BasicCompositor::CreateDataTextureSource(TextureFlags aFlags)
{
  RefPtr<DataTextureSource> result = new DataTextureSourceBasic();
  return result.forget();
}

bool
BasicCompositor::SupportsEffect(EffectTypes aEffect)
{
  return static_cast<EffectTypes>(aEffect) != EffectTypes::YCBCR;
}

static void
DrawSurfaceWithTextureCoords(DrawTarget *aDest,
                             const gfx::Rect& aDestRect,
                             SourceSurface *aSource,
                             const gfx::Rect& aTextureCoords,
                             gfx::Filter aFilter,
                             float aOpacity,
                             SourceSurface *aMask,
                             const Matrix* aMaskTransform)
{
  
  gfxRect sourceRect(aTextureCoords.x * aSource->GetSize().width,
                     aTextureCoords.y * aSource->GetSize().height,
                     aTextureCoords.width * aSource->GetSize().width,
                     aTextureCoords.height * aSource->GetSize().height);

  
  
  sourceRect.Round();

  
  Matrix matrix =
    gfxUtils::TransformRectToRect(sourceRect,
                                  gfx::IntPoint(aDestRect.x, aDestRect.y),
                                  gfx::IntPoint(aDestRect.XMost(), aDestRect.y),
                                  gfx::IntPoint(aDestRect.XMost(), aDestRect.YMost()));

  
  gfx::Rect unitRect(0, 0, 1, 1);
  ExtendMode mode = unitRect.Contains(aTextureCoords) ? ExtendMode::CLAMP : ExtendMode::REPEAT;

  FillRectWithMask(aDest, aDestRect, aSource, aFilter, DrawOptions(aOpacity),
                   mode, aMask, aMaskTransform, &matrix);
}

static SkMatrix
Matrix3DToSkia(const gfx3DMatrix& aMatrix)
{
  SkMatrix transform;
  transform.setAll(aMatrix._11,
                   aMatrix._21,
                   aMatrix._41,
                   aMatrix._12,
                   aMatrix._22,
                   aMatrix._42,
                   aMatrix._14,
                   aMatrix._24,
                   aMatrix._44);

  return transform;
}

static void
SkiaTransform(DataSourceSurface* aDest,
              DataSourceSurface* aSource,
              const gfx3DMatrix& aTransform,
              const Point& aDestOffset)
{
  if (aTransform.IsSingular()) {
    return;
  }

  IntSize destSize = aDest->GetSize();
  SkImageInfo destInfo = SkImageInfo::Make(destSize.width,
                                           destSize.height,
                                           kBGRA_8888_SkColorType,
                                           kPremul_SkAlphaType);
  SkBitmap destBitmap;
  destBitmap.setInfo(destInfo, aDest->Stride());
  destBitmap.setPixels((uint32_t*)aDest->GetData());
  SkCanvas destCanvas(destBitmap);

  IntSize srcSize = aSource->GetSize();
  SkImageInfo srcInfo = SkImageInfo::Make(srcSize.width,
                                          srcSize.height,
                                          kBGRA_8888_SkColorType,
                                          kPremul_SkAlphaType);
  SkBitmap src;
  src.setInfo(srcInfo, aSource->Stride());
  src.setPixels((uint32_t*)aSource->GetData());

  gfx3DMatrix transform = aTransform;
  transform.TranslatePost(Point3D(-aDestOffset.x, -aDestOffset.y, 0));
  destCanvas.setMatrix(Matrix3DToSkia(transform));

  SkPaint paint;
  paint.setXfermodeMode(SkXfermode::kSrc_Mode);
  paint.setAntiAlias(true);
  paint.setFilterLevel(SkPaint::kLow_FilterLevel);
  SkRect destRect = SkRect::MakeXYWH(0, 0, srcSize.width, srcSize.height);
  destCanvas.drawBitmapRectToRect(src, nullptr, destRect, &paint);
}

static inline IntRect
RoundOut(Rect r)
{
  r.RoundOut();
  return IntRect(r.x, r.y, r.width, r.height);
}

void
BasicCompositor::DrawQuad(const gfx::Rect& aRect,
                          const gfx::Rect& aClipRect,
                          const EffectChain &aEffectChain,
                          gfx::Float aOpacity,
                          const gfx::Matrix4x4 &aTransform)
{
  RefPtr<DrawTarget> buffer = mRenderTarget->mDrawTarget;

  
  
  RefPtr<DrawTarget> dest = buffer;

  buffer->PushClipRect(aClipRect);
  AutoRestoreTransform autoRestoreTransform(dest);

  Matrix newTransform;
  Rect transformBounds;
  gfx3DMatrix new3DTransform;
  IntPoint offset = mRenderTarget->GetOrigin();

  if (aTransform.Is2D()) {
    newTransform = aTransform.As2D();
  } else {
    
    dest = gfxPlatform::GetPlatform()->CreateOffscreenContentDrawTarget(RoundOut(aRect).Size(), SurfaceFormat::B8G8R8A8);
    if (!dest) {
      return;
    }

    dest->SetTransform(Matrix::Translation(-aRect.x, -aRect.y));

    
    new3DTransform = To3DMatrix(aTransform);
    gfxRect bounds = new3DTransform.TransformBounds(ThebesRect(aRect));
    bounds.IntersectRect(bounds, gfxRect(offset.x, offset.y, buffer->GetSize().width, buffer->GetSize().height));

    transformBounds = ToRect(bounds);
    transformBounds.RoundOut();

    
    newTransform = Matrix::Translation(transformBounds.x, transformBounds.y);

    
    
    new3DTransform = gfx3DMatrix::Translation(aRect.x, aRect.y, 0) * new3DTransform;
  }

  newTransform.PostTranslate(-offset.x, -offset.y);
  buffer->SetTransform(newTransform);

  RefPtr<SourceSurface> sourceMask;
  Matrix maskTransform;
  if (aEffectChain.mSecondaryEffects[EffectTypes::MASK]) {
    EffectMask *effectMask = static_cast<EffectMask*>(aEffectChain.mSecondaryEffects[EffectTypes::MASK].get());
    sourceMask = effectMask->mMaskTexture->AsSourceBasic()->GetSurface(dest);
    MOZ_ASSERT(effectMask->mMaskTransform.Is2D(), "How did we end up with a 3D transform here?!");
    MOZ_ASSERT(!effectMask->mIs3D);
    maskTransform = effectMask->mMaskTransform.As2D();
    maskTransform.PreTranslate(-offset.x, -offset.y);
  }

  switch (aEffectChain.mPrimaryEffect->mType) {
    case EffectTypes::SOLID_COLOR: {
      EffectSolidColor* effectSolidColor =
        static_cast<EffectSolidColor*>(aEffectChain.mPrimaryEffect.get());

      FillRectWithMask(dest, aRect, effectSolidColor->mColor,
                       DrawOptions(aOpacity), sourceMask, &maskTransform);
      break;
    }
    case EffectTypes::RGB: {
      TexturedEffect* texturedEffect =
          static_cast<TexturedEffect*>(aEffectChain.mPrimaryEffect.get());
      TextureSourceBasic* source = texturedEffect->mTexture->AsSourceBasic();

      if (texturedEffect->mPremultiplied) {
          DrawSurfaceWithTextureCoords(dest, aRect,
                                       source->GetSurface(dest),
                                       texturedEffect->mTextureCoords,
                                       texturedEffect->mFilter,
                                       aOpacity, sourceMask, &maskTransform);
      } else {
          RefPtr<DataSourceSurface> srcData = source->GetSurface(dest)->GetDataSurface();

          
          
          RefPtr<DataSourceSurface> premultData = gfxUtils::CreatePremultipliedDataSurface(srcData);

          DrawSurfaceWithTextureCoords(dest, aRect,
                                       premultData,
                                       texturedEffect->mTextureCoords,
                                       texturedEffect->mFilter,
                                       aOpacity, sourceMask, &maskTransform);
      }
      break;
    }
    case EffectTypes::YCBCR: {
      NS_RUNTIMEABORT("Can't (easily) support component alpha with BasicCompositor!");
      break;
    }
    case EffectTypes::RENDER_TARGET: {
      EffectRenderTarget* effectRenderTarget =
        static_cast<EffectRenderTarget*>(aEffectChain.mPrimaryEffect.get());
      RefPtr<BasicCompositingRenderTarget> surface
        = static_cast<BasicCompositingRenderTarget*>(effectRenderTarget->mRenderTarget.get());
      RefPtr<SourceSurface> sourceSurf = surface->mDrawTarget->Snapshot();

      DrawSurfaceWithTextureCoords(dest, aRect,
                                   sourceSurf,
                                   effectRenderTarget->mTextureCoords,
                                   effectRenderTarget->mFilter,
                                   aOpacity, sourceMask, &maskTransform);
      break;
    }
    case EffectTypes::COMPONENT_ALPHA: {
      NS_RUNTIMEABORT("Can't (easily) support component alpha with BasicCompositor!");
      break;
    }
    default: {
      NS_RUNTIMEABORT("Invalid effect type!");
      break;
    }
  }

  if (!aTransform.Is2D()) {
    dest->Flush();

    RefPtr<SourceSurface> snapshot = dest->Snapshot();
    RefPtr<DataSourceSurface> source = snapshot->GetDataSurface();
    RefPtr<DataSourceSurface> temp =
      Factory::CreateDataSourceSurface(RoundOut(transformBounds).Size(), SurfaceFormat::B8G8R8A8, true);
    if (NS_WARN_IF(!temp)) {
      return;
    }

    SkiaTransform(temp, source, new3DTransform, transformBounds.TopLeft());

    transformBounds.MoveTo(0, 0);
    buffer->DrawSurface(temp, transformBounds, transformBounds);
  }

  buffer->PopClip();
}

void
BasicCompositor::ClearRect(const gfx::Rect& aRect)
{
  mRenderTarget->mDrawTarget->ClearRect(aRect);
}

void
BasicCompositor::BeginFrame(const nsIntRegion& aInvalidRegion,
                            const gfx::Rect *aClipRectIn,
                            const gfx::Rect& aRenderBounds,
                            gfx::Rect *aClipRectOut ,
                            gfx::Rect *aRenderBoundsOut )
{
  mWidgetSize = mWidget->GetClientSize();
  IntRect intRect = gfx::IntRect(IntPoint(), mWidgetSize);
  Rect rect = Rect(0, 0, intRect.width, intRect.height);

  
  nsIntRegion invalidRegionSafe;
  invalidRegionSafe.And(aInvalidRegion, gfx::ThebesIntRect(intRect));

  nsIntRect invalidRect = invalidRegionSafe.GetBounds();
  mInvalidRect = IntRect(invalidRect.x, invalidRect.y, invalidRect.width, invalidRect.height);
  mInvalidRegion = invalidRegionSafe;

  if (aRenderBoundsOut) {
    *aRenderBoundsOut = Rect();
  }

  if (mInvalidRect.width <= 0 || mInvalidRect.height <= 0) {
    return;
  }

  if (mTarget) {
    
    
    mDrawTarget = gfxPlatform::GetPlatform()->CreateOffscreenContentDrawTarget(IntSize(1,1), SurfaceFormat::B8G8R8A8);
  } else {
    mDrawTarget = mWidget->StartRemoteDrawing();
  }
  if (!mDrawTarget) {
    return;
  }

  
  
  RefPtr<CompositingRenderTarget> target = CreateRenderTarget(mInvalidRect, INIT_MODE_CLEAR);
  if (!target) {
    if (!mTarget) {
      mWidget->EndRemoteDrawing();
    }
    return;
  }
  SetRenderTarget(target);

  
  
  mRenderTarget->mDrawTarget->SetTransform(Matrix::Translation(-invalidRect.x,
                                                               -invalidRect.y));

  gfxUtils::ClipToRegion(mRenderTarget->mDrawTarget, invalidRegionSafe);

  if (aRenderBoundsOut) {
    *aRenderBoundsOut = rect;
  }

  if (aClipRectIn) {
    mRenderTarget->mDrawTarget->PushClipRect(*aClipRectIn);
  } else {
    mRenderTarget->mDrawTarget->PushClipRect(rect);
    if (aClipRectOut) {
      *aClipRectOut = rect;
    }
  }
}

void
BasicCompositor::EndFrame()
{
  
  mRenderTarget->mDrawTarget->PopClip();

  if (gfxPrefs::WidgetUpdateFlashing()) {
    float r = float(rand()) / RAND_MAX;
    float g = float(rand()) / RAND_MAX;
    float b = float(rand()) / RAND_MAX;
    
    mRenderTarget->mDrawTarget->FillRect(ToRect(mInvalidRegion.GetBounds()),
                                         ColorPattern(Color(r, g, b, 0.2f)));
  }

  
  mRenderTarget->mDrawTarget->PopClip();

  
  
  RefPtr<SourceSurface> source = mRenderTarget->mDrawTarget->Snapshot();
  RefPtr<DrawTarget> dest(mTarget ? mTarget : mDrawTarget);

  nsIntPoint offset = mTarget ? mTargetBounds.TopLeft() : nsIntPoint();

  
  
  
  nsIntRegionRectIterator iter(mInvalidRegion);
  for (const nsIntRect *r = iter.Next(); r; r = iter.Next()) {
    dest->CopySurface(source,
                      IntRect(r->x - mInvalidRect.x, r->y - mInvalidRect.y, r->width, r->height),
                      IntPoint(r->x - offset.x, r->y - offset.y));
  }
  if (!mTarget) {
    mWidget->EndRemoteDrawing();
  }

  mDrawTarget = nullptr;
  mRenderTarget = nullptr;
}

}
}
