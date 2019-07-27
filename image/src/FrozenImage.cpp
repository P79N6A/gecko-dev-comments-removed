




#include "FrozenImage.h"

namespace mozilla {

using namespace gfx;

namespace image {

NS_IMPL_ISUPPORTS_INHERITED0(FrozenImage, ImageWrapper)

nsIntRect
FrozenImage::FrameRect(uint32_t )
{
  return InnerImage()->FrameRect(FRAME_FIRST);
}

void
FrozenImage::IncrementAnimationConsumers()
{
  
  
}

void
FrozenImage::DecrementAnimationConsumers()
{
  
}

NS_IMETHODIMP
FrozenImage::GetAnimated(bool* aAnimated)
{
  bool dummy;
  nsresult rv = InnerImage()->GetAnimated(&dummy);
  if (NS_SUCCEEDED(rv)) {
    *aAnimated = false;
  }
  return rv;
}

NS_IMETHODIMP_(TemporaryRef<SourceSurface>)
FrozenImage::GetFrame(uint32_t aWhichFrame,
                      uint32_t aFlags)
{
  return InnerImage()->GetFrame(FRAME_FIRST, aFlags);
}

NS_IMETHODIMP_(bool)
FrozenImage::FrameIsOpaque(uint32_t aWhichFrame)
{
  return InnerImage()->FrameIsOpaque(FRAME_FIRST);
}

NS_IMETHODIMP
FrozenImage::GetImageContainer(layers::LayerManager* aManager,
                               layers::ImageContainer** _retval)
{
  
  
  
  
  

  *_retval = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
FrozenImage::Draw(gfxContext* aContext,
                  const nsIntSize& aSize,
                  const ImageRegion& aRegion,
                  uint32_t ,
                  GraphicsFilter aFilter,
                  const Maybe<SVGImageContext>& aSVGContext,
                  uint32_t aFlags)
{
  return InnerImage()->Draw(aContext, aSize, aRegion, FRAME_FIRST,
                            aFilter, aSVGContext, aFlags);
}

NS_IMETHODIMP_(void)
FrozenImage::RequestRefresh(const TimeStamp& aTime)
{
  
}

NS_IMETHODIMP
FrozenImage::GetAnimationMode(uint16_t* aAnimationMode)
{
  *aAnimationMode = kNormalAnimMode;
  return NS_OK;
}

NS_IMETHODIMP
FrozenImage::SetAnimationMode(uint16_t aAnimationMode)
{
  
  return NS_OK;
}

NS_IMETHODIMP
FrozenImage::ResetAnimation()
{
  
  return NS_OK;
}

NS_IMETHODIMP_(float)
FrozenImage::GetFrameIndex(uint32_t aWhichFrame)
{
  MOZ_ASSERT(aWhichFrame <= FRAME_MAX_VALUE, "Invalid argument");
  return 0;
}

} 
} 
