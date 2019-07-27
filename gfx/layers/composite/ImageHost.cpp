




#include "ImageHost.h"

#include "LayersLogging.h"              
#include "composite/CompositableHost.h"  
#include "ipc/IPCMessageUtils.h"        
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/Effects.h"     
#include "mozilla/layers/ImageContainerParent.h"
#include "mozilla/layers/LayerManagerComposite.h"     
#include "nsAString.h"
#include "nsDebug.h"                    
#include "nsPrintfCString.h"            
#include "nsString.h"                   

class nsIntRegion;

namespace mozilla {
namespace gfx {
class Matrix4x4;
} 

using namespace gfx;

namespace layers {

class ISurfaceAllocator;

ImageHost::ImageHost(const TextureInfo& aTextureInfo)
  : CompositableHost(aTextureInfo)
  , mImageContainer(nullptr)
  , mLastFrameID(-1)
  , mLastProducerID(-1)
  , mBias(BIAS_NONE)
  , mLocked(false)
{}

ImageHost::~ImageHost()
{
  SetImageContainer(nullptr);
}

void
ImageHost::UseTextureHost(const nsTArray<TimedTexture>& aTextures)
{
  MOZ_ASSERT(!mLocked);

  CompositableHost::UseTextureHost(aTextures);
  MOZ_ASSERT(aTextures.Length() >= 1);

  nsTArray<TimedImage> newImages;

  
  for (int32_t i = mImages.Length() - 1; i >= 0; --i) {
    if (!mImages[i].mTextureSource) {
      mImages.RemoveElementAt(i);
    }
  }

  
  
  for (uint32_t i = 0; i < aTextures.Length(); ++i) {
    const TimedTexture& t = aTextures[i];
    MOZ_ASSERT(t.mTexture);
    if (i + 1 < aTextures.Length() &&
        t.mProducerID == mLastProducerID && t.mFrameID < mLastFrameID) {
      
      
      
      
      continue;
    }
    TimedImage& img = *newImages.AppendElement();
    img.mFrontBuffer = t.mTexture;
    for (uint32_t i = 0; i < mImages.Length(); ++i) {
      if (mImages[i].mFrontBuffer == img.mFrontBuffer) {
        img.mTextureSource = mImages[i].mTextureSource;
        mImages.RemoveElementAt(i);
        break;
      }
    }
    img.mTimeStamp = t.mTimeStamp;
    img.mPictureRect = t.mPictureRect;
    img.mFrameID = t.mFrameID;
    img.mProducerID = t.mProducerID;
  }
  
  
  for (auto& img : newImages) {
    if (!img.mTextureSource && !mImages.IsEmpty()) {
      img.mTextureSource = mImages.LastElement().mTextureSource;
      mImages.RemoveElementAt(mImages.Length() - 1);
    }
    
    
    img.mFrontBuffer->SetCropRect(img.mPictureRect);
    img.mFrontBuffer->Updated();
    img.mFrontBuffer->PrepareTextureSource(img.mTextureSource);
  }
  mImages.SwapElements(newImages);
}

void
ImageHost::RemoveTextureHost(TextureHost* aTexture)
{
  MOZ_ASSERT(!mLocked);

  CompositableHost::RemoveTextureHost(aTexture);

  for (int32_t i = mImages.Length() - 1; i >= 0; --i) {
    if (mImages[i].mFrontBuffer == aTexture) {
      aTexture->UnbindTextureSource();
      mImages.RemoveElementAt(i);
    }
  }
}

static TimeStamp
GetBiasedTime(const TimeStamp& aInput, ImageHost::Bias aBias)
{
  switch (aBias) {
  case ImageHost::BIAS_NEGATIVE:
    return aInput - TimeDuration::FromMilliseconds(1.0);
  case ImageHost::BIAS_POSITIVE:
    return aInput + TimeDuration::FromMilliseconds(1.0);
  default:
    return aInput;
  }
}

static ImageHost::Bias
UpdateBias(const TimeStamp& aCompositionTime,
           const TimeStamp& aCompositedImageTime,
           const TimeStamp& aNextImageTime, 
           ImageHost::Bias aBias)
{
  if (aCompositedImageTime.IsNull()) {
    return ImageHost::BIAS_NONE;
  }
  TimeDuration threshold = TimeDuration::FromMilliseconds(1.0);
  if (aCompositionTime - aCompositedImageTime < threshold &&
      aCompositionTime - aCompositedImageTime > -threshold) {
    
    
    
    
    
    
    
    
    
    
    return ImageHost::BIAS_NEGATIVE;
  }
  if (!aNextImageTime.IsNull() &&
      aNextImageTime - aCompositionTime < threshold &&
      aNextImageTime - aCompositionTime > -threshold) {
    
    
    
    
    
    
    
    
    
    return ImageHost::BIAS_POSITIVE;
  }
  return ImageHost::BIAS_NONE;
}

int ImageHost::ChooseImageIndex() const
{
  if (!GetCompositor() || mImages.IsEmpty()) {
    return -1;
  }
  TimeStamp now = GetCompositor()->GetCompositionTime();

  if (now.IsNull()) {
    
    
    for (uint32_t i = 0; i < mImages.Length(); ++i) {
      if (mImages[i].mFrameID == mLastFrameID &&
          mImages[i].mProducerID == mLastProducerID) {
        return i;
      }
    }
    return -1;
  }

  uint32_t result = 0;
  while (result + 1 < mImages.Length() &&
      GetBiasedTime(mImages[result + 1].mTimeStamp, mBias) <= now) {
    ++result;
  }
  return result;
}

const ImageHost::TimedImage* ImageHost::ChooseImage() const
{
  int index = ChooseImageIndex();
  return index >= 0 ? &mImages[index] : nullptr;
}

ImageHost::TimedImage* ImageHost::ChooseImage()
{
  int index = ChooseImageIndex();
  return index >= 0 ? &mImages[index] : nullptr;
}

TextureHost*
ImageHost::GetAsTextureHost(IntRect* aPictureRect)
{
  TimedImage* img = ChooseImage();
  if (aPictureRect && img) {
    *aPictureRect = img->mPictureRect;
  }
  return img ? img->mFrontBuffer.get() : nullptr;
}

void ImageHost::Attach(Layer* aLayer,
                       Compositor* aCompositor,
                       AttachFlags aFlags)
{
  CompositableHost::Attach(aLayer, aCompositor, aFlags);
  for (auto& img : mImages) {
    if (GetCompositor()) {
      img.mFrontBuffer->SetCompositor(GetCompositor());
    }
    img.mFrontBuffer->Updated();
    img.mFrontBuffer->PrepareTextureSource(img.mTextureSource);
  }
}

void
ImageHost::Composite(LayerComposite* aLayer,
                     EffectChain& aEffectChain,
                     float aOpacity,
                     const gfx::Matrix4x4& aTransform,
                     const gfx::Filter& aFilter,
                     const gfx::Rect& aClipRect,
                     const nsIntRegion* aVisibleRegion)
{
  if (!GetCompositor()) {
    
    
    
    return;
  }
  int imageIndex = ChooseImageIndex();
  if (imageIndex < 0) {
    return;
  }

  if (uint32_t(imageIndex) + 1 < mImages.Length()) {
    GetCompositor()->CompositeAgainAt(mImages[imageIndex + 1].mTimeStamp);
  }

  TimedImage* img = &mImages[imageIndex];
  
  img->mFrontBuffer->SetCompositor(GetCompositor());

  {
    AutoLockCompositableHost autoLock(this);
    if (autoLock.Failed()) {
      NS_WARNING("failed to lock front buffer");
      return;
    }

    if (!img->mFrontBuffer->BindTextureSource(img->mTextureSource)) {
      return;
    }

    if (!img->mTextureSource) {
      
      MOZ_ASSERT(false);
      return;
    }

    bool isAlphaPremultiplied =
        !(img->mFrontBuffer->GetFlags() & TextureFlags::NON_PREMULTIPLIED);
    RefPtr<TexturedEffect> effect =
        CreateTexturedEffect(img->mFrontBuffer->GetFormat(),
            img->mTextureSource.get(), aFilter, isAlphaPremultiplied,
            GetRenderState());
    if (!effect) {
      return;
    }

    if (mLastFrameID != img->mFrameID || mLastProducerID != img->mProducerID) {
      if (mImageContainer) {
        aLayer->GetLayerManager()->
            AppendImageCompositeNotification(ImageCompositeNotification(
                mImageContainer, nullptr,
                img->mTimeStamp, GetCompositor()->GetCompositionTime(),
                img->mFrameID, img->mProducerID));
      }
      mLastFrameID = img->mFrameID;
      mLastProducerID = img->mProducerID;
    }
    aEffectChain.mPrimaryEffect = effect;
    gfx::Rect pictureRect(0, 0, img->mPictureRect.width, img->mPictureRect.height);
    BigImageIterator* it = img->mTextureSource->AsBigImageIterator();
    if (it) {

      
      
      
      
      
      
      
      
      
      
      
      
      
      MOZ_ASSERT(it->GetTileCount() == 1 || !img->mTextureSource->GetNextSibling(),
                 "Can't handle multi-plane BigImages");

      it->BeginBigImageIteration();
      do {
        IntRect tileRect = it->GetTileRect();
        gfx::Rect rect(tileRect.x, tileRect.y, tileRect.width, tileRect.height);
        rect = rect.Intersect(pictureRect);
        effect->mTextureCoords = Rect(Float(rect.x - tileRect.x) / tileRect.width,
                                      Float(rect.y - tileRect.y) / tileRect.height,
                                      Float(rect.width) / tileRect.width,
                                      Float(rect.height) / tileRect.height);
        if (img->mFrontBuffer->GetFlags() & TextureFlags::ORIGIN_BOTTOM_LEFT) {
          effect->mTextureCoords.y = effect->mTextureCoords.YMost();
          effect->mTextureCoords.height = -effect->mTextureCoords.height;
        }
        GetCompositor()->DrawQuad(rect, aClipRect, aEffectChain,
                                  aOpacity, aTransform);
        GetCompositor()->DrawDiagnostics(DiagnosticFlags::IMAGE | DiagnosticFlags::BIGIMAGE,
                                         rect, aClipRect, aTransform, mFlashCounter);
      } while (it->NextTile());
      it->EndBigImageIteration();
      
      GetCompositor()->DrawDiagnostics(DiagnosticFlags::IMAGE, pictureRect,
                                       aClipRect, aTransform, mFlashCounter);
    } else {
      IntSize textureSize = img->mTextureSource->GetSize();
      effect->mTextureCoords = Rect(Float(img->mPictureRect.x) / textureSize.width,
                                    Float(img->mPictureRect.y) / textureSize.height,
                                    Float(img->mPictureRect.width) / textureSize.width,
                                    Float(img->mPictureRect.height) / textureSize.height);

      if (img->mFrontBuffer->GetFlags() & TextureFlags::ORIGIN_BOTTOM_LEFT) {
        effect->mTextureCoords.y = effect->mTextureCoords.YMost();
        effect->mTextureCoords.height = -effect->mTextureCoords.height;
      }

      GetCompositor()->DrawQuad(pictureRect, aClipRect, aEffectChain,
                                aOpacity, aTransform);
      GetCompositor()->DrawDiagnostics(DiagnosticFlags::IMAGE,
                                       pictureRect, aClipRect,
                                       aTransform, mFlashCounter);
    }
  }

  
  
  
  
  
  mBias = UpdateBias(
      GetCompositor()->GetCompositionTime(), mImages[imageIndex].mTimeStamp,
      uint32_t(imageIndex + 1) < mImages.Length() ?
          mImages[imageIndex + 1].mTimeStamp : TimeStamp(),
      mBias);
}

void
ImageHost::SetCompositor(Compositor* aCompositor)
{
  if (mCompositor != aCompositor) {
    for (auto& img : mImages) {
      img.mFrontBuffer->SetCompositor(aCompositor);
    }
  }
  CompositableHost::SetCompositor(aCompositor);
}

void
ImageHost::PrintInfo(std::stringstream& aStream, const char* aPrefix)
{
  aStream << aPrefix;
  aStream << nsPrintfCString("ImageHost (0x%p)", this).get();

  nsAutoCString pfx(aPrefix);
  pfx += "  ";
  for (auto& img : mImages) {
    aStream << "\n";
    img.mFrontBuffer->PrintInfo(aStream, pfx.get());
    AppendToString(aStream, img.mPictureRect, " [picture-rect=", "]");
  }
}

void
ImageHost::Dump(std::stringstream& aStream,
                const char* aPrefix,
                bool aDumpHtml)
{
  for (auto& img : mImages) {
    aStream << aPrefix;
    aStream << (aDumpHtml ? "<ul><li>TextureHost: "
                             : "TextureHost: ");
    DumpTextureHost(aStream, img.mFrontBuffer);
    aStream << (aDumpHtml ? " </li></ul> " : " ");
  }
}

LayerRenderState
ImageHost::GetRenderState()
{
  TimedImage* img = ChooseImage();
  if (img) {
    return img->mFrontBuffer->GetRenderState();
  }
  return LayerRenderState();
}

already_AddRefed<gfx::DataSourceSurface>
ImageHost::GetAsSurface()
{
  TimedImage* img = ChooseImage();
  if (img) {
    return img->mFrontBuffer->GetAsSurface();
  }
  return nullptr;
}

bool
ImageHost::Lock()
{
  MOZ_ASSERT(!mLocked);
  TimedImage* img = ChooseImage();
  if (!img) {
    return false;
  }
  if (!img->mFrontBuffer->Lock()) {
    return false;
  }
  mLocked = true;
  return true;
}

void
ImageHost::Unlock()
{
  MOZ_ASSERT(mLocked);
  TimedImage* img = ChooseImage();
  if (img) {
    img->mFrontBuffer->Unlock();
  }
  mLocked = false;
}

IntSize
ImageHost::GetImageSize() const
{
  const TimedImage* img = ChooseImage();
  if (img) {
    return IntSize(img->mPictureRect.width, img->mPictureRect.height);
  }
  return IntSize();
}

already_AddRefed<TexturedEffect>
ImageHost::GenEffect(const gfx::Filter& aFilter)
{
  TimedImage* img = ChooseImage();
  if (!img) {
    return nullptr;
  }
  if (!img->mFrontBuffer->BindTextureSource(img->mTextureSource)) {
    return nullptr;
  }
  bool isAlphaPremultiplied = true;
  if (img->mFrontBuffer->GetFlags() & TextureFlags::NON_PREMULTIPLIED) {
    isAlphaPremultiplied = false;
  }

  return CreateTexturedEffect(img->mFrontBuffer->GetFormat(),
                              img->mTextureSource,
                              aFilter,
                              isAlphaPremultiplied,
                              GetRenderState());
}

void
ImageHost::SetImageContainer(ImageContainerParent* aImageContainer)
{
  if (mImageContainer) {
    mImageContainer->mImageHosts.RemoveElement(this);
  }
  mImageContainer = aImageContainer;
  if (mImageContainer) {
    mImageContainer->mImageHosts.AppendElement(this);
  }
}

#ifdef MOZ_WIDGET_GONK
ImageHostOverlay::ImageHostOverlay(const TextureInfo& aTextureInfo)
  : CompositableHost(aTextureInfo)
{
}

ImageHostOverlay::~ImageHostOverlay()
{
}

void
ImageHostOverlay::Composite(LayerComposite* aLayer,
                            EffectChain& aEffectChain,
                            float aOpacity,
                            const gfx::Matrix4x4& aTransform,
                            const gfx::Filter& aFilter,
                            const gfx::Rect& aClipRect,
                            const nsIntRegion* aVisibleRegion)
{
  if (!GetCompositor()) {
    return;
  }

  if (mOverlay.handle().type() == OverlayHandle::Tnull_t)
    return;
  Color hollow(0.0f, 0.0f, 0.0f, 0.0f);
  aEffectChain.mPrimaryEffect = new EffectSolidColor(hollow);
  aEffectChain.mSecondaryEffects[EffectTypes::BLEND_MODE] = new EffectBlendMode(CompositionOp::OP_SOURCE);

  gfx::Rect rect;
  gfx::Rect clipRect(aClipRect.x, aClipRect.y,
                     aClipRect.width, aClipRect.height);
  rect.SetRect(mPictureRect.x, mPictureRect.y,
               mPictureRect.width, mPictureRect.height);

  mCompositor->DrawQuad(rect, aClipRect, aEffectChain, aOpacity, aTransform);
  mCompositor->DrawDiagnostics(DiagnosticFlags::IMAGE | DiagnosticFlags::BIGIMAGE,
                               rect, aClipRect, aTransform, mFlashCounter);
}

LayerRenderState
ImageHostOverlay::GetRenderState()
{
  LayerRenderState state;
  if (mOverlay.handle().type() == OverlayHandle::Tint32_t) {
    state.SetOverlayId(mOverlay.handle().get_int32_t());
  }
  return state;
}

void
ImageHostOverlay::UseOverlaySource(OverlaySource aOverlay,
                                   const nsIntRect& aPictureRect)
{
  mOverlay = aOverlay;
  mPictureRect = aPictureRect;
}

IntSize
ImageHostOverlay::GetImageSize() const
{
  return IntSize(mPictureRect.width, mPictureRect.height);
}

void
ImageHostOverlay::PrintInfo(std::stringstream& aStream, const char* aPrefix)
{
  aStream << aPrefix;
  aStream << nsPrintfCString("ImageHost (0x%p)", this).get();

  AppendToString(aStream, mPictureRect, " [picture-rect=", "]");

  if (mOverlay.handle().type() == OverlayHandle::Tint32_t) {
    nsAutoCString pfx(aPrefix);
    pfx += "  ";
    aStream << nsPrintfCString("Overlay: %d", mOverlay.handle().get_int32_t()).get();
  }
}

#endif
} 
} 
