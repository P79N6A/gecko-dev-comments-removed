










































#ifndef nsStyleStructInlines_h_
#define nsStyleStructInlines_h_

#include "nsStyleStruct.h"
#include "imgIRequest.h"
#include "imgIContainer.h"

inline void
nsStyleBorder::SetBorderImage(imgIRequest* aImage)
{
  mBorderImage = aImage;
  mSubImages.Clear();
}

inline imgIRequest*
nsStyleBorder::GetBorderImage() const
{
  NS_ABORT_IF_FALSE(!mBorderImage || mImageTracked,
                    "Should be tracking any images we're going to use!");
  return mBorderImage;
}

inline bool nsStyleBorder::IsBorderImageLoaded() const
{
  PRUint32 status;
  return mBorderImage &&
         NS_SUCCEEDED(mBorderImage->GetImageStatus(&status)) &&
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
  imgIContainer* subImage = nsnull;
  if (aIndex < mSubImages.Count())
    subImage = mSubImages[aIndex];
  return subImage;
}

#endif 
