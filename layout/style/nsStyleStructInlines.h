









#ifndef nsStyleStructInlines_h_
#define nsStyleStructInlines_h_

#include "nsIFrame.h"
#include "nsStyleStruct.h"
#include "imgIRequest.h"
#include "imgIContainer.h"

inline void
nsStyleBorder::SetBorderImage(imgIRequest* aImage)
{
  mBorderImageSource = aImage;
  mSubImages.Clear();
}

inline imgIRequest*
nsStyleBorder::GetBorderImage() const
{
  NS_ABORT_IF_FALSE(!mBorderImageSource || mImageTracked,
                    "Should be tracking any images we're going to use!");
  return mBorderImageSource;
}

inline bool nsStyleBorder::IsBorderImageLoaded() const
{
  PRUint32 status;
  return mBorderImageSource &&
         NS_SUCCEEDED(mBorderImageSource->GetImageStatus(&status)) &&
         (status & imgIRequest::STATUS_LOAD_COMPLETE) &&
         !(status & imgIRequest::STATUS_ERROR);
}

inline void
nsStyleBorder::SetSubImage(PRUint8 aIndex, imgIContainer* aSubImage) const
{
  const_cast<nsStyleBorder*>(this)->mSubImages.ReplaceObjectAt(aSubImage, aIndex);
}

inline imgIContainer*
nsStyleBorder::GetSubImage(PRUint8 aIndex) const
{
  imgIContainer* subImage = nullptr;
  if (aIndex < mSubImages.Count())
    subImage = mSubImages[aIndex];
  return subImage;
}

bool
nsStyleDisplay::IsFloating(const nsIFrame* aFrame) const
{
  return IsFloatingStyle() && !aFrame->IsSVGText();
}

bool
nsStyleDisplay::IsPositioned(const nsIFrame* aFrame) const
{
  return IsPositionedStyle() && !aFrame->IsSVGText();
}

bool
nsStyleDisplay::IsRelativelyPositioned(const nsIFrame* aFrame) const
{
  return IsRelativelyPositionedStyle() && !aFrame->IsSVGText();
}

bool
nsStyleDisplay::IsAbsolutelyPositioned(const nsIFrame* aFrame) const
{
  return IsAbsolutelyPositionedStyle() && !aFrame->IsSVGText();
}

#endif 
