










































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

  



  if (mBorderImage) {
    mBorderImage->RequestDecode();
    mBorderImage->LockImage();
  }
}

inline imgIRequest*
nsStyleBorder::GetBorderImage() const
{
  return mBorderImage;
}

inline PRBool nsStyleBorder::IsBorderImageLoaded() const
{
  PRUint32 status;
  return mBorderImage &&
         NS_SUCCEEDED(mBorderImage->GetImageStatus(&status)) &&
         (status & imgIRequest::STATUS_LOAD_COMPLETE);
}

inline void
nsStyleBorder::SetSubImage(PRUint8 aIndex, imgIContainer* aSubImage) const
{
  const_cast<nsStyleBorder*>(this)->mSubImages.ReplaceObjectAt(aSubImage, aIndex);
}

inline imgIContainer*
nsStyleBorder::GetSubImage(PRUint8 aIndex) const
{
  imgIContainer* subImage = 0;
  if (0 <= aIndex && mSubImages.Count() > aIndex)
    subImage = mSubImages[aIndex];
  return subImage;
}

#endif 
