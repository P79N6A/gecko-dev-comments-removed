










































#ifndef nsStyleStructInlines_h_
#define nsStyleStructInlines_h_

#include "nsStyleStruct.h"
#include "imgIRequest.h"

inline void
nsStyleBorder::SetBorderImage(imgIRequest* aImage)
{
  mBorderImage = aImage;
  RebuildActualBorder();
}

inline imgIRequest*
nsStyleBorder::GetBorderImage() const
{
  return mBorderImage;
}

inline PRBool nsStyleBorder::HasVisibleStyle(PRUint8 aSide)
{
  PRUint8 style = GetBorderStyle(aSide);
  return (style != NS_STYLE_BORDER_STYLE_NONE &&
          style != NS_STYLE_BORDER_STYLE_HIDDEN);
}

inline void nsStyleBorder::SetBorderWidth(PRUint8 aSide, nscoord aBorderWidth)
{
  nscoord roundedWidth =
    NS_ROUND_BORDER_TO_PIXELS(aBorderWidth, mTwipsPerPixel);
  mBorder.side(aSide) = roundedWidth;
  if (HasVisibleStyle(aSide))
    mComputedBorder.side(aSide) = roundedWidth;
}

inline void nsStyleBorder::SetBorderImageWidthOverride(PRUint8 aSide,
                                                       nscoord aBorderWidth)
{
  mBorderImageWidth.side(aSide) =
    NS_ROUND_BORDER_TO_PIXELS(aBorderWidth, mTwipsPerPixel);
}

inline void nsStyleBorder::RebuildActualBorderSide(PRUint8 aSide)
{
  mComputedBorder.side(aSide) =
    (HasVisibleStyle(aSide) ? mBorder.side(aSide) : 0);
}

inline void nsStyleBorder::SetBorderStyle(PRUint8 aSide, PRUint8 aStyle)
{
  NS_ASSERTION(aSide <= NS_SIDE_LEFT, "bad side"); 
  mBorderStyle[aSide] &= ~BORDER_STYLE_MASK; 
  mBorderStyle[aSide] |= (aStyle & BORDER_STYLE_MASK);
  RebuildActualBorderSide(aSide);
}

inline void nsStyleBorder::RebuildActualBorder()
{
  NS_FOR_CSS_SIDES(side) {
    RebuildActualBorderSide(side);
  }
}

inline PRBool nsStyleBorder::IsBorderImageLoaded() const
{
  PRUint32 status;
  return mBorderImage &&
         NS_SUCCEEDED(mBorderImage->GetImageStatus(&status)) &&
         (status & imgIRequest::STATUS_FRAME_COMPLETE);
}

#endif 
