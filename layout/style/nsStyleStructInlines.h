










































#ifndef nsStyleStructInlines_h_
#define nsStyleStructInlines_h_

#include "nsStyleStruct.h"
#include "imgIRequest.h"

inline void
nsStyleBorder::SetBorderImage(imgIRequest* aImage)
{
  mBorderImage = aImage;

  



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

#endif 
